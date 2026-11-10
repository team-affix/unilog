import path from 'path';
import { describe, expect, it } from 'vitest';
import { commands } from '../src';
import type { ModuleSummary } from '../src/commands';
import { tmpDir, write } from './helpers';

/**
 * A debate: Alice publishes her beliefs. Bob depends on them and proves something she wouldn't like,
 * using only her axioms.
 */
const ALICE = {
    'Alice/Beliefs.ul': `
        lying_wrong : lies X => wrong X.
        surprise_lies : surprises X => lies X.
        party : surprises bob_party.
    `,
};

function debate(bobsCode: string, extraDeps: Record<string, string> = {}) {
    const dir = tmpDir();
    write(dir, {
        'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['Bob'] }),
        'Bob/Rebuttal.ul': bobsCode,
    });
    write(path.join(dir, 'deps'), { ...ALICE, ...extraDeps });
    return dir;
}

describe('axioms: counting', () => {
    const dir = () =>
        debate(
            `
            import Alice.Beliefs.
            import Shared.Common.
            mine : believes bob things.
            party_wrong = lying_wrong (surprise_lies party).
        `,
            {
                // Alice's module also imports Shared.Common: a diamond, so Shared.Common must be counted once.
                'Alice/Beliefs.ul': 'import Shared.Common.\n' + (ALICE['Alice/Beliefs.ul'] as string),
                'Shared/Common.ul': 'common : everyone (agrees X).',
            },
        );

    it('counts only the axioms declared in the module itself', () => {
        expect(commands.axioms('Bob.Rebuttal', dir()).out).toEqual([
            'Bob.Rebuttal.mine : believes bob things',
            '1 axiom(s) in Bob.Rebuttal',
        ]);
        expect(commands.axioms('Alice.Beliefs', dir(), { count: true }).out).toEqual(['3']);
    });

    it('--transitive counts the module and everything it imports, each module once', () => {
        expect(commands.axioms('Bob.Rebuttal', dir(), { transitive: true, count: true }).out).toEqual(['5']);
        expect(commands.axioms('Bob.Rebuttal', dir(), { transitive: true }).out.at(-1)).toBe(
            '5 axiom(s) in Bob.Rebuttal and the 2 module(s) it imports',
        );
    });

    it('--json gives each axiom with its module and location', () => {
        const r = JSON.parse(commands.axioms('Bob.Rebuttal', dir(), { transitive: true, json: true }).out[0] as string);
        expect(r).toMatchObject({
            module: 'Bob.Rebuttal',
            transitive: true,
            count: 5,
            modules: [
                { name: 'Bob.Rebuttal', axioms: 1 },
                { name: 'Alice.Beliefs', axioms: 3 },
                { name: 'Shared.Common', axioms: 1 },
            ],
        });
        expect(r.axioms[0]).toMatchObject({
            name: 'Bob.Rebuttal.mine',
            module: 'Bob.Rebuttal',
            proposition: 'believes bob things',
            line: 4,
        });
    });

    it('prints propositions as written, including variables and nested implications', () => {
        const d = debate('r : (a X => b) => f (g X) => c.');
        expect(commands.axioms('Bob.Rebuttal', d).out[0]).toBe('Bob.Rebuttal.r : (a X => b) => f (g X) => c');
    });

    it('only parses: it counts axioms in code that would fail to check', () => {
        const d = debate('a : p.\nbroken = nope.\nb : q.');
        expect(commands.check(d).ok).toBe(false);
        expect(commands.axioms('Bob.Rebuttal', d, { count: true })).toEqual({ ok: true, out: ['2'], err: [] });
    });

    it('reports a missing module, and a missing import when transitive', () => {
        expect(commands.axioms('Nope.Gone', dir())).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/Module Nope\.Gone not found/)],
        });
        const d = debate('import Missing.Thing.\na : p.');
        expect(commands.axioms('Bob.Rebuttal', d, { count: true }).out).toEqual(['1']);
        expect(commands.axioms('Bob.Rebuttal', d, { transitive: true })).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/Rebuttal\.ul:1:1: Module Missing\.Thing not found/)],
        });
    });

    it('is independent of check: check reports validity only', () => {
        const d = debate('import Alice.Beliefs.\nconvenient : wrong alice.');
        expect(commands.check(d)).toMatchObject({ ok: true });
    });
});

describe('rests-on: what a conclusion depends on', () => {
    it('lists each axiom a conclusion rests on, with its module', () => {
        const dir = debate(`
            import Alice.Beliefs.
            party_wrong = lying_wrong (surprise_lies party).
        `);
        expect(JSON.parse(commands.restsOn('Bob.Rebuttal.party_wrong', dir, true).out[0] as string)).toEqual({
            declaration: 'Bob.Rebuttal.party_wrong',
            axioms: [
                { name: 'Alice.Beliefs.lying_wrong', module: 'Alice.Beliefs', proposition: 'lies X => wrong X' },
                { name: 'Alice.Beliefs.party', module: 'Alice.Beliefs', proposition: 'surprises bob_party' },
                { name: 'Alice.Beliefs.surprise_lies', module: 'Alice.Beliefs', proposition: 'surprises X => lies X' },
            ],
        });
    });

    it('sees axioms coming from any imported module, not just the checked one', () => {
        const dir = debate(
            `
            import Alice.Beliefs.
            import Bob.Helpers.
            alice_wrong = lying_wrong alice_lies.
        `,
            { 'Bob/Helpers.ul': 'alice_lies : lies alice.' },
        );
        expect(commands.axioms('Bob.Rebuttal', dir, { count: true }).out).toEqual(['0']);
        const { axioms } = JSON.parse(commands.restsOn('Bob.Rebuttal.alice_wrong', dir, true).out[0] as string) as {
            axioms: { module: string }[];
        };
        expect(axioms.map((a) => a.module)).toEqual(['Alice.Beliefs', 'Bob.Helpers']);
    });
});

describe('summary', () => {
    const dir = () =>
        debate(`
            import Alice.Beliefs.
            party_wrong = lying_wrong (surprise_lies party).
            via = P => lying_wrong P.
        `);

    it('tabulates axioms and theorems per module, checked and imported', () => {
        expect(commands.summary(dir()).out).toEqual([
            'module         source    axioms  theorems',
            'Alice.Beliefs  imported       3         0',
            'Bob.Rebuttal   checked        0         2',
            'axioms: 0 in checked code, 3 in imported modules',
        ]);
    });

    it('outputs the declaration graph as JSON, with what each declaration rests on', () => {
        const { modules } = JSON.parse(commands.summary(dir(), true).out[0] as string) as { modules: ModuleSummary[] };
        expect(modules.map((m) => [m.name, m.own, m.axioms, m.theorems])).toEqual([
            ['Alice.Beliefs', false, 3, 0],
            ['Bob.Rebuttal', true, 0, 2],
        ]);
        const bob = modules[1];
        expect(bob?.imports).toEqual(['Alice.Beliefs']);
        expect(bob?.declarations).toEqual([
            {
                name: 'Bob.Rebuttal.party_wrong',
                kind: 'theorem',
                proposition: 'wrong bob_party',
                uses: ['Alice.Beliefs.lying_wrong', 'Alice.Beliefs.surprise_lies', 'Alice.Beliefs.party'],
                restsOn: ['Alice.Beliefs.lying_wrong', 'Alice.Beliefs.party', 'Alice.Beliefs.surprise_lies'],
            },
            {
                name: 'Bob.Rebuttal.via',
                kind: 'theorem',
                proposition: 'lies X => wrong X',
                uses: ['Alice.Beliefs.lying_wrong'],
                restsOn: ['Alice.Beliefs.lying_wrong'],
            },
        ]);
    });

    it('reports check errors instead of a summary', () => {
        expect(commands.summary(debate('x = nope.'))).toMatchObject({ ok: false });
    });
});
