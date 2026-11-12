import { BaseErrorListener, CharStream, CommonTokenStream, ParserRuleContext, Recognizer, Token } from 'antlr4ng';
import { Loc, span, UnilogError } from './errors';
import { UnilogLexer } from './generated/UnilogLexer';
import {
    ApplyContext,
    ArrowAssumeContext,
    AssumeContext,
    BadAssumeContext,
    OldAssumeContext,
    ClaimDeclContext,
    DeclContext,
    FileContext,
    ImportDeclContext,
    PropContext,
    PropImpContext,
    PropLamContext,
    PropAtomContext,
    PropNameContext,
    ProofAtomContext,
    ProofContext,
    ProofNameContext,
    InferenceDeclContext,
    TerminatorContext,
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
        // '.' is only in the grammar to report the old terminator, so it is never suggested.
        const text = msg
            .replace('<EOF>', 'end of file')
            .replace(/\bNAME\b/g, 'a name')
            .replace(/\bLAMBDA\b/g, "'\\X.'")
            .replace(/, '\.'|'\.', /g, '');
        throw new UnilogError(`Syntax error: ${text}`, loc);
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
        if (text.includes('.')) throw new UnilogError(`${role} cannot be a qualified name: '${text}'`, locOf(tok));
        return text;
    };

    /** \X Y. body, as nested single-variable lambdas. Variables bound in propositions are uppercase. */
    function propLambda(tok: Token, body: PropExpr, loc: Loc): PropExpr {
        return lambdaParams(tok).reduceRight<PropExpr>((inner, { name, loc: paramLoc }) => {
            if (!isVariableName(name))
                throw new UnilogError(
                    `A lambda binds a variable, so its name starts with an uppercase letter or '_': '${name}'`,
                    paramLoc,
                );
            return { k: 'lam', param: name, body: inner, loc, paramLoc };
        }, body);
    }

    function prop(ctx: PropContext): PropExpr {
        if (ctx instanceof PropLamContext) return propLambda(ctx.LAMBDA().symbol, prop(ctx.prop()), locOf(ctx));
        const imp = ctx as PropImpContext;
        const appCtx = imp.propApp();
        const args = appCtx.propAtom().map(propAtom);
        const tail = appCtx.LAMBDA();
        const tailBody = appCtx.prop();
        if (tail && tailBody) {
            const start = locOf(tail.symbol);
            args.push(propLambda(tail.symbol, prop(tailBody), span(start, locOf(tailBody))));
        }
        const left = args
            .slice(1)
            .reduce((fn, arg) => ({ k: 'app', fn, arg, loc: span(fn.loc, arg.loc) }) as PropExpr, args[0] as PropExpr);
        const rest = imp.prop();
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

    /** The names in an assumption head such as '\F G X.', each with its location. */
    const lambdaParams = (tok: Token): { name: string; loc: Loc }[] => {
        const text = tok.text ?? '';
        const params: { name: string; loc: Loc }[] = [];
        let line = tok.line;
        let col = tok.column + 1;
        for (let i = 0; i < text.length;) {
            const m = /^[A-Za-z0-9_']+/.exec(text.slice(i));
            if (m) {
                params.push({ name: m[0], loc: { file, line, col, end: { line, col: col + m[0].length } } });
                i += m[0].length;
                col += m[0].length;
            } else {
                if (text[i] === '\n') {
                    line++;
                    col = 1;
                } else col++;
                i++;
            }
        }
        return params;
    };

    function proof(ctx: ProofContext): ProofExpr {
        if (ctx instanceof OldAssumeContext) {
            const name = ctx.NAME().getText();
            throw new UnilogError(
                `Assumptions are written with a backslash: '\\${name}. ...' rather than '${name} => ...' ('=>' means implication)`,
                locOf(ctx),
            );
        }
        if (ctx instanceof ArrowAssumeContext || ctx instanceof BadAssumeContext) {
            const names = ctx.NAME().map((n) => n.getText());
            const head = `\\${names.length ? names.join(' ') : 'P'}.`;
            throw new UnilogError(
                ctx instanceof ArrowAssumeContext
                    ? `Assumptions are written '${head} ...' rather than '\\${names.join(' ')} -> ...'`
                    : `An assumption needs a '.' after its name: '${head} ...'`,
                locOf(ctx),
            );
        }
        if (ctx instanceof AssumeContext) {
            // \P Q. body is \P. \Q. body: one assumption per name, innermost last.
            const body = proof(ctx.proof());
            const loc = locOf(ctx);
            return lambdaParams(ctx.LAMBDA().symbol).reduceRight<ProofExpr>((inner, { name: param, loc: paramLoc }) => {
                if (!isVariableName(param))
                    throw new UnilogError(
                        `Assumptions are variables, so their names start with an uppercase letter or '_': '${param}'`,
                        paramLoc,
                    );
                return { k: 'lam', param, body: inner, loc, paramLoc };
            }, body);
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
        const end = (ctx as DeclContext & { terminator(): TerminatorContext }).terminator();
        if (end.DOT())
            throw new UnilogError(
                "Declarations end with ';' ('.' only joins the parts of a qualified name, and ends an assumption's '\\P.')",
                locOf(end),
            );
        if (ctx instanceof ImportDeclContext)
            return {
                k: 'import',
                path: ctx.NAME().getText().split('.'),
                loc: locOf(ctx),
                pathLoc: locOf(ctx.NAME().symbol),
            };
        const named = ctx as ClaimDeclContext | InferenceDeclContext;
        const name = simpleName(named.NAME().symbol, 'A declaration name');
        const nameLoc = locOf(named.NAME().symbol);
        if (!/^[a-z0-9]/.test(name))
            throw new UnilogError(`Declaration names must start with a lowercase letter or digit: '${name}'`, nameLoc);
        if (ctx instanceof InferenceDeclContext)
            return { k: 'inference', name, proof: proof(ctx.proof()), loc: locOf(ctx), nameLoc };
        const claim = ctx as ClaimDeclContext;
        const p = prop(claim.prop());
        const pr = claim.proof();
        return pr
            ? { k: 'inference', name, prop: p, proof: proof(pr), loc: locOf(ctx), nameLoc }
            : { k: 'axiom', name, prop: p, loc: locOf(ctx), nameLoc };
    }

    return { prop, file: (tree: FileContext) => tree.decl().map(decl) };
}
