import { BaseErrorListener, CharStream, CommonTokenStream, ParserRuleContext, Recognizer, Token } from 'antlr4ng';
import { Loc, UnilogError } from './errors';
import { UnilogLexer } from './generated/UnilogLexer';
import {
    ApplyContext,
    AssumeContext,
    ClaimDeclContext,
    DeclContext,
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

export function parse(src: string, file: string): Decl[] {
    const errors = new ThrowingErrorListener(file);
    const lexer = new UnilogLexer(CharStream.fromString(src));
    lexer.removeErrorListeners();
    lexer.addErrorListener(errors);
    const parser = new UnilogParser(new CommonTokenStream(lexer));
    parser.removeErrorListeners();
    parser.addErrorListener(errors);
    const tree = parser.file();

    const locOf = (ctx: ParserRuleContext | Token): Loc => {
        const start = ctx instanceof ParserRuleContext ? ctx.start : ctx;
        return { file, line: start?.line ?? 0, col: (start?.column ?? 0) + 1 };
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
            .reduce((fn, arg) => ({ k: 'app', fn, arg, loc: fn.loc }) as PropExpr, atoms[0] as PropExpr);
        const rest = ctx.prop();
        return rest ? { k: 'imp', from: left, to: prop(rest), loc: left.loc } : left;
    }

    function propAtom(ctx: PropAtomContext): PropExpr {
        if (ctx instanceof PropNameContext) {
            const name = simpleName(ctx.NAME().symbol, 'A symbol in a proposition');
            return { k: isVariableName(name) ? 'var' : 'atom', name, loc: locOf(ctx) };
        }
        return prop((ctx as PropAtomContext & { prop(): PropContext }).prop());
    }

    function proof(ctx: ProofContext): ProofExpr {
        if (ctx instanceof AssumeContext) {
            const param = simpleName(ctx.NAME().symbol, 'An assumption');
            if (isVariableName(param))
                throw new UnilogError(`Assumption names must start with a lowercase letter: '${param}'`, locOf(ctx));
            return { k: 'lam', param, body: proof(ctx.proof()), loc: locOf(ctx) };
        }
        const atoms = (ctx as ApplyContext).proofAtom().map(proofAtom);
        return atoms
            .slice(1)
            .reduce((fn, arg) => ({ k: 'app', fn, arg, loc: fn.loc }) as ProofExpr, atoms[0] as ProofExpr);
    }

    function proofAtom(ctx: ProofAtomContext): ProofExpr {
        if (ctx instanceof ProofNameContext)
            return { k: 'ref', path: ctx.NAME().getText().split('.'), loc: locOf(ctx) };
        return proof((ctx as ProofAtomContext & { proof(): ProofContext }).proof());
    }

    function decl(ctx: DeclContext): Decl {
        if (ctx instanceof ImportDeclContext)
            return { k: 'import', path: ctx.NAME().getText().split('.'), loc: locOf(ctx) };
        const named = ctx as ClaimDeclContext | TheoremDeclContext;
        const name = simpleName(named.NAME().symbol, 'A declaration name');
        if (!/^[a-z0-9]/.test(name))
            throw new UnilogError(
                `Declaration names must start with a lowercase letter or digit: '${name}'`,
                locOf(ctx),
            );
        if (ctx instanceof TheoremDeclContext)
            return { k: 'theorem', name, proof: proof(ctx.proof()), loc: locOf(ctx) };
        const claim = ctx as ClaimDeclContext;
        const p = prop(claim.prop());
        const pr = claim.proof();
        return pr
            ? { k: 'theorem', name, prop: p, proof: proof(pr), loc: locOf(ctx) }
            : { k: 'axiom', name, prop: p, loc: locOf(ctx) };
    }

    return tree.decl().map(decl);
}
