import { describe, expect, it } from 'vitest';
import { app, atom, fresh, generalize, imp, instantiate, Printer, resolve, rigid, show, unify } from '../src';

const likes = (x: Parameters<typeof app>[0], y: Parameters<typeof app>[0]) => app(app(atom('likes'), x), y);

describe('unify', () => {
    it('binds variables', () => {
        const X = fresh('X');
        expect(unify(likes(X, atom('cake')), likes(atom('jake'), atom('cake')))).toBeUndefined();
        expect(show(resolve(X))).toBe('jake');
    });

    it('reports the clashing pair', () => {
        const m = unify(likes(atom('jake'), atom('cake')), likes(atom('jake'), atom('pie')));
        expect(m && [show(m.a), show(m.b)]).toEqual(['cake', 'pie']);
    });

    it('performs the occurs check', () => {
        const X = fresh('X');
        expect(unify(X, app(atom('s'), X))?.occurs).toBe(true);
    });

    it('lets a variable stand for a predicate (P X matches likes jake cake)', () => {
        const P = fresh('P');
        const X = fresh('X');
        expect(unify(app(P, X), likes(atom('jake'), atom('cake')))).toBeUndefined();
        expect([show(resolve(P)), show(resolve(X))]).toEqual(['likes jake', 'cake']);
    });

    it('never binds a rigid variable', () => {
        const R = rigid('X');
        expect(unify(R, atom('jake'))).toBeDefined();
        expect(unify(R, R)).toBeUndefined();
        expect(unify(R, rigid('X'))).toBeDefined();
    });

    it('distinguishes implication from application', () => {
        expect(unify(imp(atom('a'), atom('b')), app(atom('a'), atom('b')))).toBeDefined();
    });
});

describe('generalize / instantiate', () => {
    it('gives each use fresh variables', () => {
        const X = fresh('X');
        const s = generalize(imp(X, X));
        const one = instantiate(s);
        const two = instantiate(s);
        expect(unify(one, imp(atom('a'), atom('a')))).toBeUndefined();
        expect(unify(two, imp(atom('b'), atom('b')))).toBeUndefined();
    });

    it('turns rigid variables back into quantified ones', () => {
        const X = rigid('X');
        const s = generalize(imp(X, imp(X, rigid('Y'))));
        expect(s.vars).toHaveLength(2);
        expect(unify(instantiate(s), imp(atom('a'), imp(atom('a'), atom('b'))))).toBeUndefined();
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

    it('keeps source names, disambiguating collisions, and letters invented variables', () => {
        const p = new Printer();
        expect(p.print(imp(fresh('X'), imp(fresh('X'), fresh())))).toBe('X => X1 => A');
    });
});
