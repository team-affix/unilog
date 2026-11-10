import { describe, expect, it } from 'vitest';
import { commands } from '../src';
import { tmpDir, write } from './helpers';

/**
 * Variables must never be confused across scopes: between a query and a declaration, between two
 * declarations, or between the premises of a curried implication. Every variable in a declaration is
 * quantified once around the whole declaration, including ones that first appear in a later premise.
 */
const DECLS = `
    same : A => A.
    any : A => B.
    later : p X => q Y => r X Y.
    fixed : p jake => q Y => r jake Y.
    twin : p X => q X => r X X.
    b : likes X cake => likes X sugar.
    bj : likes jake cake => likes jake sugar.
    nested : (A => A) => done.
    nested_c : (c => c) => done.
`;

function names(query: string, mode: 'default' | 'exact' | 'pattern'): string[] {
    const dir = tmpDir();
    write(dir, { 'M/D.ul': DECLS });
    const r = commands.find(query, dir, { exact: mode === 'exact', pattern: mode === 'pattern', json: true });
    return (JSON.parse(r.out[0] as string) as { found: { name: string }[] }).found.map((f) => f.name.slice(4)).sort();
}

describe('find never confuses bound variables', () => {
    it.each([
        // query,                         default (proves it),            exact (same generality),  pattern (same shape)
        ['X => Y', ['any'], ['any'], ['any', 'b', 'bj', 'fixed', 'later', 'nested', 'nested_c', 'same', 'twin']], // pattern: any implication at all
        ['X => X', ['any', 'same'], ['same'], ['any', 'same']],
        ['c => c', ['any', 'same'], [], ['any', 'same']],
        // A variable first introduced at the second premise (Y) is still bound by the whole declaration.
        ['p X => q Y => r X Y', ['any', 'later'], ['later'], ['any', 'fixed', 'later', 'twin']],
        ['p jake => q Y => r jake Y', ['any', 'fixed', 'later'], ['fixed'], ['any', 'fixed', 'later', 'twin']],
        ['p X => q X => r X X', ['any', 'later', 'twin'], ['twin'], ['any', 'fixed', 'later', 'twin']],
        // Swapping the variables in the conclusion is a different claim.
        ['p X => q Y => r Y X', ['any'], [], ['any', 'fixed', 'later', 'twin']],
        // The query's X and Y are distinct even though b uses X in both places.
        ['likes Y cake => likes X sugar', ['any'], [], ['any', 'b', 'bj']],
        ['likes Z cake => likes Z sugar', ['any', 'b'], ['b'], ['any', 'b', 'bj']],
        ['likes jake cake => likes jake sugar', ['any', 'b', 'bj'], ['bj'], ['any', 'b', 'bj']],
        // A premise that is itself an implication: its variables are bound by the outer declaration too.
        ['(c => c) => done', ['any', 'nested', 'nested_c'], ['nested_c'], ['any', 'nested', 'nested_c']],
        ['(A => A) => done', ['any', 'nested'], ['nested'], ['any', 'nested', 'nested_c']],
    ])('%s', (query, byDefault, exact, pattern) => {
        expect(names(query, 'default')).toEqual(byDefault);
        expect(names(query, 'exact')).toEqual(exact);
        expect(names(query, 'pattern')).toEqual(pattern);
    });

    it('rejects --pattern together with --exact', () => {
        expect(commands.find('X', tmpDir(), { pattern: true, exact: true })).toMatchObject({ ok: false });
    });
});

describe('checking never confuses bound variables either', () => {
    it('instantiates a declaration freshly at each use, including within one proof', () => {
        const dir = tmpDir();
        write(dir, {
            'M/D.ul': `
                later : p X => q Y => r X Y.
                pa : p a.
                pb : p b.
                qc : q c.
                pair : A => B => both A B.
                two = pair (later pa qc) (later pb qc).
            `,
        });
        expect(commands.show('M.D.two', dir).out).toEqual(['theorem M.D.two : both (r a c) (r b c)']);
    });

    it("keeps a stated claim's variables distinct from the proof's", () => {
        const dir = tmpDir();
        write(dir, { 'M/D.ul': 'later : p X => q Y => r X Y.\nbad : p X => q Y => r Y X = later.' });
        expect(commands.check(dir)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/does not prove the stated claim/)],
        });
    });
});
