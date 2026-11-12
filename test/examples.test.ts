import path from 'path';
import { describe, expect, it } from 'vitest';
import { commands } from '../src';

const examples = path.join(__dirname, '..', 'examples');

describe('examples/', () => {
    it('all check', () => {
        const r = commands.check(examples);
        expect(r.err).toEqual([]);
        expect(r.ok).toBe(true);
    });

    it('classical results list excluded middle among their axioms', () => {
        expect(commands.restsOn('Logic.Classical.double_negation', examples).out).toEqual([
            'Logic.Classical.lem : \\P. or P (P => false)',
            'Logic.Prelude.case : \\A B C. or A B => (A => C) => (B => C) => C',
            'Logic.Prelude.explode : \\C. false => C',
        ]);
    });

    it('constructive results do not', () => {
        expect(commands.restsOn('Logic.Prelude.tollens', examples).out).toEqual([
            'Logic.Prelude.tollens rests on no axioms',
        ]);
    });
});
