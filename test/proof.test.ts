import { describe, expect, it } from 'vitest';
import { commands } from '../src';
import type { ProofNode } from '../src/commands';
import { tmpDir, write } from './helpers';

function project(files: Record<string, string>) {
    const dir = tmpDir();
    write(dir, files);
    return dir;
}

const T = {
    'M/T.ul': `
        b : likes X cake => likes X sugar;
        a : likes jake cake;
        pair : A => B => both A B;
        lemma = b a;
        twice = pair lemma lemma;
        lift = \\P. pair P lemma;
        use_lift = lift a;
    `,
};

describe('proof', () => {
    it('shows each step with what it proves there, down to the axioms', () => {
        expect(commands.proof('M.T.lemma', project(T)).out).toEqual([
            'M.T.lemma = b a : likes jake sugar',
            '├── b : likes jake cake => likes jake sugar   [axiom M.T.b]',
            '└── a : likes jake cake   [axiom M.T.a]',
        ]);
    });

    it('expands inferences the proof uses, each once', () => {
        expect(commands.proof('M.T.twice', project(T)).out).toEqual([
            'M.T.twice = pair lemma lemma : both (likes jake sugar) (likes jake sugar)',
            '├── pair lemma : likes jake sugar => both (likes jake sugar) (likes jake sugar)',
            '│   ├── pair : likes jake sugar => likes jake sugar => both (likes jake sugar) (likes jake sugar)   [axiom M.T.pair]',
            '│   └── lemma : likes jake sugar   [inference M.T.lemma = b a]',
            '│       ├── b : likes jake cake => likes jake sugar   [axiom M.T.b]',
            '│       └── a : likes jake cake   [axiom M.T.a]',
            '└── lemma : likes jake sugar   [inference M.T.lemma, shown above]',
        ]);
    });

    it("shows assumptions, and a used inference's own (general) proof", () => {
        const out = commands.proof('M.T.use_lift', project(T)).out;
        expect(out[1]).toBe(
            '├── lift : likes jake cake => both (likes jake cake) (likes jake sugar)   [inference M.T.lift = \\P. pair P lemma]',
        );
        expect(out).toContain('│       │   └── P : A   [assumption]');
    });

    it('--shallow does not expand used inferences', () => {
        expect(commands.proof('M.T.use_lift', project(T), { shallow: true }).out).toEqual([
            'M.T.use_lift = lift a : both (likes jake cake) (likes jake sugar)',
            '├── lift : likes jake cake => both (likes jake cake) (likes jake sugar)   [inference M.T.lift]',
            '└── a : likes jake cake   [axiom M.T.a]',
        ]);
    });

    it('an axiom has no proof', () => {
        expect(commands.proof('M.T.a', project(T)).out).toEqual(['M.T.a : likes jake cake   [axiom]']);
    });

    it('names variables consistently between the statement and the steps', () => {
        const out = commands.proof('M.T.lift', project(T)).out;
        expect(out[0]).toBe('M.T.lift = \\P. pair P lemma : A => both A (likes jake sugar)');
        expect(out[1]).toBe('└── pair P lemma : both A (likes jake sugar)');
    });

    it('follows proofs across modules', () => {
        const dir = project({
            'Kant/Ethics.ul': 'duty : good X => ought X;\nhonesty : good truthful;\ntell_truth = duty honesty;',
            'Mine/Main.ul': 'import Kant.Ethics;\nmine = tell_truth;',
        });
        expect(commands.proof('Mine.Main.mine', dir).out).toEqual([
            'Mine.Main.mine = tell_truth : ought truthful',
            '└── tell_truth : ought truthful   [inference Kant.Ethics.tell_truth = duty honesty]',
            '    ├── duty : good truthful => ought truthful   [axiom Kant.Ethics.duty]',
            '    └── honesty : good truthful   [axiom Kant.Ethics.honesty]',
        ]);
    });

    it('--json gives the tree as data', () => {
        const root = JSON.parse(commands.proof('M.T.lemma', project(T), { json: true }).out[0] as string) as ProofNode;
        expect(root).toEqual({
            expr: 'M.T.lemma',
            proposition: 'likes jake sugar',
            kind: 'inference',
            name: 'M.T.lemma',
            proof: 'b a',
            rule: 'apply',
            children: [
                {
                    expr: 'b',
                    proposition: 'likes jake cake => likes jake sugar',
                    kind: 'axiom',
                    name: 'M.T.b',
                    children: [],
                },
                { expr: 'a', proposition: 'likes jake cake', kind: 'axiom', name: 'M.T.a', children: [] },
            ],
        });
    });

    it('--flat lists every declaration the proof relies on, once each, in declaration order', () => {
        expect(commands.proof('M.T.twice', project(T), { flat: true }).out).toEqual([
            'M.T.b : likes X cake => likes X sugar   [axiom]',
            'M.T.a : likes jake cake   [axiom]',
            'M.T.pair : A => B => both A B   [axiom]',
            'M.T.lemma = b a : likes jake sugar',
            'M.T.twice = pair lemma lemma : both (likes jake sugar) (likes jake sugar)',
        ]);
    });

    it('--flat keeps declarations in source order even when axioms and inferences alternate; --axioms-first groups them', () => {
        const dir = project({
            'M/I.ul': `
                a : p;
                ab : p => q;
                b = ab a;
                bc : q => r;
                unused : s;
                c = bc b;
            `,
        });
        expect(commands.proof('M.I.c', dir, { flat: true }).out).toEqual([
            'M.I.a : p   [axiom]',
            'M.I.ab : p => q   [axiom]',
            'M.I.b = ab a : q',
            'M.I.bc : q => r   [axiom]',
            'M.I.c = bc b : r',
        ]);
        expect(commands.proof('M.I.c', dir, { flat: true, axiomsFirst: true }).out).toEqual([
            'M.I.a : p   [axiom]',
            'M.I.ab : p => q   [axiom]',
            'M.I.bc : q => r   [axiom]',
            'M.I.b = ab a : q',
            'M.I.c = bc b : r',
        ]);
    });

    it('--flat follows inferences through their own proofs, across modules', () => {
        const dir = project({
            'Kant/Ethics.ul': 'duty : good X => ought X;\nhonesty : good truthful;\ntell_truth = duty honesty;',
            'Mine/Main.ul': 'import Kant.Ethics;\nmine = tell_truth;\nboth_ways = \\P. duty P;',
        });
        expect(commands.proof('Mine.Main.mine', dir, { flat: true }).out).toEqual([
            'Kant.Ethics.duty : good X => ought X   [axiom]',
            'Kant.Ethics.honesty : good truthful   [axiom]',
            'Kant.Ethics.tell_truth = duty honesty : ought truthful',
            'Mine.Main.mine = tell_truth : ought truthful',
        ]);
        expect(commands.proof('Mine.Main.both_ways', dir, { flat: true }).out).toEqual([
            'Kant.Ethics.duty : good X => ought X   [axiom]',
            'Mine.Main.both_ways = \\P. duty P : good X => ought X',
        ]);
    });

    it('--flat --shallow lists only what the proof uses directly', () => {
        expect(commands.proof('M.T.use_lift', project(T), { flat: true, shallow: true }).out).toEqual([
            'M.T.a : likes jake cake   [axiom]',
            'M.T.lift : A => both A (likes jake sugar)   [inference]',
            'M.T.use_lift = lift a : both (likes jake cake) (likes jake sugar)',
        ]);
    });

    it('--flat of an axiom is just the axiom', () => {
        expect(commands.proof('M.T.a', project(T), { flat: true }).out).toEqual(['M.T.a : likes jake cake   [axiom]']);
    });

    it('--flat --json gives the list as data', () => {
        expect(
            JSON.parse(commands.proof('M.T.lemma', project(T), { flat: true, json: true }).out[0] as string),
        ).toEqual([
            { name: 'M.T.b', kind: 'axiom', proposition: 'likes X cake => likes X sugar' },
            { name: 'M.T.a', kind: 'axiom', proposition: 'likes jake cake' },
            { name: 'M.T.lemma', kind: 'inference', proposition: 'likes jake sugar', proof: 'b a' },
        ]);
    });

    it('reports an unknown declaration', () => {
        expect(commands.proof('M.T.nope', project(T))).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/no declaration 'nope'/)],
        });
    });
});
