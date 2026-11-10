/**
 * Propositions. A proposition is a symbolic expression:
 *
 *   atom      lowercase name, e.g. `cake`. Global symbols: the same spelling means the same thing everywhere.
 *   var       uppercase name, e.g. `X`. Stands for any proposition (or part of one).
 *   rigid     a variable that may not be instantiated: used while checking a stated claim, so a proof must
 *             establish the claim in full generality, not a special case of it.
 *   app       application, e.g. `likes jake cake` is ((likes jake) cake). Curried and binary.
 *   imp       implication `A => B`. A proof of `A => B` turns proofs of A into proofs of B.
 *
 * Variables are unified destructively: a var's `ref` is set when it is bound.
 */
export type Term = Atom | Var | Rigid | App | Imp;

export interface Atom {
    readonly k: 'atom';
    readonly name: string;
}
export interface Var {
    readonly k: 'var';
    readonly id: number;
    /** Source name, used only for printing. */
    readonly hint: string;
    ref?: Term;
}
export interface Rigid {
    readonly k: 'rigid';
    readonly id: number;
    readonly hint: string;
}
export interface App {
    readonly k: 'app';
    readonly fn: Term;
    readonly arg: Term;
}
export interface Imp {
    readonly k: 'imp';
    readonly from: Term;
    readonly to: Term;
}

let nextId = 0;

export const atom = (name: string): Atom => ({ k: 'atom', name });
/** `hint` is the variable's source name. Variables the checker invents have no hint, and print as A, B, C... */
export const fresh = (hint = ''): Var => ({ k: 'var', id: nextId++, hint });
export const rigid = (hint: string): Rigid => ({ k: 'rigid', id: nextId++, hint });
export const app = (fn: Term, arg: Term): App => ({ k: 'app', fn, arg });
export const imp = (from: Term, to: Term): Imp => ({ k: 'imp', from, to });

/** Follows variable bindings until reaching an unbound variable or a non-variable. */
export function prune(t: Term): Term {
    while (t.k === 'var' && t.ref) t = t.ref;
    return t;
}

/** A copy with every bound variable replaced by what it is bound to. */
export function resolve(t: Term): Term {
    t = prune(t);
    switch (t.k) {
        case 'app':
            return app(resolve(t.fn), resolve(t.arg));
        case 'imp':
            return imp(resolve(t.from), resolve(t.to));
        default:
            return t;
    }
}

function occurs(v: Var, t: Term): boolean {
    t = prune(t);
    if (t === v) return true;
    if (t.k === 'app') return occurs(v, t.fn) || occurs(v, t.arg);
    if (t.k === 'imp') return occurs(v, t.from) || occurs(v, t.to);
    return false;
}

/** Why two terms failed to unify: the innermost pair that clashed. */
export interface Mismatch {
    a: Term;
    b: Term;
    occurs?: boolean;
}

/**
 * First-order syntactic unification with occurs check. Binds variables in place.
 * Returns undefined on success, or the clashing pair on failure (bindings made so far are left in place;
 * callers treat failure as fatal).
 */
export function unify(a: Term, b: Term): Mismatch | undefined {
    a = prune(a);
    b = prune(b);
    if (a === b) return undefined;
    if (a.k === 'var') {
        if (occurs(a, b)) return { a, b, occurs: true };
        a.ref = b;
        return undefined;
    }
    if (b.k === 'var') return unify(b, a);
    if (a.k === 'atom' && b.k === 'atom' && a.name === b.name) return undefined;
    if (a.k === 'rigid' && b.k === 'rigid' && a.id === b.id) return undefined;
    if (a.k === 'app' && b.k === 'app') return unify(a.fn, b.fn) ?? unify(a.arg, b.arg);
    if (a.k === 'imp' && b.k === 'imp') return unify(a.from, b.from) ?? unify(a.to, b.to);
    return { a, b };
}

/** A proposition with its variables universally quantified ("for all X, ..."). */
export interface Scheme {
    readonly vars: readonly Var[];
    readonly body: Term;
}

function collectVars(t: Term, out: Var[]): void {
    t = prune(t);
    if (t.k === 'var') {
        if (!out.includes(t)) out.push(t);
    } else if (t.k === 'app') {
        collectVars(t.fn, out);
        collectVars(t.arg, out);
    } else if (t.k === 'imp') {
        collectVars(t.from, out);
        collectVars(t.to, out);
    }
}

/** Quantifies over every unbound variable. Rigid variables become ordinary quantified variables again. */
export function generalize(t: Term): Scheme {
    const rigids = new Map<number, Var>();
    const unrigid = (u: Term): Term => {
        u = prune(u);
        switch (u.k) {
            case 'rigid': {
                let v = rigids.get(u.id);
                if (!v) rigids.set(u.id, (v = fresh(u.hint)));
                return v;
            }
            case 'app':
                return app(unrigid(u.fn), unrigid(u.arg));
            case 'imp':
                return imp(unrigid(u.from), unrigid(u.to));
            default:
                return u;
        }
    };
    const body = unrigid(t);
    const vars: Var[] = [];
    collectVars(body, vars);
    return { vars, body };
}

/** A fresh copy of a scheme's body, with new variables for each quantified one. */
export function instantiate(s: Scheme): Term {
    const map = new Map<Var, Var>(s.vars.map((v) => [v, fresh(v.hint)]));
    const copy = (t: Term): Term => {
        t = prune(t);
        switch (t.k) {
            case 'var':
                return map.get(t) ?? t;
            case 'app':
                return app(copy(t.fn), copy(t.arg));
            case 'imp':
                return imp(copy(t.from), copy(t.to));
            default:
                return t;
        }
    };
    return copy(s.body);
}

/**
 * A copy of a scheme's body with each quantified variable replaced by a rigid one: "an arbitrary, fixed
 * value". Something that unifies with this proves the scheme in full generality.
 */
export function instantiateRigid(s: Scheme): Term {
    const map = new Map<Var, Rigid>(s.vars.map((v) => [v, rigid(v.hint)]));
    const copy = (t: Term): Term => {
        t = prune(t);
        switch (t.k) {
            case 'var':
                return map.get(t) ?? t;
            case 'app':
                return app(copy(t.fn), copy(t.arg));
            case 'imp':
                return imp(copy(t.from), copy(t.to));
            default:
                return t;
        }
    };
    return copy(s.body);
}

/**
 * Prints terms, giving each distinct variable a distinct name. Variables keep their source names where
 * possible (X, X1, X2 ... when names collide). One printer per message, so names are consistent within it.
 */
export class Printer {
    private readonly names = new Map<number, string>();
    private readonly used = new Set<string>();

    private nameOf(id: number, hint: string): string {
        let name = this.names.get(id);
        if (name) return name;
        if (hint === '' || hint === '_') {
            const letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
            for (let i = 0; ; i++) {
                name = (letters[i % 26] as string) + (i >= 26 ? String(Math.floor(i / 26)) : '');
                if (!this.used.has(name)) break;
            }
        } else {
            name = hint;
            for (let i = 1; this.used.has(name); i++) name = `${hint}${i}`;
        }
        this.names.set(id, name);
        this.used.add(name);
        return name;
    }

    print(t: Term): string {
        t = prune(t);
        switch (t.k) {
            case 'atom':
                return t.name;
            case 'var':
            case 'rigid':
                return this.nameOf(t.id, t.hint);
            case 'app': {
                const fn = prune(t.fn);
                const arg = prune(t.arg);
                const f = fn.k === 'imp' ? `(${this.print(fn)})` : this.print(fn);
                const a = arg.k === 'app' || arg.k === 'imp' ? `(${this.print(arg)})` : this.print(arg);
                return `${f} ${a}`;
            }
            case 'imp': {
                const from = prune(t.from);
                const f = from.k === 'imp' ? `(${this.print(from)})` : this.print(from);
                return `${f} => ${this.print(t.to)}`;
            }
        }
    }
}

/** Prints a single term with fresh variable names. */
export function show(t: Term): string {
    return new Printer().print(t);
}
