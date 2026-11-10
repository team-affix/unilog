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
        b : likes X cake => likes X sugar.
        a : likes jake cake.
        pair : A => B => both A B.
        lemma = b a.
        twice = pair lemma lemma.
        lift = p => pair p lemma.
        use_lift = lift a.
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

    it('expands theorems the proof uses, each once', () => {
        expect(commands.proof('M.T.twice', project(T)).out).toEqual([
            'M.T.twice = pair lemma lemma : both (likes jake sugar) (likes jake sugar)',
            '├── pair lemma : likes jake sugar => both (likes jake sugar) (likes jake sugar)',
            '│   ├── pair : likes jake sugar => likes jake sugar => both (likes jake sugar) (likes jake sugar)   [axiom M.T.pair]',
            '│   └── lemma : likes jake sugar   [theorem M.T.lemma = b a]',
            '│       ├── b : likes jake cake => likes jake sugar   [axiom M.T.b]',
            '│       └── a : likes jake cake   [axiom M.T.a]',
            '└── lemma : likes jake sugar   [theorem M.T.lemma, shown above]',
        ]);
    });

    it("shows assumptions, and a used theorem's own (general) proof", () => {
        const out = commands.proof('M.T.use_lift', project(T)).out;
        expect(out[1]).toBe(
            '├── lift : likes jake cake => both (likes jake cake) (likes jake sugar)   [theorem M.T.lift = p => pair p lemma]',
        );
        expect(out).toContain('│       │   └── p : A   [assumption]');
    });

    it('--shallow does not expand used theorems', () => {
        expect(commands.proof('M.T.use_lift', project(T), { shallow: true }).out).toEqual([
            'M.T.use_lift = lift a : both (likes jake cake) (likes jake sugar)',
            '├── lift : likes jake cake => both (likes jake cake) (likes jake sugar)   [theorem M.T.lift]',
            '└── a : likes jake cake   [axiom M.T.a]',
        ]);
    });

    it('an axiom has no proof', () => {
        expect(commands.proof('M.T.a', project(T)).out).toEqual(['M.T.a : likes jake cake   [axiom]']);
    });

    it('names variables consistently between the statement and the steps', () => {
        const out = commands.proof('M.T.lift', project(T)).out;
        expect(out[0]).toBe('M.T.lift = p => pair p lemma : A => both A (likes jake sugar)');
        expect(out[1]).toBe('└── pair p lemma : both A (likes jake sugar)');
    });

    it('follows proofs across modules', () => {
        const dir = project({
            'Kant/Ethics.ul': 'duty : good X => ought X.\nhonesty : good truthful.\ntell_truth = duty honesty.',
            'Mine/Main.ul': 'import Kant.Ethics.\nmine = tell_truth.',
        });
        expect(commands.proof('Mine.Main.mine', dir).out).toEqual([
            'Mine.Main.mine = tell_truth : ought truthful',
            '└── tell_truth : ought truthful   [theorem Kant.Ethics.tell_truth = duty honesty]',
            '    ├── duty : good truthful => ought truthful   [axiom Kant.Ethics.duty]',
            '    └── honesty : good truthful   [axiom Kant.Ethics.honesty]',
        ]);
    });

    it('--json gives the tree as data', () => {
        const root = JSON.parse(commands.proof('M.T.lemma', project(T), { json: true }).out[0] as string) as ProofNode;
        expect(root).toEqual({
            expr: 'M.T.lemma',
            proposition: 'likes jake sugar',
            kind: 'theorem',
            name: 'M.T.lemma',
            proof: 'b a',
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

    it('reports an unknown declaration', () => {
        expect(commands.proof('M.T.nope', project(T))).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/no declaration 'nope'/)],
        });
    });
});
