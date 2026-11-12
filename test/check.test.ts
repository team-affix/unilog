import { describe, expect, it } from 'vitest';
import { UnilogError } from '../src';
import { checkSource, proves, provesIn } from './helpers';

describe('the examples from the design discussion', () => {
    it('pattern matching: b a proves likes jake sugar', () => {
        const src = `
            a : likes jake cake;
            b : likes X cake => likes X sugar;
            c = b a;
        `;
        expect(provesIn(src, 'c')).toBe('likes jake sugar');
    });

    it('raw abstraction: a variable pattern accepts any proof', () => {
        const src = `
            everything_prover : X => theorem X;
            a : likes jake sugar;
            b = everything_prover a;
        `;
        expect(provesIn(src, 'b')).toBe('theorem (likes jake sugar)');
    });

    it('relations instead of functions: lt as rules', () => {
        const src = `
            znat : nat z;
            snat : nat X => nat (s X);
            ltz : nat Y => lt z (s Y);
            lts : lt X Y => lt (s X) (s Y);
            one_lt_two : lt (s z) (s (s z)) = lts (ltz znat);
        `;
        expect(provesIn(src, 'one_lt_two')).toBe('lt (s z) (s (s z))');
    });
});

describe('axioms and inferences', () => {
    it('quantifies an axiom over its variables, freshly at each use', () => {
        const m = checkSource(`
            likes_self : likes X X;
            a = likes_self;
            pair : P => Q => both P Q;
            b = pair likes_self likes_self;
        `);
        expect(proves(m, 'b')).toBe('both (likes X X) (likes X1 X1)');
    });

    it('infers general inferences from assumptions', () => {
        const m = checkSource(`
            id = \\P. P;
            compose = \\F G X. G (F X);
            const = \\X Y. X;
        `);
        expect(proves(m, 'id')).toBe('A => A');
        expect(proves(m, 'compose')).toBe('(A => B) => (B => C) => A => C');
        expect(proves(m, 'const')).toBe('A => B => A');
    });

    it('applies assumptions that are themselves implications', () => {
        const src = `
            b : likes X cake => likes X sugar;
            apply_to_b = \\F. F b;
        `;
        expect(provesIn(src, 'apply_to_b')).toBe('((likes X cake => likes X sugar) => A) => A');
    });

    it('lets variables stand for predicates', () => {
        const src = `
            a : likes jake cake;
            intro : P X => some P;
            c = intro a;
        `;
        expect(provesIn(src, 'c')).toBe('some (likes jake)');
    });

    it('lets a variable occur only in the conclusion (ex falso)', () => {
        const src = `
            explode : false => Y;
            oops : false;
            anything = explode oops;
            want : likes jake broccoli = explode oops;
        `;
        expect(provesIn(src, 'anything')).toBe('A');
        expect(provesIn(src, 'want')).toBe('likes jake broccoli');
    });
});

describe('currying: applying one premise at a time', () => {
    const src = `
        gift : likes X Y => has Z Y => gives Z X Y;
        a : likes jake cake;
        h : has mom cake;
        half = gift a;
        half_stated : has Z cake => gives Z jake cake = gift a;
        full = half h;
        both = gift a h;
    `;

    it('partial application yields an implication over the remaining premises', () => {
        const m = checkSource(src);
        expect(proves(m, 'half')).toBe('has Z cake => gives Z jake cake');
        expect(proves(m, 'half_stated')).toBe('has Z cake => gives Z jake cake');
    });

    it('finishing later is the same as applying everything at once', () => {
        const m = checkSource(src);
        expect(proves(m, 'full')).toBe('gives mom jake cake');
        expect(proves(m, 'both')).toBe('gives mom jake cake');
    });

    it('a partially applied rule is reusable, with its remaining variables fresh at each use', () => {
        const m = checkSource(src + 'p2 : A => B => both A B;\nh2 : has dad cake;\ntwo = p2 (half h) (half h2);');
        expect(proves(m, 'two')).toBe('both (gives mom jake cake) (gives dad jake cake)');
    });
});

describe('stated claims', () => {
    const base = `
        a : likes jake cake;
        b : likes X cake => likes X sugar;
    `;

    it('accepts a proof of exactly the claim', () => {
        expect(provesIn(base + 'c : likes jake sugar = b a;', 'c')).toBe('likes jake sugar');
    });

    it('accepts a claim that is a special case of what the proof shows', () => {
        expect(provesIn(base + 'c : likes jake cake => likes jake sugar = b;', 'c')).toBe(
            'likes jake cake => likes jake sugar',
        );
    });

    it('rejects a claim the proof does not show', () => {
        expect(() => checkSource(base + 'c : likes jake pie = b a;')).toThrow(/does not prove the stated claim/);
    });

    it('rejects a claim more general than the proof (claim variables must stay general)', () => {
        const err = (() => {
            try {
                checkSource(base + 'c : likes X sugar = b a;');
            } catch (e) {
                return e as UnilogError;
            }
        })();
        expect(err?.message).toMatch(/does not prove the stated claim/);
        expect(err?.message).toMatch(/must stay general/);
    });

    it('accepts a general claim proved generally', () => {
        expect(provesIn('swap : X => Y => X = \\X Y. X;', 'swap')).toBe('X => Y => X');
        expect(() => checkSource('bad : X => Y => Y = \\X Y. X;')).toThrow(/does not prove/);
    });

    it('records the claim, not the more general proof', () => {
        expect(provesIn('id_a : a => a = \\P. P;', 'id_a')).toBe('a => a');
    });
});

describe('soundness', () => {
    it('cannot apply a rule to the wrong proof', () => {
        expect(() =>
            checkSource(`
                a : likes jake pie;
                b : likes X cake => likes X sugar;
                c = b a;
            `),
        ).toThrow(/Cannot apply b to a[\s\S]*cake does not match pie/);
    });

    it('cannot apply something that is not an implication', () => {
        expect(() => checkSource('a : p;\nb : q;\nc = a b;')).toThrow(/Cannot apply a to b/);
    });

    it('rejects self-application (occurs check)', () => {
        expect(() => checkSource('selfapp = \\X. X X;')).toThrow(/would have to contain itself/);
    });

    it('allows no recursion: a declaration cannot use itself', () => {
        expect(() => checkSource('loop = loop;')).toThrow(/'loop' cannot be used in its own proof \(no recursion\)/);
        expect(() => checkSource('loop : X = loop;')).toThrow(/cannot be used in its own proof/);
    });

    it('allows no forward references', () => {
        expect(() => checkSource('c = b a;\na : p;\nb : p => q;')).toThrow(
            /'b' is declared later in this module; declarations can only use earlier ones/,
        );
        expect(() => checkSource('c = nowhere;')).toThrow(
            /'nowhere' is not declared in this module or in any module it imports/,
        );
    });

    it('cannot prove an arbitrary proposition from no axioms', () => {
        // With no axioms, only implications built from assumptions are provable, never a bare X.
        expect(() => checkSource('cheat : X = \\P. P;')).toThrow(/does not prove/);
    });

    it('scopes assumptions to their body', () => {
        expect(() => checkSource('f = \\P. P;\ng = P;')).toThrow(
            /'P' is not an assumption in scope \(assumptions are introduced with '\\P\. \.\.\.'\)/,
        );
    });

    it('rejects the Y combinator (no fixed points, so nothing proves itself)', () => {
        expect(() => checkSource('y = \\F. (\\X. F (X X)) (\\X. F (X X));')).toThrow(/would have to contain itself/);
    });

    it('treats an assumption as one fixed proposition: it cannot stand for two different ones', () => {
        // If P could be both an `a` and a `b`, this would prove a => (anything b gives).
        const src = 'to_c : a => c;\nto_d : b => d;\npair : A => B => both A B;\nbad = \\P. pair (to_c P) (to_d P);';
        expect(() => checkSource(src)).toThrow(/Cannot apply to_d to P[\s\S]*b does not match a/);
    });

    it('cannot derive A => B for unrelated A and B from assumptions alone', () => {
        expect(() => checkSource('cheat : A => B = \\P. P;')).toThrow(/does not prove the stated claim/);
    });

    it('in a proof, an uppercase name is an assumption and a lowercase name a declaration', () => {
        expect(() => checkSource('a : p;\nf = A;')).toThrow(/'A' is not an assumption in scope/);
        expect(() => checkSource('f = \\P. p;')).toThrow(/'p' is not declared/);
        // A declaration and an assumption can share a spelling apart from case without interfering.
        expect(provesIn('p : likes jake cake;\nk = \\P. p;', 'k')).toBe('A => likes jake cake');
    });

    it('rejects duplicate declarations', () => {
        expect(() => checkSource('a : p;\na : q;')).toThrow(/already declared/);
    });
});

describe('logic built from axioms (no special support in the core)', () => {
    const prelude = `
        pair : A => B => and A B;
        fst : and A B => A;
        snd : and A B => B;
        left : A => or A B;
        right : B => or A B;
        case : or A B => (A => C) => (B => C) => C;
        explode : false => C;
    `;

    it('and is commutative', () => {
        expect(provesIn(prelude + 'and_comm = \\P. pair (snd P) (fst P);', 'and_comm')).toBe('and A B => and B A');
    });

    it('or is commutative', () => {
        expect(provesIn(prelude + 'or_comm = \\P. case P right left;', 'or_comm')).toBe('or B A => or A B');
    });

    it('not P is P => false: contradiction implies anything', () => {
        expect(provesIn(prelude + 'contra = \\P Np. explode (Np P);', 'contra')).toBe('A => (A => false) => B');
    });

    it('modus tollens', () => {
        const src = prelude + 'tollens : (P => Q) => (Q => false) => (P => false) = \\F Nq P. Nq (F P);';
        expect(provesIn(src, 'tollens')).toBe('(P => Q) => (Q => false) => P => false');
    });

    it('excluded middle is not provable, but can be stated as a belief', () => {
        expect(() => checkSource(prelude + 'lem : or P (P => false) = \\P. P;')).toThrow(/does not prove/);
        const m = checkSource(
            prelude +
                'lem : or P (P => false);\ndne : ((P => false) => false) => P = \\Nnp. case lem (\\P. P) (\\Np. explode (Nnp Np));',
        );
        expect(proves(m, 'dne')).toBe('((P => false) => false) => P');
        expect(m.decls.get('lem')?.kind).toBe('axiom');
    });
});

describe('dependencies between declarations', () => {
    it('records which declarations each proof uses', () => {
        const m = checkSource(`
            a : likes jake cake;
            b : likes X cake => likes X sugar;
            c = b a;
            d = \\P. c;
        `);
        expect(m.decls.get('c')?.uses).toEqual(['Test.b', 'Test.a']);
        expect(m.decls.get('d')?.uses).toEqual(['Test.c']);
        expect(m.decls.get('a')?.uses).toEqual([]);
    });
});
