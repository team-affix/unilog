import { BaseErrorListener, CharStream, CommonTokenStream, ParserRuleContext, Recognizer, Token } from 'antlr4ng';
import { Loc, span, UnilogError } from './errors';
import { UnilogLexer } from './generated/UnilogLexer';
import {
    ApplyContext,
    AssumeContext,
    ClaimDeclContext,
    DeclContext,
    FileContext,
    ImportDeclContext,
    PropContext,
    PropAtomContext,
    PropNameContext,
    ProofAtomContext,
    ProofContext,
    ProofNameContext,
    TheoremDeclContext,
    UnilogParser,
} from './generated/UnilogParser';
import { Decl, PropExpr, ProofExpr } from './syntax';

/**
 * Parses Unilog source with the ANTLR grammar in grammar/Unilog.g4, then turns the parse tree into the
 * AST the checker uses. The grammar handles structure; the rules about names (which names are
 * variables, where qualified names are allowed) are applied here.
 */

/** True for names that denote variables in propositions: starting with an uppercase letter or '_'. */
export function isVariableName(name: string): boolean {
    return /^[A-Z_]/.test(name);
}

/** Turns ANTLR syntax errors into UnilogErrors with a location, stopping at the first one. */
class ThrowingErrorListener extends BaseErrorListener {
    constructor(private readonly file: string) {
        super();
    }

    override syntaxError(
        _recognizer: Recognizer<never>,
        offendingSymbol: unknown,
        line: number,
        charPositionInLine: number,
        msg: string,
    ): void {
        const loc = { file: this.file, line, col: charPositionInLine + 1 };
        const tok = offendingSymbol as Token | null;
        if (tok?.type === UnilogLexer.UNEXPECTED) throw new UnilogError(`Unexpected character '${tok.text}'`, loc);
        throw new UnilogError(
            `Syntax error: ${msg.replace('<EOF>', 'end of file').replace(/\bNAME\b/g, 'a name')}`,
            loc,
        );
    }
}

function makeParser(src: string, file: string): UnilogParser {
    const errors = new ThrowingErrorListener(file);
    const lexer = new UnilogLexer(CharStream.fromString(src));
    lexer.removeErrorListeners();
    lexer.addErrorListener(errors);
    const parser = new UnilogParser(new CommonTokenStream(lexer));
    parser.removeErrorListeners();
    parser.addErrorListener(errors);
    return parser;
}

/** Parses a file of declarations. */
export function parse(src: string, file: string): Decl[] {
    return converters(file).file(makeParser(src, file).file());
}

/** Parses a single proposition, e.g. a query. */
export function parseProposition(src: string, file = '<query>'): PropExpr {
    return converters(file).prop(makeParser(src, file).query().prop());
}

/** Functions turning parse-tree nodes into AST nodes, reporting locations (with ranges) in `file`. */
function converters(file: string) {
    const locOf = (ctx: ParserRuleContext | Token): Loc => {
        const start = ctx instanceof ParserRuleContext ? ctx.start : ctx;
        const stop = ctx instanceof ParserRuleContext ? (ctx.stop ?? ctx.start) : ctx;
        return {
            file,
            line: start?.line ?? 0,
            col: (start?.column ?? 0) + 1,
            // Tokens never span lines: a token ends on its own line, its length after its start.
            end: { line: stop?.line ?? 0, col: (stop?.column ?? 0) + 1 + (stop?.text?.length ?? 0) },
        };
    };

    const simpleName = (tok: Token, role: string): string => {
        const text = tok.text ?? '';
        if (text.includes('.'))
            throw new UnilogError(
                `${role} cannot be a qualified name: '${text}' (if the '.' ends a declaration, put a space or newline after it)`,
                locOf(tok),
            );
        return text;
    };

    function prop(ctx: PropContext): PropExpr {
        const atoms = ctx.propApp().propAtom().map(propAtom);
        const left = atoms
            .slice(1)
            .reduce((fn, arg) => ({ k: 'app', fn, arg, loc: span(fn.loc, arg.loc) }) as PropExpr, atoms[0] as PropExpr);
        const rest = ctx.prop();
        if (!rest) return left;
        const to = prop(rest);
        return { k: 'imp', from: left, to, loc: span(left.loc, to.loc) };
    }

    function propAtom(ctx: PropAtomContext): PropExpr {
        if (ctx instanceof PropNameContext) {
            const name = simpleName(ctx.NAME().symbol, 'A symbol in a proposition');
            return { k: isVariableName(name) ? 'var' : 'atom', name, loc: locOf(ctx) };
        }
        // A parenthesized proposition's range includes its parentheses.
        return { ...prop((ctx as PropAtomContext & { prop(): PropContext }).prop()), loc: locOf(ctx) };
    }

    function proof(ctx: ProofContext): ProofExpr {
        if (ctx instanceof AssumeContext) {
            const param = simpleName(ctx.NAME().symbol, 'An assumption');
            if (isVariableName(param))
                throw new UnilogError(`Assumption names must start with a lowercase letter: '${param}'`, locOf(ctx));
            return { k: 'lam', param, body: proof(ctx.proof()), loc: locOf(ctx), paramLoc: locOf(ctx.NAME().symbol) };
        }
        const atoms = (ctx as ApplyContext).proofAtom().map(proofAtom);
        return atoms
            .slice(1)
            .reduce(
                (fn, arg) => ({ k: 'app', fn, arg, loc: span(fn.loc, arg.loc) }) as ProofExpr,
                atoms[0] as ProofExpr,
            );
    }

    function proofAtom(ctx: ProofAtomContext): ProofExpr {
        if (ctx instanceof ProofNameContext)
            return { k: 'ref', path: ctx.NAME().getText().split('.'), loc: locOf(ctx) };
        // A parenthesized step's range includes its parentheses.
        return { ...proof((ctx as ProofAtomContext & { proof(): ProofContext }).proof()), loc: locOf(ctx) };
    }

    function decl(ctx: DeclContext): Decl {
        if (ctx instanceof ImportDeclContext)
            return {
                k: 'import',
                path: ctx.NAME().getText().split('.'),
                loc: locOf(ctx),
                pathLoc: locOf(ctx.NAME().symbol),
            };
        const named = ctx as ClaimDeclContext | TheoremDeclContext;
        const name = simpleName(named.NAME().symbol, 'A declaration name');
        const nameLoc = locOf(named.NAME().symbol);
        if (!/^[a-z0-9]/.test(name))
            throw new UnilogError(`Declaration names must start with a lowercase letter or digit: '${name}'`, nameLoc);
        if (ctx instanceof TheoremDeclContext)
            return { k: 'theorem', name, proof: proof(ctx.proof()), loc: locOf(ctx), nameLoc };
        const claim = ctx as ClaimDeclContext;
        const p = prop(claim.prop());
        const pr = claim.proof();
        return pr
            ? { k: 'theorem', name, prop: p, proof: proof(pr), loc: locOf(ctx), nameLoc }
            : { k: 'axiom', name, prop: p, loc: locOf(ctx), nameLoc };
    }

    return { prop, file: (tree: FileContext) => tree.decl().map(decl) };
}
