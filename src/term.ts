/**
 * Propositions. A proposition is a symbolic expression:
 *
 *   atom      lowercase name, e.g. `cake`. Global symbols: the same spelling means the same thing everywhere.
 *   var       an unknown: stands for a proposition (or part of one) that matching fills in.
 *   rigid     a fixed symbol: matches only itself. Used while checking "for every X": the proof must work
 *             for an X about which nothing is known.
 *   bound     a variable bound by an enclosing lambda (a de Bruijn index: 0 is the nearest binder).
 *   app       application, e.g. `likes jake cake` is ((likes jake) cake). Curried and binary.
 *   imp       implication `A => B`. A proof of `A => B` turns proofs of A into proofs of B.
 *   lam       a gap: `\X. P` handed to something, e.g. the `\X. loves X mom` in `exists \X. loves X mom`.
 *             `(\X. P) A` is the same proposition as P with A for X.
 *   all       "for every X, P": a lambda where a proposition is expected.
 *
 * Unknowns are filled in place: a var's `ref` is set when it is solved.
 */
export type Term = Atom | Var | Rigid | Bound | App | Imp | Lam | All;

/** Types, inferred and never written: `t` for any proposition or symbol, `A -> B` for something with a gap. */
export type Ty = 't' | { readonly from: Ty; readonly to: Ty };

export interface Atom {
    readonly k: 'atom';
    readonly name: string;
}
export interface Var {
    readonly k: 'var';
    readonly id: number;
    /** Source name, used only for printing. */
    readonly hint: string;
    readonly ty: Ty;
    /** How many "for every" scopes deep it was made: it may only be filled with fixed symbols from those. */
    level: number;
    ref?: Term;
}
export interface Rigid {
    readonly k: 'rigid';
    readonly id: number;
    readonly hint: string;
    readonly ty: Ty;
    /** The "for every" scope it belongs to. */
    readonly level: number;
}
export interface Bound {
    readonly k: 'bound';
    readonly i: number;
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
export interface Lam {
    readonly k: 'lam';
    readonly hint: string;
    readonly ty: Ty;
    readonly body: Term;
}
export interface All {
    readonly k: 'all';
    readonly hint: string;
    readonly ty: Ty;
    readonly body: Term;
}

let nextId = 0;

export const atom = (name: string): Atom => ({ k: 'atom', name });
/** `hint` is the variable's source name. Unknowns the checker invents have no hint, and print as A, B, C... */
export const fresh = (hint = '', ty: Ty = 't', level = 0): Var => ({ k: 'var', id: nextId++, hint, ty, level });
export const rigid = (hint: string, ty: Ty = 't', level = 0): Rigid => ({ k: 'rigid', id: nextId++, hint, ty, level });
export const bound = (i: number): Bound => ({ k: 'bound', i });
export const app = (fn: Term, arg: Term): App => ({ k: 'app', fn, arg });
export const imp = (from: Term, to: Term): Imp => ({ k: 'imp', from, to });
export const lam = (hint: string, ty: Ty, body: Term): Lam => ({ k: 'lam', hint, ty, body });
export const all = (hint: string, ty: Ty, body: Term): All => ({ k: 'all', hint, ty, body });

export const arrow = (from: Ty, to: Ty): Ty => ({ from, to });
export const tyEq = (a: Ty, b: Ty): boolean =>
    a === 't' || b === 't' ? a === b : tyEq(a.from, b.from) && tyEq(a.to, b.to);
export const showTy = (t: Ty): string =>
    t === 't' ? 't' : `${t.from === 't' ? 't' : `(${showTy(t.from)})`} -> ${showTy(t.to)}`;

/** Follows solved unknowns until reaching an unsolved one or something else. */
export function prune(t: Term): Term {
    while (t.k === 'var' && t.ref) t = t.ref;
    return t;
}

// --- Bound variables (de Bruijn indices) ---

/** Adds `d` to every bound index of at least `cutoff`. */
function shift(t: Term, d: number, cutoff = 0): Term {
    t = prune(t);
    switch (t.k) {
        case 'bound':
            return t.i >= cutoff ? bound(t.i + d) : t;
        case 'app':
            return app(shift(t.fn, d, cutoff), shift(t.arg, d, cutoff));
        case 'imp':
            return imp(shift(t.from, d, cutoff), shift(t.to, d, cutoff));
        case 'lam':
        case 'all':
            return { ...t, body: shift(t.body, d, cutoff + 1) };
        default:
            return t;
    }
}

/** Replaces bound index `j` with `s`, removing that binder (indices above it go down by one). */
function subst(t: Term, j: number, s: Term): Term {
    t = prune(t);
    switch (t.k) {
        case 'bound':
            return t.i === j ? shift(s, j) : t.i > j ? bound(t.i - 1) : t;
        case 'app':
            return app(subst(t.fn, j, s), subst(t.arg, j, s));
        case 'imp':
            return imp(subst(t.from, j, s), subst(t.to, j, s));
        case 'lam':
        case 'all':
            return { ...t, body: subst(t.body, j + 1, s) };
        default:
            return t;
    }
}

/** A binder's body with `s` for its variable. */
export const open = (b: Lam | All, s: Term): Term => subst(b.body, 0, s);

/** Replaces every occurrence of `target` (an unknown or fixed symbol) in `t` with the bound variable of a new outer binder. */
function close(t: Term, target: Var | Rigid, depth = 0): Term {
    t = prune(t);
    if (t === target || (t.k === 'rigid' && target.k === 'rigid' && t.id === target.id)) return bound(depth);
    switch (t.k) {
        case 'bound':
            return t.i >= depth ? bound(t.i + 1) : t;
        case 'app':
            return app(close(t.fn, target, depth), close(t.arg, target, depth));
        case 'imp':
            return imp(close(t.from, target, depth), close(t.to, target, depth));
        case 'lam':
        case 'all':
            return { ...t, body: close(t.body, target, depth + 1) };
        default:
            return t;
    }
}

/**
 * Filling gaps always finishes (that is what the types guarantee), but as a safety net against inputs built to
 * be expensive, a check gives up after this many gap fillings.
 */
const MAX_FILLS = 1_000_000;
let fills = 0;
/** Resets the safety net: called once per declaration. */
export const resetFills = (): void => {
    fills = 0;
};

/** Head normal form: solved unknowns followed, and a gap applied to something filled. */
export function whnf(t: Term): Term {
    t = prune(t);
    if (t.k !== 'app') return t;
    const fn = whnf(t.fn);
    if (fn.k === 'lam') {
        if (++fills > MAX_FILLS) throw new Error('This proposition is too large to check');
        return whnf(subst(fn.body, 0, t.arg));
    }
    return fn === t.fn ? t : app(fn, t.arg);
}

/** Fully normalized copy: every solved unknown replaced, every gap filled. (Types make this always finish.) */
export function norm(t: Term): Term {
    t = whnf(t);
    switch (t.k) {
        case 'app':
            return app(norm(t.fn), norm(t.arg));
        case 'imp':
            return imp(norm(t.from), norm(t.to));
        case 'all':
            return { ...t, body: norm(t.body) };
        case 'lam': {
            // \X. f X is just f (when f doesn't mention X).
            const body = norm(t.body);
            if (body.k === 'app' && body.arg.k === 'bound' && body.arg.i === 0 && !mentions(body.fn, 0))
                return shift(body.fn, -1);
            return { ...t, body };
        }
        default:
            return t;
    }
}

/** Whether bound index i (counted from t's top) occurs in t. */
function mentions(t: Term, i: number): boolean {
    t = prune(t);
    switch (t.k) {
        case 'bound':
            return t.i === i;
        case 'app':
            return mentions(t.fn, i) || mentions(t.arg, i);
        case 'imp':
            return mentions(t.from, i) || mentions(t.to, i);
        case 'lam':
        case 'all':
            return mentions(t.body, i + 1);
        default:
            return false;
    }
}

/** The head and arguments of an application: `f a b` is f with [a, b]. */
function spine(t: Term): { head: Term; args: Term[] } {
    const args: Term[] = [];
    t = whnf(t);
    while (t.k === 'app') {
        args.unshift(t.arg);
        t = whnf(t.fn);
    }
    return { head: t, args };
}

function visit(t: Term, f: (t: Term) => void): void {
    t = prune(t);
    f(t);
    switch (t.k) {
        case 'app':
            visit(t.fn, f);
            visit(t.arg, f);
            break;
        case 'imp':
            visit(t.from, f);
            visit(t.to, f);
            break;
        case 'lam':
        case 'all':
            visit(t.body, f);
    }
}

/** The type of a term, where it can be told: unknowns and fixed symbols carry theirs; atoms can be anything. */
export function typeOf(t: Term): Ty | undefined {
    t = whnf(t);
    switch (t.k) {
        case 'var':
        case 'rigid':
            return t.ty;
        case 'lam':
            return arrow(t.ty, 't');
        case 'all':
        case 'imp':
            return 't';
        case 'app': {
            const f = typeOf(t.fn);
            return f && f !== 't' ? f.to : undefined;
        }
        default:
            return undefined;
    }
}

/** Structural equality of normalized terms (bound variables are indices, so renaming doesn't matter). */
function same(a: Term, b: Term): boolean {
    a = prune(a);
    b = prune(b);
    if (a === b) return true;
    switch (a.k) {
        case 'atom':
            return b.k === 'atom' && a.name === b.name;
        case 'rigid':
            return b.k === 'rigid' && a.id === b.id;
        case 'bound':
            return b.k === 'bound' && a.i === b.i;
        case 'app':
            return b.k === 'app' && same(a.fn, b.fn) && same(a.arg, b.arg);
        case 'imp':
            return b.k === 'imp' && same(a.from, b.from) && same(a.to, b.to);
        case 'lam':
        case 'all':
            return b.k === a.k && tyEq(a.ty, b.ty) && same(a.body, b.body);
        default:
            return false;
    }
}

/** Why two terms failed to match: the innermost pair that clashed. */
export interface Mismatch {
    a: Term;
    b: Term;
    /** The unknown `a` would have to contain itself. */
    occurs?: boolean;
    /** The fixed symbol `escape` would have to leave its "for every". */
    escape?: Rigid;
    /** Types differ: a gap where a proposition is expected, or the reverse. */
    type?: boolean;
}

/** An application of an unknown that can't be solved yet, e.g. `B T` with both unknown. */
interface Pending {
    head: Var;
    args: Term[];
    rhs: Term;
}

/** Level of the fixed symbols used to look inside two lambdas being matched: nothing outside may contain them. */
const INSIDE = Number.MAX_SAFE_INTEGER;

/** Whether a fixed symbol stands for a lambda's own variable (rather than a "for every" being checked). */
export const isLambdaVariable = (r: Rigid): boolean => r.level === INSIDE;

/**
 * Matching (unification) of propositions, with:
 * - gaps: `B T` with B unknown is solved when the answer is unique (see `flexible`), and otherwise waits;
 * - lambdas: compared by looking inside both with the same fixed symbol;
 * - "for every" scopes: an unknown may only be filled with fixed symbols from its own scope or outer ones;
 * - types: an unknown proposition can't be filled with a gap, nor the reverse.
 */
export class Unifier {
    /** The current "for every" depth. */
    level = 0;
    private pending: Pending[] = [];
    /** Every change made, so a trial guess can be undone. */
    private trail: { v: Var; ref?: Term; level: number }[] = [];

    fresh(hint = '', ty: Ty = 't'): Var {
        return fresh(hint, ty, this.level);
    }

    /** Enters a "for every": returns a fixed symbol for its variable, valid until `leave`. */
    enter(hint: string, ty: Ty): Rigid {
        this.level++;
        return rigid(hint, ty, this.level);
    }

    leave(): void {
        this.level--;
    }

    /** Strips leading "for every"s, putting a new unknown for each variable. */
    instantiate(t: Term): Term {
        t = whnf(t);
        while (t.k === 'all') t = whnf(open(t, this.fresh(t.hint, t.ty)));
        return t;
    }

    /** Matches a and b, filling unknowns. Returns undefined on success, or the clash. */
    unify(a: Term, b: Term): Mismatch | undefined {
        return this.match(a, b) ?? this.retry();
    }

    /** Gaps that could not be determined, once everything else is done. */
    unresolved(): readonly { head: Var; args: readonly Term[]; rhs: Term }[] {
        this.retry();
        return this.pending;
    }

    private match(a: Term, b: Term): Mismatch | undefined {
        a = whnf(a);
        b = whnf(b);
        if (a === b || same(a, b)) return undefined;
        const sa = spine(a);
        const sb = spine(b);
        if (sa.head.k === 'var') return sa.args.length ? this.flexible(sa.head, sa.args, b) : this.solve(sa.head, b);
        if (sb.head.k === 'var') return sb.args.length ? this.flexible(sb.head, sb.args, a) : this.solve(sb.head, a);
        if (a.k === 'atom' && b.k === 'atom' && a.name === b.name) return undefined;
        if (a.k === 'rigid' && b.k === 'rigid' && a.id === b.id) return undefined;
        if (a.k === 'bound' && b.k === 'bound' && a.i === b.i) return undefined;
        if (a.k === 'app' && b.k === 'app') return this.match(a.fn, b.fn) ?? this.match(a.arg, b.arg);
        if (a.k === 'imp' && b.k === 'imp') return this.match(a.from, b.from) ?? this.match(a.to, b.to);
        if ((a.k === 'lam' && b.k === 'lam') || (a.k === 'all' && b.k === 'all')) {
            if (!tyEq(a.ty, b.ty)) return { a, b, type: true };
            const x = rigid(a.hint, a.ty, INSIDE);
            return this.match(open(a, x), open(b as Lam | All, x));
        }
        // A gap against something else with a gap: \X. f X is f.
        if (a.k === 'lam' && b.k !== 'all' && b.k !== 'imp') {
            const x = rigid(a.hint, a.ty, INSIDE);
            return this.match(open(a, x), app(b, x));
        }
        if (b.k === 'lam' && a.k !== 'all' && a.k !== 'imp') {
            const x = rigid(b.hint, b.ty, INSIDE);
            return this.match(app(a, x), open(b, x));
        }
        return { a, b };
    }

    /** Fills unknown v with t, after the occurs, scope and type checks. */
    private solve(v: Var, t: Term): Mismatch | undefined {
        t = norm(t);
        if (t === v) return undefined;
        let failure: Mismatch | undefined;
        visit(t, (u) => {
            if (failure) return;
            if (u === v) failure = { a: v, b: t, occurs: true };
            else if (u.k === 'rigid' && u.level > v.level) failure = { a: v, b: t, escape: u };
        });
        if (failure) return failure;
        const ty = typeOf(t);
        if (ty && !tyEq(ty, v.ty)) return { a: v, b: t, type: true };
        if (v.ty === 't' && t.k === 'lam') return { a: v, b: t, type: true };
        // Unknowns inside t now also stand in v's scope.
        visit(t, (u) => {
            if (u.k === 'var' && u.level > v.level) {
                this.trail.push({ v: u, ref: u.ref, level: u.level });
                u.level = v.level;
            }
        });
        this.trail.push({ v, ref: v.ref, level: v.level });
        v.ref = t;
        return undefined;
    }

    /**
     * `head args = rhs` with head unknown. Solved directly when every argument is a distinct fixed symbol from
     * an inner scope: the gap is wherever each occurs. Otherwise it waits (see `retry`).
     */
    private flexible(head: Var, args: Term[], rhs: Term): Mismatch | undefined {
        const fixed = args.map((x) => whnf(x));
        const pattern =
            fixed.every((x) => x.k === 'rigid' && x.level > head.level) &&
            new Set(fixed.map((x) => (x as Rigid).id)).size === fixed.length;
        if (!pattern) {
            this.pending.push({ head, args, rhs });
            return undefined;
        }
        let body = norm(rhs);
        for (let i = fixed.length - 1; i >= 0; i--) {
            const x = fixed[i] as Rigid;
            body = lam(x.hint, x.ty, close(body, x));
        }
        return this.solve(head, body);
    }

    /**
     * Retries waiting gaps. First those whose unknown has since been filled. Then, for `B a = s` with a and s
     * known, each possible gap (each choice of which occurrences of a in s are the gap) is tried against all the
     * other waiting constraints. Exactly one fits: it is taken. None: a mismatch. Several: it keeps waiting,
     * and if it is still undetermined at the end, the checker asks for the claim instead of guessing.
     */
    private retry(guess = true): Mismatch | undefined {
        for (let progress = true; progress;) {
            progress = false;
            const waiting = this.pending;
            this.pending = [];
            for (const p of waiting) {
                if (prune(p.head) !== p.head) {
                    const m = this.match(
                        p.args.reduce<Term>((f, x) => app(f, x), p.head),
                        p.rhs,
                    );
                    if (m) return m;
                    progress = true;
                } else this.pending.push(p);
            }
            if (progress || !guess) continue;
            for (const p of this.pending) {
                const args = p.args.map(norm);
                const rhs = norm(p.rhs);
                if (!args.every(known) || !known(rhs)) continue;
                const fits: Term[] = [];
                let failure: Mismatch | undefined;
                // The parameter types come from the unknown's own type: B : A1 -> ... -> t.
                const params: Ty[] = [];
                for (let ty: Ty = p.head.ty, i = 0; i < args.length; i++) {
                    params.push(ty === 't' ? (typeOf(args[i] as Term) ?? 't') : ty.from);
                    ty = ty === 't' ? 't' : ty.to;
                }
                for (const body of gaps(rhs, args)) {
                    const candidate = args.reduceRight<Term>((b, _, i) => lam('', params[i] as Ty, b), body);
                    const before = { trail: this.trail.length, pending: this.pending };
                    this.pending = this.pending.filter((q) => q !== p);
                    const m = this.solve(p.head, candidate) ?? this.retry(false);
                    this.undo(before.trail);
                    this.pending = before.pending;
                    if (m) failure ??= m;
                    else fits.push(candidate);
                }
                if (fits.length === 0) return failure ?? { a: p.head, b: rhs };
                if (fits.length === 1) {
                    this.pending = this.pending.filter((q) => q !== p);
                    const m = this.solve(p.head, fits[0] as Term);
                    if (m) return m;
                    progress = true;
                    break;
                }
            }
        }
        return undefined;
    }

    /** A point to come back to with `rollback`. */
    mark(): { trail: number; pending: Pending[] } {
        return { trail: this.trail.length, pending: [...this.pending] };
    }

    /** Undoes everything since `mark`. */
    rollback(m: { trail: number; pending: Pending[] }): void {
        this.undo(m.trail);
        this.pending = m.pending;
    }

    private undo(to: number): void {
        while (this.trail.length > to) {
            const t = this.trail.pop() as { v: Var; ref?: Term; level: number };
            t.v.ref = t.ref;
            t.v.level = t.level;
        }
    }
}

/** True if t contains no unknowns. */
function known(t: Term): boolean {
    let ok = true;
    visit(t, (u) => {
        if (u.k === 'var') ok = false;
    });
    return ok;
}

/** At most this many possible gaps are tried for one constraint. */
const MAX_GAPS = 256;

/**
 * Every body b with `(\Z1 ... Zn. b) args = s`: each subterm of s equal to an argument may be left as it is or
 * become that argument's bound variable.
 */
function gaps(s: Term, args: readonly Term[]): Term[] {
    const n = args.length;
    const go = (t: Term, depth: number): Term[] => {
        t = prune(t);
        const out: Term[] = [];
        args.forEach((x, i) => {
            if (same(t, shift(x, depth))) out.push(bound(depth + n - 1 - i));
        });
        const both = (xs: Term[], ys: Term[], f: (x: Term, y: Term) => Term) =>
            xs.flatMap((x) => ys.map((y) => f(x, y))).slice(0, MAX_GAPS);
        switch (t.k) {
            case 'app':
                out.push(...both(go(t.fn, depth), go(t.arg, depth), app));
                break;
            case 'imp':
                out.push(...both(go(t.from, depth), go(t.to, depth), imp));
                break;
            case 'lam':
            case 'all':
                out.push(...go(t.body, depth + 1).map((body) => ({ ...t, body })));
                break;
            case 'bound':
                out.push(t.i >= depth ? bound(t.i + n) : t);
                break;
            default:
                out.push(t);
        }
        return out.slice(0, MAX_GAPS);
    };
    return go(s, 0);
}

/**
 * Closes a proposition over its remaining unknowns: each becomes a "for every" at the front, in order of first
 * appearance.
 */
export function generalize(t: Term): Term {
    t = norm(t);
    const vars: Var[] = [];
    visit(t, (u) => {
        if (u.k === 'var' && !vars.includes(u)) vars.push(u);
    });
    // A source name is kept only if no other variable here has it; the rest are named A, B, C ... when printed.
    const unique = (v: Var) => v.hint !== '' && vars.filter((w) => w.hint === v.hint).length === 1;
    for (let i = vars.length - 1; i >= 0; i--) {
        const v = vars[i] as Var;
        t = all(unique(v) ? v.hint : '', v.ty, close(t, v));
    }
    return t;
}

/**
 * Prints terms, giving each distinct unknown and fixed symbol a distinct name. They keep their source names
 * where possible (X, X1, X2 ... when names collide). One printer per message, so names are consistent within it.
 */
export class Printer {
    private readonly names = new Map<number, string>();
    private readonly used = new Set<string>();

    private pick(hint: string, taken: (n: string) => boolean): string {
        if (hint === '' || hint === '_') {
            const letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
            for (let i = 0; ; i++) {
                const name = (letters[i % 26] as string) + (i >= 26 ? String(Math.floor(i / 26)) : '');
                if (!taken(name)) return name;
            }
        }
        let name = hint;
        for (let i = 1; taken(name); i++) name = `${hint}${i}`;
        return name;
    }

    private nameOf(id: number, hint: string): string {
        let name = this.names.get(id);
        if (name) return name;
        name = this.pick(hint, (n) => this.used.has(n));
        this.names.set(id, name);
        this.used.add(name);
        return name;
    }

    print(t: Term): string {
        return this.show(norm(t), [], true);
    }

    /** `scope`: names of the enclosing binders, innermost last. `tail`: nothing follows, so a lambda needs no parentheses. */
    private show(t: Term, scope: string[], tail: boolean): string {
        t = prune(t);
        switch (t.k) {
            case 'atom':
                return t.name;
            case 'var':
            case 'rigid':
                return this.nameOf(t.id, t.hint);
            case 'bound':
                return scope[scope.length - 1 - t.i] ?? `?${t.i}`;
            case 'app': {
                const fn = this.show(t.fn, scope, false);
                const arg = prune(t.arg);
                const a =
                    arg.k === 'lam' && tail
                        ? this.show(arg, scope, true)
                        : arg.k === 'app' || arg.k === 'imp' || arg.k === 'lam' || arg.k === 'all'
                          ? `(${this.show(arg, scope, true)})`
                          : this.show(arg, scope, false);
                return `${prune(t.fn).k === 'imp' ? `(${fn})` : fn} ${a}`;
            }
            case 'imp': {
                const from = prune(t.from);
                const left = from.k === 'imp' ? `(${this.show(from, scope, true)})` : this.show(from, scope, false);
                return `${left} => ${this.show(t.to, scope, tail)}`;
            }
            case 'lam':
            case 'all': {
                // Consecutive "for every"s are written together: \X Y. body.
                const names: string[] = [];
                let body: Term = t;
                const inner = [...scope];
                for (let b = prune(body); b.k === t.k && (t.k === 'all' || names.length === 0); b = prune(body)) {
                    const bb = b as Lam | All;
                    const name = this.pick(bb.hint, (n) => this.used.has(n) || inner.includes(n));
                    names.push(name);
                    inner.push(name);
                    body = bb.body;
                }
                const text = `\\${names.join(' ')}. ${this.show(body, inner, true)}`;
                return tail ? text : `(${text})`;
            }
        }
    }
}

/** Prints a single term with fresh variable names. */
export function show(t: Term): string {
    return new Printer().print(t);
}

/**
 * Whether a proof of `a` is also a proof of `b` (both closed): b's "for every"s are fixed symbols, a's are
 * unknowns that matching may fill. `{X} wrong X` proves `wrong bob`, but not the reverse.
 */
export function proves(a: Term, b: Term): boolean {
    const u = new Unifier();
    let target = whnf(b);
    while (target.k === 'all') {
        const x = u.enter(target.hint, target.ty);
        target = whnf(open(target, x));
    }
    return u.unify(u.instantiate(a), target) === undefined && u.unresolved().length === 0;
}
