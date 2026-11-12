import { describe, expect, it } from 'vitest';
import { commands } from '../src';
import { tmpDir, write } from './helpers';

/**
 * Variables must never be confused across scopes: between a query and a declaration, between two
 * declarations, or between the premises of a curried implication. A variable bound at the front of a
 * declaration is bound for the whole of it, including premises after the one where it first appears.
 */
const DECLS = `
    same : \\A. A => A;
    any : \\A B. A => B;
    later : \\X Y. p X => q Y => r X Y;
    fixed : \\Y. p jake => q Y => r jake Y;
    twin : \\X. p X => q X => r X X;
    b : \\X. likes X cake => likes X sugar;
    bj : likes jake cake => likes jake sugar;
    nested : \\A. (A => A) => done;
    nested_c : (c => c) => done;
`;

function names(query: string, mode: 'default' | 'exact'): string[] {
    const dir = tmpDir();
    write(dir, { 'M/D.ul': DECLS });
    const r = commands.find(query, dir, { exact: mode === 'exact', json: true });
    if (!r.out[0]) return [];
    return (JSON.parse(r.out[0]) as { found: { name: string }[] }).found.map((f) => f.name.slice(4)).sort();
}

describe('find never confuses bound variables', () => {
    it.each([
        // query,                                   default (proves it),          exact (same generality)
        ['\\X Y. X => Y', ['any'], ['any']],
        ['\\X. X => X', ['any', 'same'], ['same']],
        ['c => c', ['any', 'same'], []],
        // A variable first used in the second premise (Y) is still bound by the whole declaration.
        ['\\X Y. p X => q Y => r X Y', ['any', 'later'], ['later']],
        ['\\Y. p jake => q Y => r jake Y', ['any', 'fixed', 'later'], ['fixed']],
        ['\\X. p X => q X => r X X', ['any', 'later', 'twin'], ['twin']],
        // Swapping the variables in the conclusion is a different claim.
        ['\\X Y. p X => q Y => r Y X', ['any'], []],
        // The query's X and Y are distinct even though b uses X in both places.
        ['\\X Y. likes Y cake => likes X sugar', ['any'], []],
        ['\\Z. likes Z cake => likes Z sugar', ['any', 'b'], ['b']],
        ['likes jake cake => likes jake sugar', ['any', 'b', 'bj'], ['bj']],
        // A premise that is itself an implication: its variables are bound by the outer declaration too.
        ['(c => c) => done', ['any', 'nested', 'nested_c'], ['nested_c']],
        ['\\A. (A => A) => done', ['any', 'nested'], ['nested']],
    ])('%s', (query, byDefault, exact) => {
        expect(names(query, 'default')).toEqual(byDefault);
        expect(names(query, 'exact')).toEqual(exact);
    });

    it('requires every variable in a query to be bound', () => {
        expect(commands.find('wrong X', tmpDir())).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/'X' is not bound/)],
        });
    });
});

describe('checking never confuses bound variables either', () => {
    it('instantiates a declaration freshly at each use, including within one proof', () => {
        const dir = tmpDir();
        write(dir, {
            'M/D.ul': `
                later : \\X Y. p X => q Y => r X Y;
                pa : p a;
                pb : p b;
                qc : q c;
                pair : \\A B. A => B => both A B;
                two = pair (later pa qc) (later pb qc);
            `,
        });
        expect(commands.show('M.D.two', dir).out).toEqual(['inference M.D.two : both (r a c) (r b c)']);
    });

    it("keeps a stated claim's variables distinct from the proof's", () => {
        const dir = tmpDir();
        write(dir, { 'M/D.ul': 'later : \\X Y. p X => q Y => r X Y;\nbad : \\X Y. p X => q Y => r Y X = later;' });
        expect(commands.check(dir)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/does not prove the stated claim/)],
        });
    });
});
