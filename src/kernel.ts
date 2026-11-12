/**
 * The proof kernel: a small, independent re-check of every proof.
 *
 * The checker (check.ts, term.ts) does the clever work: it fills in unknowns, finds gaps and works out what a
 * proof proves. Clever code can have bugs. So after the checker accepts a declaration, it writes the proof
 * out in full, with everything it filled in made explicit (a *certificate*), and this kernel checks that
 * certificate step by step. The kernel never searches, guesses or fills anything in: it only substitutes and
 * compares. To trust Unilog's verdicts, this file is the only code you need to read.
 *
 * It shares no code with the checker, so a bug in one cannot hide in the other.
 *
 * The rules (natural deduction):
 *
 *   decl n          the proposition declaration n was accepted with
 *   hyp i           the i-th enclosing assumption (0 = innermost)
 *   lam P b         assume P, prove B with b:          proves P => B
 *   app f a         f proves A => B, a proves A:       proves B
 *   gen b           b proves P for a fresh symbol x:   proves "for every X, P"
 *   inst p t        p proves "for every X, P":         proves P with t for X
 */

// --- Terms ---

/** Types: `t` for propositions and symbols, `A -> B` for something with a gap. */
export type KType = 't' | { readonly from: KType; readonly to: KType };

/**
 * Terms, with bound variables as de Bruijn indices (`bvar 0` is the nearest enclosing lam or all).
 * `gvar i` refers to the i-th enclosing `gen` of the certificate (0 = innermost): a fixed symbol.
 */
export type KTerm =
    | { readonly k: 'atom'; readonly name: string }
    | { readonly k: 'bvar'; readonly i: number }
    | { readonly k: 'gvar'; readonly i: number }
    | { readonly k: 'app'; readonly fn: KTerm; readonly arg: KTerm }
    | { readonly k: 'imp'; readonly from: KTerm; readonly to: KTerm }
    /** A gap: `\X. body`, handed to something. */
    | { readonly k: 'lam'; readonly ty: KType; readonly body: KTerm }
    /** "For every X, body". */
    | { readonly k: 'all'; readonly ty: KType; readonly body: KTerm };

/** A certificate: a proof with everything explicit. */
export type KProof =
    | { readonly k: 'decl'; readonly name: string }
    | { readonly k: 'hyp'; readonly i: number }
    | { readonly k: 'lam'; readonly prop: KTerm; readonly body: KProof }
    | { readonly k: 'app'; readonly fn: KProof; readonly arg: KProof }
    | { readonly k: 'gen'; readonly ty: KType; readonly body: KProof }
    | { readonly k: 'inst'; readonly of: KProof; readonly term: KTerm };

/** Terms inside the kernel: as KTerm, plus `fixed` (a fixed symbol, by unique id) in place of `gvar` indices. */
type Term =
    | { readonly k: 'atom'; readonly name: string }
    | { readonly k: 'bvar'; readonly i: number }
    | { readonly k: 'gvar'; readonly i: number }
    | { readonly k: 'fixed'; readonly id: number }
    | { readonly k: 'app'; readonly fn: Term; readonly arg: Term }
    | { readonly k: 'imp'; readonly from: Term; readonly to: Term }
    | { readonly k: 'lam'; readonly ty: KType; readonly body: Term }
    | { readonly k: 'all'; readonly ty: KType; readonly body: Term };

export class KernelError extends Error {}

const fail = (why: string): never => {
    throw new KernelError(why);
};

// --- Substitution (de Bruijn) ---

/** Adds d to every bound index of at least `from`. */
function shift(t: Term, d: number, from = 0): Term {
    switch (t.k) {
        case 'bvar':
            return t.i >= from ? { k: 'bvar', i: t.i + d } : t;
        case 'app':
            return { k: 'app', fn: shift(t.fn, d, from), arg: shift(t.arg, d, from) };
        case 'imp':
            return { k: 'imp', from: shift(t.from, d, from), to: shift(t.to, d, from) };
        case 'lam':
        case 'all':
            return { ...t, body: shift(t.body, d, from + 1) };
        default:
            return t;
    }
}

/** t with s for bound index j, removing that binder. */
function subst(t: Term, j: number, s: Term): Term {
    switch (t.k) {
        case 'bvar':
            return t.i === j ? shift(s, j) : t.i > j ? { k: 'bvar', i: t.i - 1 } : t;
        case 'app':
            return { k: 'app', fn: subst(t.fn, j, s), arg: subst(t.arg, j, s) };
        case 'imp':
            return { k: 'imp', from: subst(t.from, j, s), to: subst(t.to, j, s) };
        case 'lam':
        case 'all':
            return { ...t, body: subst(t.body, j + 1, s) };
        default:
            return t;
    }
}

/** t with the fixed symbol `id` replaced by bound index `depth` (making it the variable of a new binder). */
function abstract(t: Term, id: number, depth = 0): Term {
    switch (t.k) {
        case 'fixed':
            return t.id === id ? { k: 'bvar', i: depth } : t;
        case 'bvar':
            return t.i >= depth ? { k: 'bvar', i: t.i + 1 } : t;
        case 'app':
            return { k: 'app', fn: abstract(t.fn, id, depth), arg: abstract(t.arg, id, depth) };
        case 'imp':
            return { k: 'imp', from: abstract(t.from, id, depth), to: abstract(t.to, id, depth) };
        case 'lam':
        case 'all':
            return { ...t, body: abstract(t.body, id, depth + 1) };
        default:
            return t;
    }
}

function mentions(t: Term, test: (u: Term) => boolean): boolean {
    if (test(t)) return true;
    switch (t.k) {
        case 'app':
            return mentions(t.fn, test) || mentions(t.arg, test);
        case 'imp':
            return mentions(t.from, test) || mentions(t.to, test);
        case 'lam':
        case 'all':
            return mentions(t.body, test);
        default:
            return false;
    }
}

/** Whether bound index i (counted at t's top) occurs in t. */
function usesBound(t: Term, i: number): boolean {
    switch (t.k) {
        case 'bvar':
            return t.i === i;
        case 'app':
            return usesBound(t.fn, i) || usesBound(t.arg, i);
        case 'imp':
            return usesBound(t.from, i) || usesBound(t.to, i);
        case 'lam':
        case 'all':
            return usesBound(t.body, i + 1);
        default:
            return false;
    }
}

/**
 * Normal form: every gap applied to something is filled, and `\X. f X` is written `f`. Only called on terms
 * that passed `typeOf`, which guarantees it finishes.
 */
function normalize(t: Term): Term {
    switch (t.k) {
        case 'app': {
            const fn = normalize(t.fn);
            const arg = normalize(t.arg);
            return fn.k === 'lam' ? normalize(subst(fn.body, 0, arg)) : { k: 'app', fn, arg };
        }
        case 'imp':
            return { k: 'imp', from: normalize(t.from), to: normalize(t.to) };
        case 'all':
            return { ...t, body: normalize(t.body) };
        case 'lam': {
            const body = normalize(t.body);
            if (body.k === 'app' && body.arg.k === 'bvar' && body.arg.i === 0 && !usesBound(body.fn, 0))
                return shift(body.fn, -1);
            return { ...t, body };
        }
        default:
            return t;
    }
}

const sameType = (a: KType, b: KType): boolean =>
    a === 't' || b === 't' ? a === b : sameType(a.from, b.from) && sameType(a.to, b.to);

/** Identical normal forms. Bound variables are indices, so renaming them never matters. */
function same(a: Term, b: Term): boolean {
    switch (a.k) {
        case 'atom':
            return b.k === 'atom' && a.name === b.name;
        case 'bvar':
            return b.k === 'bvar' && a.i === b.i;
        case 'fixed':
            return b.k === 'fixed' && a.id === b.id;
        case 'app':
            return b.k === 'app' && same(a.fn, b.fn) && same(a.arg, b.arg);
        case 'imp':
            return b.k === 'imp' && same(a.from, b.from) && same(a.to, b.to);
        case 'lam':
        case 'all':
            return b.k === a.k && sameType(a.ty, b.ty) && same(a.body, b.body);
        default:
            return false;
    }
}

// --- Types ---

/** Types while checking: atoms may be used at any type, so their types are worked out per occurrence. */
type Ty = { k: 't' } | { k: 'fn'; from: Ty; to: Ty } | { k: 'var'; ref?: Ty };

const toTy = (t: KType): Ty => (t === 't' ? { k: 't' } : { k: 'fn', from: toTy(t.from), to: toTy(t.to) });

function resolveTy(t: Ty): Ty {
    while (t.k === 'var' && t.ref) t = t.ref;
    return t;
}

function occursTy(v: Ty, t: Ty): boolean {
    t = resolveTy(t);
    return t === v || (t.k === 'fn' && (occursTy(v, t.from) || occursTy(v, t.to)));
}

function unifyTy(a: Ty, b: Ty): boolean {
    a = resolveTy(a);
    b = resolveTy(b);
    if (a === b) return true;
    if (a.k === 'var') {
        if (occursTy(a, b)) return false;
        a.ref = b;
        return true;
    }
    if (b.k === 'var') return unifyTy(b, a);
    if (a.k === 't' && b.k === 't') return true;
    return a.k === 'fn' && b.k === 'fn' && unifyTy(a.from, b.from) && unifyTy(a.to, b.to);
}

/**
 * The type of a term, failing if it has none. In particular, nothing can be applied to itself: that would
 * allow Curry's paradox. `fixed` gives the types of the fixed symbols in scope.
 */
function typeOf(t: Term, fixed: ReadonlyMap<number, KType>, bound: readonly Ty[] = []): Ty {
    switch (t.k) {
        case 'atom':
            return { k: 'var' };
        case 'bvar':
            return bound[bound.length - 1 - t.i] ?? fail(`bound variable ${t.i} has no binder`);
        case 'gvar':
            return fail('a fixed symbol was not resolved');
        case 'fixed': {
            const ty = fixed.get(t.id);
            return ty ? toTy(ty) : fail('a fixed symbol is used outside its "for every"');
        }
        case 'app': {
            const result: Ty = { k: 'var' };
            const fn = typeOf(t.fn, fixed, bound);
            const arg = typeOf(t.arg, fixed, bound);
            return unifyTy(fn, { k: 'fn', from: arg, to: result }) ? result : fail('an application does not type');
        }
        case 'imp':
            isProposition(t.from, fixed, bound);
            isProposition(t.to, fixed, bound);
            return { k: 't' };
        case 'lam':
            return { k: 'fn', from: toTy(t.ty), to: typeOf(t.body, fixed, [...bound, toTy(t.ty)]) };
        case 'all':
            isProposition(t.body, fixed, [...bound, toTy(t.ty)]);
            return { k: 't' };
    }
}

function isProposition(t: Term, fixed: ReadonlyMap<number, KType>, bound: readonly Ty[] = []): void {
    if (!unifyTy(typeOf(t, fixed, bound), { k: 't' })) fail('expected a proposition, found something with a gap');
}

// --- Checking ---

let nextId = 0;

/** What the kernel knows at a point in a certificate: assumptions and fixed symbols in scope, innermost last. */
interface Scope {
    readonly hyps: readonly Term[];
    readonly gens: readonly { id: number; ty: KType }[];
}

/** A certificate's term, with its `gvar`s resolved to the fixed symbols in scope. */
function resolve(t: Term, scope: Scope): Term {
    switch (t.k) {
        case 'gvar': {
            const g = scope.gens[scope.gens.length - 1 - t.i];
            return g ? { k: 'fixed', id: g.id } : fail(`fixed symbol ${t.i} is not in scope`);
        }
        case 'app':
            return { k: 'app', fn: resolve(t.fn, scope), arg: resolve(t.arg, scope) };
        case 'imp':
            return { k: 'imp', from: resolve(t.from, scope), to: resolve(t.to, scope) };
        case 'lam':
        case 'all':
            return { ...t, body: resolve(t.body, scope) };
        default:
            return t;
    }
}

const fixedTypes = (scope: Scope) => new Map(scope.gens.map((g) => [g.id, g.ty]));

/** What a certificate proves, in normal form. Fails if any step is not a correct use of its rule. */
function proves(p: KProof, scope: Scope, env: (name: string) => KTerm | undefined): Term {
    switch (p.k) {
        case 'decl':
            return normalize(env(p.name) ?? fail(`unknown declaration ${p.name}`));

        case 'hyp':
            return scope.hyps[scope.hyps.length - 1 - p.i] ?? fail(`assumption ${p.i} is not in scope`);

        case 'lam': {
            const prop = resolve(p.prop, scope);
            isProposition(prop, fixedTypes(scope));
            const assumed = normalize(prop);
            const body = proves(p.body, { ...scope, hyps: [...scope.hyps, assumed] }, env);
            return { k: 'imp', from: assumed, to: body };
        }

        case 'app': {
            const fn = proves(p.fn, scope, env);
            if (fn.k !== 'imp') return fail('applying something that is not an implication');
            const arg = proves(p.arg, scope, env);
            return same(fn.from, arg) ? fn.to : fail("the argument does not prove the function's premise");
        }

        case 'gen': {
            // A fresh symbol: nothing in scope can mention it, since it did not exist before.
            const id = nextId++;
            const body = proves(p.body, { ...scope, gens: [...scope.gens, { id, ty: p.ty }] }, env);
            return { k: 'all', ty: p.ty, body: abstract(body, id) };
        }

        case 'inst': {
            const general = proves(p.of, scope, env);
            if (general.k !== 'all') return fail('instantiating something that is not a "for every"');
            const value = resolve(p.term, scope);
            if (!unifyTy(typeOf(value, fixedTypes(scope)), toTy(general.ty)))
                fail('an instantiation has the wrong type');
            return normalize(subst(general.body, 0, value));
        }
    }
}

/** Checks that a declared proposition is closed and well typed, as an axiom must be. */
export function checkProposition(prop: KTerm): void {
    const scope: Scope = { hyps: [], gens: [] };
    isProposition(resolve(prop, scope), new Map());
}

/**
 * Checks that `certificate` proves exactly `claim`, using the declarations `env` knows (each already
 * checked). Throws a KernelError if not.
 */
export function checkProof(certificate: KProof, claim: KTerm, env: (name: string) => KTerm | undefined): void {
    checkProposition(claim);
    const proved = proves(certificate, { hyps: [], gens: [] }, env);
    if (!same(proved, normalize(resolve(claim, { hyps: [], gens: [] }))))
        fail('the certificate proves something other than the claim');
    if (mentions(proved, (t) => t.k === 'fixed')) fail('a fixed symbol escaped its "for every"');
}
