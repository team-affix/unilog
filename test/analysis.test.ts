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

describe('check --max-axioms', () => {
    it('passes when the checked code declares no axioms, even though its imports do', () => {
        const dir = debate(`
            import Alice.Beliefs.
            party_wrong = lying_wrong (surprise_lies party).
        `);
        expect(commands.check(dir, { maxAxioms: 0 })).toEqual({
            ok: true,
            out: ['ok: 1 module(s) checked, declaring 1 theorem(s) and 0 axiom(s); 1 imported module(s)'],
            err: [],
        });
        expect(commands.show('Bob.Rebuttal.party_wrong', dir).out).toEqual([
            'theorem Bob.Rebuttal.party_wrong : wrong bob_party',
        ]);
    });

    it('fails, listing each axiom with its location, when the checked code declares one', () => {
        const dir = debate(`
            import Alice.Beliefs.
            convenient : wrong alice.
        `);
        const r = commands.check(dir, { maxAxioms: 0 });
        expect(r.ok).toBe(false);
        expect(r.err[0]).toBe('The checked code declares 1 axiom(s), more than the allowed 0:');
        expect(r.err[1]).toMatch(/Bob\/Rebuttal\.ul:3:13: Bob\.Rebuttal\.convenient : wrong alice$/);
    });

    it('allows up to the given number', () => {
        const dir = debate('a : p.\nb : q.');
        expect(commands.check(dir, { maxAxioms: 2 }).ok).toBe(true);
        expect(commands.check(dir, { maxAxioms: 1 }).ok).toBe(false);
    });

    it('still reports ordinary check errors first', () => {
        const dir = debate('x = nope.');
        expect(commands.check(dir, { maxAxioms: 0 })).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/'nope' is not declared/)],
        });
    });
});

describe('axioms --json: closing the "second package" loophole', () => {
    it('lists each axiom a conclusion rests on with its module', () => {
        const dir = debate(`
            import Alice.Beliefs.
            party_wrong = lying_wrong (surprise_lies party).
        `);
        expect(JSON.parse(commands.axioms('Bob.Rebuttal.party_wrong', dir, true).out[0] as string)).toEqual({
            declaration: 'Bob.Rebuttal.party_wrong',
            axioms: [
                { name: 'Alice.Beliefs.lying_wrong', module: 'Alice.Beliefs', proposition: 'lies X => wrong X' },
                { name: 'Alice.Beliefs.party', module: 'Alice.Beliefs', proposition: 'surprises bob_party' },
                { name: 'Alice.Beliefs.surprise_lies', module: 'Alice.Beliefs', proposition: 'surprises X => lies X' },
            ],
        });
    });

    it('exposes axioms smuggled in through another dependency, which --max-axioms 0 alone would miss', () => {
        const dir = debate(
            `
            import Alice.Beliefs.
            import Bob.Helpers.
            alice_wrong = lying_wrong alice_lies.
        `,
            { 'Bob/Helpers.ul': 'alice_lies : lies alice.' },
        );
        // Bob's checked module declares no axioms...
        // (Bob/Helpers.ul sits in deps/, so it is imported, not checked.)
        expect(commands.check(dir, { maxAxioms: 0 }).ok).toBe(true);
        // ...but his conclusion rests on one that isn't Alice's, and a verifier can see it.
        const { axioms } = JSON.parse(commands.axioms('Bob.Rebuttal.alice_wrong', dir, true).out[0] as string) as {
            axioms: { module: string }[];
        };
        expect(axioms.filter((a) => !a.module.startsWith('Alice.')).map((a) => a.module)).toEqual(['Bob.Helpers']);
    });
});

describe('summary', () => {
    const dir = () =>
        debate(`
            import Alice.Beliefs.
            party_wrong = lying_wrong (surprise_lies party).
            via = p => lying_wrong p.
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
