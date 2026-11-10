import { Loc, UnilogError } from './errors';
import { Decl, PropExpr, ProofExpr } from './syntax';

/**
 * Grammar:
 *
 *   file     := decl*
 *   decl     := 'import' NAME '.'
 *             | NAME ':' prop '.'                 axiom
 *             | NAME '=' proof '.'                theorem, proposition inferred
 *             | NAME ':' prop '=' proof '.'       theorem, proposition stated and checked
 *   prop     := papp ('=>' prop)?                 '=>' is right-associative
 *   papp     := patom+
 *   patom    := NAME | '(' prop ')'               Uppercase or '_' start: variable. Otherwise: atom.
 *   proof    := NAME '=>' proof | rapp            assumption, or application
 *   rapp     := ratom+
 *   ratom    := NAME | '(' proof ')'              NAME may be qualified: Mod.Sub.name
 *
 * NAME is [A-Za-z0-9_'] characters, optionally joined by '.' with no spaces (a qualified name).
 * A '.' not immediately followed by a name character ends a declaration. '#' starts a comment.
 */

type Token =
    | { t: 'name'; text: string; loc: Loc }
    | { t: 'arrow' | 'colon' | 'equals' | 'lparen' | 'rparen' | 'dot' | 'eof'; loc: Loc };

const NAME_CHAR = /[A-Za-z0-9_']/;

export function tokenize(src: string, file: string): Token[] {
    const tokens: Token[] = [];
    let i = 0;
    let line = 1;
    let col = 1;
    const loc = (): Loc => ({ file, line, col });
    const advance = (n: number) => {
        for (let j = 0; j < n; j++) {
            if (src[i] === '\n') {
                line++;
                col = 1;
            } else col++;
            i++;
        }
    };

    while (i < src.length) {
        const c = src[i] as string;
        if (c === '#') {
            while (i < src.length && src[i] !== '\n') advance(1);
        } else if (/\s/.test(c)) {
            advance(1);
        } else if (src.startsWith('=>', i)) {
            tokens.push({ t: 'arrow', loc: loc() });
            advance(2);
        } else if (c === ':' || c === '=' || c === '(' || c === ')' || c === '.') {
            const t = ({ ':': 'colon', '=': 'equals', '(': 'lparen', ')': 'rparen', '.': 'dot' } as const)[c];
            tokens.push({ t, loc: loc() });
            advance(1);
        } else if (NAME_CHAR.test(c)) {
            const start = loc();
            let j = i;
            for (;;) {
                while (j < src.length && NAME_CHAR.test(src[j] as string)) j++;
                // A dot directly followed by a name character continues a qualified name.
                if (src[j] === '.' && j + 1 < src.length && NAME_CHAR.test(src[j + 1] as string)) j++;
                else break;
            }
            tokens.push({ t: 'name', text: src.slice(i, j), loc: start });
            advance(j - i);
        } else {
            throw new UnilogError(`Unexpected character '${c}'`, loc());
        }
    }
    tokens.push({ t: 'eof', loc: loc() });
    return tokens;
}

/** True for names that denote variables in propositions: starting with an uppercase letter or '_'. */
export function isVariableName(name: string): boolean {
    return /^[A-Z_]/.test(name);
}

const describe = (tok: Token) =>
    tok.t === 'name'
        ? `'${tok.text}'`
        : tok.t === 'eof'
          ? 'end of file'
          : `'${{ arrow: '=>', colon: ':', equals: '=', lparen: '(', rparen: ')', dot: '.' }[tok.t]}'`;

export function parse(src: string, file: string): Decl[] {
    const tokens = tokenize(src, file);
    let pos = 0;
    const peek = (o = 0) => tokens[Math.min(pos + o, tokens.length - 1)] as Token;
    const next = () => tokens[pos++] as Token;
    const expect = (t: Token['t'], what: string): Token => {
        const tok = next();
        if (tok.t !== t) throw new UnilogError(`Expected ${what}, found ${describe(tok)}`, tok.loc);
        return tok;
    };
    const startsAtom = (tok: Token) => tok.t === 'name' || tok.t === 'lparen';

    function simpleName(tok: Token & { t: 'name' }, role: string): string {
        if (tok.text.includes('.'))
            throw new UnilogError(
                `${role} cannot be a qualified name: '${tok.text}' (if the '.' ends a declaration, put a space or newline after it)`,
                tok.loc,
            );
        return tok.text;
    }

    function prop(): PropExpr {
        const left = propApp();
        if (peek().t === 'arrow') {
            next();
            return { k: 'imp', from: left, to: prop(), loc: left.loc };
        }
        return left;
    }
    function propApp(): PropExpr {
        let e = propAtom();
        while (startsAtom(peek())) e = { k: 'app', fn: e, arg: propAtom(), loc: e.loc };
        return e;
    }
    function propAtom(): PropExpr {
        const tok = next();
        if (tok.t === 'lparen') {
            const e = prop();
            expect('rparen', "')'");
            return e;
        }
        if (tok.t !== 'name') throw new UnilogError(`Expected a proposition, found ${describe(tok)}`, tok.loc);
        const name = simpleName(tok, 'A symbol in a proposition');
        return { k: isVariableName(name) ? 'var' : 'atom', name, loc: tok.loc };
    }

    function proof(): ProofExpr {
        const tok = peek();
        if (tok.t === 'name' && peek(1).t === 'arrow') {
            next();
            next();
            const param = simpleName(tok, 'An assumption');
            if (isVariableName(param))
                throw new UnilogError(`Assumption names must start with a lowercase letter: '${param}'`, tok.loc);
            return { k: 'lam', param, body: proof(), loc: tok.loc };
        }
        let e = proofAtom();
        while (startsAtom(peek())) e = { k: 'app', fn: e, arg: proofAtom(), loc: e.loc };
        return e;
    }
    function proofAtom(): ProofExpr {
        const tok = next();
        if (tok.t === 'lparen') {
            const e = proof();
            expect('rparen', "')'");
            return e;
        }
        if (tok.t !== 'name') throw new UnilogError(`Expected a proof, found ${describe(tok)}`, tok.loc);
        return { k: 'ref', path: tok.text.split('.'), loc: tok.loc };
    }

    const decls: Decl[] = [];
    while (peek().t !== 'eof') {
        const head = expect('name', 'a declaration') as Token & { t: 'name' };
        if (head.text === 'import') {
            const path = expect('name', 'a module name after import') as Token & { t: 'name' };
            expect('dot', "'.' after the import");
            decls.push({ k: 'import', path: path.text.split('.'), loc: head.loc });
            continue;
        }
        const name = simpleName(head, 'A declaration name');
        if (isVariableName(name) || !/^[a-z0-9]/.test(name))
            throw new UnilogError(`Declaration names must start with a lowercase letter or digit: '${name}'`, head.loc);
        let p: PropExpr | undefined;
        let pr: ProofExpr | undefined;
        if (peek().t === 'colon') {
            next();
            p = prop();
        }
        if (peek().t === 'equals') {
            next();
            pr = proof();
        }
        if (!p && !pr) throw new UnilogError(`Expected ':' or '=' after '${name}'`, peek().loc);
        expect('dot', "'.' at the end of the declaration");
        decls.push(
            pr
                ? { k: 'theorem', name, prop: p, proof: pr, loc: head.loc }
                : { k: 'axiom', name, prop: p as PropExpr, loc: head.loc },
        );
    }
    return decls;
}
