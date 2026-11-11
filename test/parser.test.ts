import { describe, expect, it } from 'vitest';
import { parse, UnilogError } from '../src';

describe('lexing (via parse)', () => {
    it('joins dotted names into qualified names, and ends declarations with semicolons', () => {
        const decls = parse('a : b => C; # comment\nx = Mod.Sub.y z;', 'f');
        expect(decls).toHaveLength(2);
        expect(decls[1]).toMatchObject({
            proof: { k: 'app', fn: { k: 'ref', path: ['Mod', 'Sub', 'y'] }, arg: { path: ['z'] } },
        });
    });

    it('skips comments, including at the end of the file', () => {
        expect(parse('# only a comment', 'f')).toEqual([]);
        expect(parse('a : p; # trailing', 'f')).toHaveLength(1);
    });

    it('records line and column', () => {
        const [, d] = parse('a : p;\n  bb : q;', 'f');
        expect(d?.loc).toEqual({ file: 'f', line: 2, col: 3, end: { line: 2, col: 10 } });
        expect(d?.k !== 'import' && d?.nameLoc).toEqual({ file: 'f', line: 2, col: 3, end: { line: 2, col: 5 } });
    });

    it('records ranges for proof steps, spanning applications', () => {
        const [d] = parse('c = b (f a) x;', 'f');
        const proof = d?.k === 'inference' ? d.proof : undefined;
        // ((b (f a)) x): the whole application runs from b to x
        expect(proof?.loc).toEqual({ file: 'f', line: 1, col: 5, end: { line: 1, col: 14 } });
        expect(proof?.k === 'app' && proof.arg.loc).toEqual({ file: 'f', line: 1, col: 13, end: { line: 1, col: 14 } });
        // A parenthesized step's range includes its parentheses: (f a) runs from column 7 to 11.
        const inner = proof?.k === 'app' && proof.fn.k === 'app' ? proof.fn.arg : undefined;
        expect(inner?.loc).toEqual({ file: 'f', line: 1, col: 7, end: { line: 1, col: 12 } });
    });

    it('rejects unknown characters with a location', () => {
        expect(() => parse('a : b & c;', 'f.ul')).toThrow(/f\.ul:1:7: Unexpected character '&'/);
    });
});

describe('parse', () => {
    it('parses axioms, inferences and imports', () => {
        const decls = parse('import Kant.Ethics;\na : likes jake cake;\nc = b a;\nd : p = c;', 'f');
        expect(decls.map((d) => d.k)).toEqual(['import', 'axiom', 'inference', 'inference']);
        expect(decls[0]).toMatchObject({ k: 'import', path: ['Kant', 'Ethics'] });
        expect(decls[3]).toMatchObject({ k: 'inference', name: 'd', prop: { k: 'atom', name: 'p' } });
    });

    it('makes => right-associative and application left-associative', () => {
        const [d] = parse('r : a b c => d => e;', 'f');
        expect(d).toMatchObject({
            prop: {
                k: 'imp',
                from: { k: 'app', fn: { k: 'app', fn: { name: 'a' }, arg: { name: 'b' } }, arg: { name: 'c' } },
                to: { k: 'imp', from: { name: 'd' }, to: { name: 'e' } },
            },
        });
    });

    it('treats uppercase and _ names as variables in propositions', () => {
        const [d] = parse('r : X _y z;', 'f');
        expect(d).toMatchObject({
            prop: {
                k: 'app',
                fn: { k: 'app', fn: { k: 'var', name: 'X' }, arg: { k: 'var', name: '_y' } },
                arg: { k: 'atom' },
            },
        });
    });

    it('parses assumptions in proofs', () => {
        const [d] = parse('t = \\F X. F (g X);', 'f');
        expect(d).toMatchObject({
            proof: { k: 'lam', param: 'F', body: { k: 'lam', param: 'X', body: { k: 'app' } } },
        });
    });

    it("ends an assumption's head at its '.', even with no space before the body", () => {
        // \X.Kant.duty X: the assumption X, then the qualified name Kant.duty (not the name X.Kant.duty).
        const [d] = parse('t = \\X.Kant.duty X;', 'f');
        expect(d).toMatchObject({
            proof: { k: 'lam', param: 'X', body: { k: 'app', fn: { k: 'ref', path: ['Kant', 'duty'] } } },
        });
    });

    it('records where each assumption is named, also across lines', () => {
        const [d] = parse('t = \\F\n    G. F G;', 'f');
        const proof = d?.k === 'inference' ? d.proof : undefined;
        expect(proof?.k === 'lam' && proof.paramLoc).toEqual({ file: 'f', line: 1, col: 6, end: { line: 1, col: 7 } });
        const inner = proof?.k === 'lam' ? proof.body : undefined;
        expect(inner?.k === 'lam' && inner.paramLoc).toEqual({ file: 'f', line: 2, col: 5, end: { line: 2, col: 6 } });
    });

    it.each([
        ['missing terminator', 'a : b', /1:6: Syntax error: .*end of file.*expecting \{';', '='\}/],
        ['neither : nor =', 'a;', /1:2: Syntax error/],
        ['uppercase declaration name', 'Foo : b;', /must start with a lowercase/],
        [
            'qualified symbol in a proposition',
            'a : b.c;',
            /A symbol in a proposition cannot be a qualified name: 'b\.c'/,
        ],
        [
            'lowercase assumption',
            't = \\x. x;',
            /Assumptions are variables, so their names start with an uppercase letter/,
        ],
        ['an old-style terminator', 'a : likes jake cake.', /1:20: Declarations end with ';'/],
        ['an old-style terminator after an import', 'import Kant.Ethics.', /1:19: Declarations end with ';'/],
        [
            'the -> assumption syntax',
            't = \\F X -> F X;',
            /1:5: Assumptions are written '\\F X\. \.\.\.' rather than '\\F X -> \.\.\.'/,
        ],
        ["an assumption without its '.'", 't = \\P;', /An assumption needs a '\.' after its name: '\\P\. \.\.\.'/],
        [
            'the => assumption syntax',
            't = X => X;',
            /Assumptions are written with a backslash: '\\X\. \.\.\.' rather than 'X => \.\.\.'/,
        ],
        ['unbalanced parens', 'a : (b c;', /1:9: Syntax error: missing '\)'/],
        ['empty proposition', 'a : ;', /1:5: Syntax error: .*expecting \{'\(', a name\}/],
        ['import without a module', 'import ;', /1:8: Syntax error: missing a name/],
    ])('rejects %s', (_, src, msg) => {
        expect(() => parse(src, 'f')).toThrow(UnilogError);
        expect(() => parse(src, 'f')).toThrow(msg);
    });
});
