import { describe, expect, it } from 'vitest';
import { parse, tokenize, UnilogError } from '../src';

describe('tokenize', () => {
    it('splits names, symbols and qualified names, skipping comments', () => {
        const toks = tokenize('a : b => C. # comment\nx = Mod.Sub.y z.', 'f');
        expect(toks.map((t) => (t.t === 'name' ? t.text : t.t))).toEqual([
            'a',
            'colon',
            'b',
            'arrow',
            'C',
            'dot',
            'x',
            'equals',
            'Mod.Sub.y',
            'z',
            'dot',
            'eof',
        ]);
    });

    it('tracks line and column', () => {
        const toks = tokenize('a\n  bb', 'f');
        expect(toks[1]?.loc).toEqual({ file: 'f', line: 2, col: 3 });
    });

    it('rejects unknown characters with a location', () => {
        expect(() => tokenize('a : b & c.', 'f.ul')).toThrow(/f\.ul:1:7: Unexpected character '&'/);
    });
});

describe('parse', () => {
    it('parses axioms, theorems and imports', () => {
        const decls = parse('import Kant.Ethics.\na : likes jake cake.\nc = b a.\nd : p = c.', 'f');
        expect(decls.map((d) => d.k)).toEqual(['import', 'axiom', 'theorem', 'theorem']);
        expect(decls[0]).toMatchObject({ k: 'import', path: ['Kant', 'Ethics'] });
        expect(decls[3]).toMatchObject({ k: 'theorem', name: 'd', prop: { k: 'atom', name: 'p' } });
    });

    it('makes => right-associative and application left-associative', () => {
        const [d] = parse('r : a b c => d => e.', 'f');
        expect(d).toMatchObject({
            prop: {
                k: 'imp',
                from: { k: 'app', fn: { k: 'app', fn: { name: 'a' }, arg: { name: 'b' } }, arg: { name: 'c' } },
                to: { k: 'imp', from: { name: 'd' }, to: { name: 'e' } },
            },
        });
    });

    it('treats uppercase and _ names as variables in propositions', () => {
        const [d] = parse('r : X _y z.', 'f');
        expect(d).toMatchObject({
            prop: {
                k: 'app',
                fn: { k: 'app', fn: { k: 'var', name: 'X' }, arg: { k: 'var', name: '_y' } },
                arg: { k: 'atom' },
            },
        });
    });

    it('parses assumptions in proofs', () => {
        const [d] = parse('t = f => x => f (g x).', 'f');
        expect(d).toMatchObject({
            proof: { k: 'lam', param: 'f', body: { k: 'lam', param: 'x', body: { k: 'app' } } },
        });
    });

    it.each([
        ['missing terminator', 'a : b', /Expected '\.'/],
        ['neither : nor =', 'a.', /Expected ':' or '='/],
        ['uppercase declaration name', 'Foo : b.', /must start with a lowercase/],
        ['qualified symbol in a proposition', 'a : b.c.', /put a space or newline/],
        ['uppercase assumption', 't = X => X.', /Assumption names must start with a lowercase/],
        ['unbalanced parens', 'a : (b c.', /Expected '\)'/],
        ['empty proposition', 'a : .', /Expected a proposition/],
    ])('rejects %s', (_, src, msg) => {
        expect(() => parse(src, 'f')).toThrow(UnilogError);
        expect(() => parse(src, 'f')).toThrow(msg);
    });
});
