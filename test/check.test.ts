import { describe, expect, it } from 'vitest';
import { UnilogError } from '../src';
import { checkSource, proves, provesIn } from './helpers';

describe('the examples from the design discussion', () => {
    it('pattern matching: b a proves likes jake sugar', () => {
        const src = `
            a : likes jake cake;
            b : \\X. likes X cake => likes X sugar;
            c = b a;
        `;
        expect(provesIn(src, 'c')).toBe('likes jake sugar');
    });

    it('raw abstraction: a variable pattern accepts any proof', () => {
        const src = `
            everything_prover : \\X. X => theorem X;
            a : likes jake sugar;
            b = everything_prover a;
        `;
        expect(provesIn(src, 'b')).toBe('theorem (likes jake sugar)');
    });

    it('relations instead of functions: lt as rules', () => {
        const src = `
            znat : nat z;
            snat : \\X. nat X => nat (s X);
            ltz : \\Y. nat Y => lt z (s Y);
            lts : \\X Y. lt X Y => lt (s X) (s Y);
            one_lt_two : lt (s z) (s (s z)) = lts (ltz znat);
        `;
        expect(provesIn(src, 'one_lt_two')).toBe('lt (s z) (s (s z))');
    });
});

describe('axioms and inferences', () => {
    it('quantifies an axiom over its variables, freshly at each use', () => {
        const m = checkSource(`
            likes_self : \\X. likes X X;
            a = likes_self;
            pair : \\P Q. P => Q => both P Q;
            b = pair likes_self likes_self;
        `);
        expect(proves(m, 'b')).toBe('\\A B. both (likes A A) (likes B B)');
    });

    it('infers general inferences from assumptions', () => {
        const m = checkSource(`
            id = \\P. P;
            compose = \\F G X. G (F X);
            const = \\X Y. X;
        `);
        expect(proves(m, 'id')).toBe('\\A. A => A');
        expect(proves(m, 'compose')).toBe('\\A B C. (A => B) => (B => C) => A => C');
        expect(proves(m, 'const')).toBe('\\A B. A => B => A');
    });

    it('applies assumptions that are themselves implications', () => {
        const src = `
            b : \\X. likes X cake => likes X sugar;
            apply_to_b = \\F. F b;
        `;
        expect(provesIn(src, 'apply_to_b')).toBe('\\X A. ((likes X cake => likes X sugar) => A) => A');
    });

    it('lets variables stand for propositions with a gap, wherever the claim puts the gap', () => {
        const src = `
            a : likes jake cake;
            intro : \\P X. P X => some P;
            likes_cake : some (\\X. likes X cake) = intro a;
            jake_likes : some (likes jake) = intro a;
        `;
        expect(provesIn(src, 'likes_cake')).toBe('some \\X. likes X cake');
        expect(provesIn(src, 'jake_likes')).toBe('some (likes jake)');
    });

    it('refuses to guess a gap: without a claim, likes jake cake could have it at jake, at cake, or nowhere', () => {
        const src = `
            a : likes jake cake;
            intro : \\P X. P X => some P;
            c = intro a;
        `;
        expect(() => provesIn(src, 'c')).toThrow(/Cannot tell where the gap is in P[\s\S]*State the claim/);
    });

    it('lets a variable occur only in the conclusion (ex falso)', () => {
        const src = `
            explode : \\Y. false => Y;
            oops : false;
            anything = explode oops;
            want : likes jake broccoli = explode oops;
        `;
        expect(provesIn(src, 'anything')).toBe('\\Y. Y');
        expect(provesIn(src, 'want')).toBe('likes jake broccoli');
    });
});

describe('currying: applying one premise at a time', () => {
    const src = `
        gift : \\X Y Z. likes X Y => has Z Y => gives Z X Y;
        a : likes jake cake;
        h : has mom cake;
        half = gift a;
        half_stated : \\Z. has Z cake => gives Z jake cake = gift a;
        full = half h;
        both = gift a h;
    `;

    it('partial application yields an implication over the remaining premises', () => {
        const m = checkSource(src);
        expect(proves(m, 'half')).toBe('\\Z. has Z cake => gives Z jake cake');
        expect(proves(m, 'half_stated')).toBe('\\Z. has Z cake => gives Z jake cake');
    });

    it('finishing later is the same as applying everything at once', () => {
        const m = checkSource(src);
        expect(proves(m, 'full')).toBe('gives mom jake cake');
        expect(proves(m, 'both')).toBe('gives mom jake cake');
    });

    it('a partially applied rule is reusable, with its remaining variables fresh at each use', () => {
        const m = checkSource(
            src + 'p2 : \\A B. A => B => both A B;\nh2 : has dad cake;\ntwo = p2 (half h) (half h2);',
        );
        expect(proves(m, 'two')).toBe('both (gives mom jake cake) (gives dad jake cake)');
    });
});

describe('stated claims', () => {
    const base = `
        a : likes jake cake;
        b : \\X. likes X cake => likes X sugar;
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
                checkSource(base + 'c : \\X. likes X sugar = b a;');
            } catch (e) {
                return e as UnilogError;
            }
        })();
        // The claim is checked first, so the clash shows up at the argument that only proves it for jake.
        expect(err?.message).toMatch(
            /Cannot apply b to a[\s\S]*X \(stands for every X, so it must stay general\) does not match jake/,
        );
        expect(err?.message).toMatch(/must stay general/);
    });

    it('accepts a general claim proved generally', () => {
        expect(provesIn('swap : \\X Y. X => Y => X = \\X Y. X;', 'swap')).toBe('\\X Y. X => Y => X');
        expect(() => checkSource('bad : \\X Y. X => Y => Y = \\X Y. X;')).toThrow(/does not prove/);
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
                b : \\X. likes X cake => likes X sugar;
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
        expect(() => checkSource('loop : \\X. X = loop;')).toThrow(/cannot be used in its own proof/);
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
        expect(() => checkSource('cheat : \\X. X = \\P. P;')).toThrow(/does not prove/);
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
        const src =
            'to_c : a => c;\nto_d : b => d;\npair : \\A B. A => B => both A B;\nbad = \\P. pair (to_c P) (to_d P);';
        expect(() => checkSource(src)).toThrow(/Cannot apply to_d to P[\s\S]*b does not match a/);
    });

    it('cannot derive A => B for unrelated A and B from assumptions alone', () => {
        expect(() => checkSource('cheat : \\A B. A => B = \\P. P;')).toThrow(/does not prove the stated claim/);
    });

    it('in a proof, an uppercase name is an assumption and a lowercase name a declaration', () => {
        expect(() => checkSource('a : p;\nf = A;')).toThrow(/'A' is not an assumption in scope/);
        expect(() => checkSource('f = \\P. p;')).toThrow(/'p' is not declared/);
        // A declaration and an assumption can share a spelling apart from case without interfering.
        expect(provesIn('p : likes jake cake;\nk = \\P. p;', 'k')).toBe('\\A. A => likes jake cake');
    });

    it('rejects duplicate declarations', () => {
        expect(() => checkSource('a : p;\na : q;')).toThrow(/already declared/);
    });
});

describe('logic built from axioms (no special support in the core)', () => {
    const prelude = `
        pair : \\A B. A => B => and A B;
        fst : \\A B. and A B => A;
        snd : \\A B. and A B => B;
        left : \\A B. A => or A B;
        right : \\B A. B => or A B;
        case : \\A B C. or A B => (A => C) => (B => C) => C;
        explode : \\C. false => C;
    `;

    it('and is commutative', () => {
        expect(provesIn(prelude + 'and_comm = \\P. pair (snd P) (fst P);', 'and_comm')).toBe(
            '\\A B. and A B => and B A',
        );
    });

    it('or is commutative', () => {
        expect(provesIn(prelude + 'or_comm = \\P. case P right left;', 'or_comm')).toBe('\\B A. or B A => or A B');
    });

    it('not P is P => false: contradiction implies anything', () => {
        expect(provesIn(prelude + 'contra = \\P Np. explode (Np P);', 'contra')).toBe('\\A C. A => (A => false) => C');
    });

    it('modus tollens', () => {
        const src = prelude + 'tollens : \\P Q. (P => Q) => (Q => false) => (P => false) = \\F Nq P. Nq (F P);';
        expect(provesIn(src, 'tollens')).toBe('\\P Q. (P => Q) => (Q => false) => P => false');
    });

    it('excluded middle is not provable, but can be stated as a belief', () => {
        expect(() => checkSource(prelude + 'lem : \\P. or P (P => false) = \\P. P;')).toThrow(/does not prove/);
        const m = checkSource(
            prelude +
                'lem : \\P. or P (P => false);\ndne : \\P. ((P => false) => false) => P = \\Nnp. case lem (\\P. P) (\\Np. explode (Nnp Np));',
        );
        expect(proves(m, 'dne')).toBe('\\P. ((P => false) => false) => P');
        expect(m.decls.get('lem')?.kind).toBe('axiom');
    });
});

describe('dependencies between declarations', () => {
    it('records which declarations each proof uses', () => {
        const m = checkSource(`
            a : likes jake cake;
            b : \\X. likes X cake => likes X sugar;
            c = b a;
            d = \\P. c;
        `);
        expect(m.decls.get('c')?.uses).toEqual(['Test.b', 'Test.a']);
        expect(m.decls.get('d')?.uses).toEqual(['Test.c']);
        expect(m.decls.get('a')?.uses).toEqual([]);
    });
});

describe('quantifiers', () => {
    const base = `
        pair : \\A B. A => B => both A B;
        mortal_all : \\X. mortal X;
        mortal_socrates : mortal socrates;
        premise : (\\M. mortal M) => both (mortal socrates) (mortal plato) = \\F. pair F F;
    `;

    it('a universal premise can be used at different instances', () => {
        expect(provesIn(base, 'premise')).toBe('(\\M. mortal M) => both (mortal socrates) (mortal plato)');
    });

    it('a universal premise needs a general proof', () => {
        expect(provesIn(base + 'ok = premise mortal_all;', 'ok')).toBe('both (mortal socrates) (mortal plato)');
        expect(() => checkSource(base + 'bad = premise mortal_socrates;')).toThrow(
            /Cannot apply premise to mortal_socrates[\s\S]*M \(stands for every M, so it must stay general\) does not match socrates/,
        );
    });

    it('every variable must be bound', () => {
        expect(() => checkSource('a : likes X cake;')).toThrow(/'X' is not bound/);
    });

    it("rejects a variable applied to itself (Curry's paradox)", () => {
        expect(() => checkSource('c : \\C. C C;')).toThrow(/'C' is applied to itself/);
    });

    it('rejects a variable used both as a proposition and as something with a gap', () => {
        expect(() => checkSource('c : \\F. F jake => F;')).toThrow(/'F' is used inconsistently/);
    });

    describe('defined by users', () => {
        const exists = `
            exists_intro : \\B T. B T => exists B;
            exists_elim : \\B C. exists B => (\\Y. B Y => C) => C;
            lois : loves lois superman;
            someone : exists \\X. loves X superman = exists_intro lois;
            loved_if_anyone : \\X. loves X superman => loved superman;
        `;

        it('introduces a quantifier with a rule written once, the claim locating the gap', () => {
            expect(provesIn(exists, 'someone')).toBe('exists \\X. loves X superman');
        });

        it("asks for the claim when the gap can't be located", () => {
            expect(() => checkSource(exists + 'guess = exists_intro lois;')).toThrow(
                /Cannot tell where the gap is in B[\s\S]*State the claim/,
            );
        });

        it('eliminates it with a handler that works for anyone', () => {
            expect(provesIn(exists + 'it = exists_elim someone loved_if_anyone;', 'it')).toBe('loved superman');
        });

        it('rejects a handler that only works for one person', () => {
            const src = exists + 'if_lois : loves lois superman => loved superman;\nbad = exists_elim someone if_lois;';
            expect(() => checkSource(src)).toThrow(
                /Y \(stands for every Y, so it must stay general\) does not match lois/,
            );
        });

        it("rejects a conclusion about the unknown someone (it can't escape its 'for every')", () => {
            const src = exists + 'happy_if : \\X. loves X superman => happy X;\nbad = exists_elim someone happy_if;';
            expect(() => checkSource(src)).toThrow(/stands for every Y here, so it can't be part of C/);
        });
    });

    describe('rules that look inside lambdas', () => {
        it('matches the parts of a body', () => {
            const src = `
                some_both_left : \\P Q. (exists \\X. both (P X) (Q X)) => exists P;
                dog_barks : exists \\X. both (dog X) (barks X);
                some_dog = some_both_left dog_barks;
            `;
            expect(provesIn(src, 'some_dog')).toBe('exists dog');
        });

        it("can require a body not to mention the lambda's variable", () => {
            const src = `
                vacuous : \\C. (exists \\X. C) => C;
                yes : exists \\X. happy jake;
                no : exists \\X. happy X;
                ok = vacuous yes;
            `;
            expect(provesIn(src, 'ok')).toBe('happy jake');
            expect(() => checkSource(src + 'bad = vacuous no;')).toThrow(/C can't depend on X/);
        });

        it('can match the quantifier itself with a variable', () => {
            const src = `
                ext : \\Q B C. (\\X. B X => C X) => (\\X. C X => B X) => Q B => Q C;
                dog_canine : \\X. dog X => canine X;
                canine_dog : \\X. canine X => dog X;
                most_dogs_bark : most dog;
                most_canines : most canine = ext dog_canine canine_dog most_dogs_bark;
            `;
            expect(provesIn(src, 'most_canines')).toBe('most canine');
        });
    });

    describe('equality as two rules', () => {
        const eq = `
            refl : \\X. eq X X;
            subst : \\X Y P. eq X Y => P X => P Y;
            kent : eq clark_kent superman;
        `;

        it('derives symmetry and transitivity', () => {
            const src =
                eq +
                `
                sym : \\X Y. eq X Y => eq Y X = \\E. subst E refl;
                trans : \\X Y Z. eq X Y => eq Y Z => eq X Z = \\Exy Eyz. subst Eyz Exy;
            `;
            expect(provesIn(src, 'sym')).toBe('\\X Y. eq X Y => eq Y X');
            expect(provesIn(src, 'trans')).toBe('\\X Y Z. eq X Y => eq Y Z => eq X Z');
        });

        it('substitutes in any position, including inside a universal', () => {
            const src =
                eq +
                `
                kent_loves_lois : loves clark_kent lois;
                superman_loves_lois : loves superman lois = subst kent kent_loves_lois;
                all_like_kent : \\X. likes X clark_kent;
                all_like_superman : \\X. likes X superman = subst kent all_like_kent;
            `;
            expect(provesIn(src, 'superman_loves_lois')).toBe('loves superman lois');
            expect(provesIn(src, 'all_like_superman')).toBe('\\X. likes X superman');
        });

        it('rejects a substitution that changes something else too', () => {
            const src = eq + 'k : loves clark_kent lois;\nbad : loves superman mary = subst kent k;';
            expect(() => checkSource(src)).toThrow(/does not match/);
        });
    });
});
