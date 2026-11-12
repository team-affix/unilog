import { describe, expect, it } from 'vitest';
import {
    all,
    app,
    atom,
    bound,
    fresh,
    generalize,
    imp,
    lam,
    Printer,
    proves,
    propositionTerm,
    parseProposition,
    rigid,
    show,
    Term,
    Unifier,
} from '../src';

const likes = (x: Term, y: Term) => app(app(atom('likes'), x), y);
const prop = (s: string) => propositionTerm(parseProposition(s));

describe('matching', () => {
    it('fills unknowns', () => {
        const u = new Unifier();
        const X = u.fresh('X');
        expect(u.unify(likes(X, atom('cake')), likes(atom('jake'), atom('cake')))).toBeUndefined();
        expect(show(X)).toBe('jake');
    });

    it('reports the clashing pair', () => {
        const m = new Unifier().unify(likes(atom('jake'), atom('cake')), likes(atom('jake'), atom('pie')));
        expect(m && [show(m.a), show(m.b)]).toEqual(['cake', 'pie']);
    });

    it('performs the occurs check', () => {
        const u = new Unifier();
        const X = u.fresh('X');
        expect(u.unify(X, app(atom('s'), X))?.occurs).toBe(true);
    });

    it('never fills a fixed symbol', () => {
        const u = new Unifier();
        const R = rigid('X');
        expect(u.unify(R, atom('jake'))).toBeDefined();
        expect(u.unify(R, R)).toBeUndefined();
        expect(u.unify(R, rigid('X'))).toBeDefined();
    });

    it('distinguishes implication from application', () => {
        expect(new Unifier().unify(imp(atom('a'), atom('b')), app(atom('a'), atom('b')))).toBeDefined();
    });

    it("keeps a fixed symbol inside its 'for every': an unknown from outside can't contain it", () => {
        const u = new Unifier();
        const outside = u.fresh('C');
        const x = u.enter('X', 't');
        expect(u.unify(outside, app(atom('happy'), x))?.escape).toBe(x);
        const inside = u.fresh('D');
        expect(u.unify(inside, app(atom('happy'), x))).toBeUndefined();
        u.leave();
    });

    it('compares lambdas up to renaming their variables, and looks inside them', () => {
        expect(proves(prop('exists \\X. loves X mom'), prop('exists \\Y. loves Y mom'))).toBe(true);
        const u = new Unifier();
        const P = u.fresh('P', { from: 't', to: 't' });
        // exists \X. both (P X) barks  against  exists \X. both (dog X) barks: P is dog.
        const pattern = app(atom('exists'), lam('X', 't', app(app(atom('both'), app(P, bound(0))), atom('barks'))));
        expect(u.unify(pattern, prop('exists \\X. both (dog X) barks'))).toBeUndefined();
        expect(show(P)).toBe('dog');
    });

    it('treats \\X. f X as f', () => {
        expect(proves(prop('exists \\X. dog X'), prop('exists dog'))).toBe(true);
        expect(show(prop('exists \\X. dog X'))).toBe('exists dog');
    });
});

describe('gaps', () => {
    it('fills a gap applied to something', () => {
        expect(show(app(lam('X', 't', likes(bound(0), atom('mom'))), atom('socrates')))).toBe('likes socrates mom');
    });

    it('waits on a gap that is not determined yet, and solves it once it is', () => {
        const u = new Unifier();
        const B = u.fresh('B', { from: 't', to: 't' });
        const T = u.fresh('T');
        expect(u.unify(app(B, T), likes(atom('lois'), atom('superman')))).toBeUndefined();
        expect(u.unresolved()).toHaveLength(1);
        expect(u.unify(B, lam('X', 't', likes(bound(0), atom('superman'))))).toBeUndefined();
        expect(show(T)).toBe('lois');
        expect(u.unresolved()).toHaveLength(0);
    });

    it('finds the gap from two fillings: where they differ', () => {
        const u = new Unifier();
        const B = u.fresh('B', { from: 't', to: 't' });
        u.unify(app(B, atom('clark')), likes(atom('clark'), atom('lois')));
        u.unify(app(B, atom('superman')), likes(atom('superman'), atom('lois')));
        expect(u.unresolved()).toHaveLength(0);
        expect(show(B)).toBe('\\A. likes A lois');
    });

    it('leaves an ambiguous gap unresolved rather than guessing', () => {
        const u = new Unifier();
        const B = u.fresh('B', { from: 't', to: 't' });
        u.unify(app(B, atom('jake')), likes(atom('jake'), atom('cake')));
        expect(u.unresolved()).toHaveLength(1);
    });
});

describe('proves', () => {
    it('a universal proves its instances, not the reverse', () => {
        expect(proves(prop('\\X. wrong X'), prop('wrong bob'))).toBe(true);
        expect(proves(prop('wrong bob'), prop('\\X. wrong X'))).toBe(false);
        expect(proves(prop('\\X. wrong X'), prop('\\Y. wrong Y'))).toBe(true);
    });
});

describe('generalize', () => {
    it('binds the remaining unknowns at the front, in order', () => {
        const X = fresh('X');
        const Y = fresh('Y');
        expect(show(generalize(imp(X, imp(Y, X))))).toBe('\\X Y. X => Y => X');
    });

    it('names clashing source names afresh', () => {
        expect(show(generalize(imp(fresh('B'), fresh('B'))))).toBe('\\A B. A => B');
    });
});

describe('Printer', () => {
    it('parenthesizes only where needed', () => {
        const a = atom('a');
        const b = atom('b');
        expect(show(imp(imp(a, b), imp(a, b)))).toBe('(a => b) => a => b');
        expect(show(app(atom('f'), app(a, b)))).toBe('f (a b)');
        expect(show(app(app(atom('f'), a), b))).toBe('f a b');
        expect(show(app(atom('not'), imp(a, b)))).toBe('not (a => b)');
    });

    it('writes lambdas as \\X. body, in parentheses unless nothing follows', () => {
        const gap = lam('X', 't', likes(bound(0), atom('mom')));
        expect(show(app(atom('exists'), gap))).toBe('exists \\X. likes X mom');
        expect(show(imp(app(atom('exists'), gap), atom('q')))).toBe('exists (\\X. likes X mom) => q');
        expect(show(all('X', 't', all('Y', 't', likes(bound(1), bound(0)))))).toBe('\\X Y. likes X Y');
        expect(show(imp(all('M', 't', app(atom('mortal'), bound(0))), atom('q')))).toBe('(\\M. mortal M) => q');
    });

    it('keeps source names, disambiguating collisions, and letters invented unknowns', () => {
        const p = new Printer();
        expect(p.print(imp(fresh('X'), imp(fresh('X'), fresh())))).toBe('X => X1 => A');
    });
});
