import { certificate, instantiated, kernelTerm, Work } from './certificate';
import { Loc, UnilogError } from './errors';
import { checkProof, checkProposition, KernelError, KProof, KTerm } from './kernel';
import { isVariableName } from './parser';
import { Decl, PropExpr, ProofExpr } from './syntax';
import {
    all,
    app,
    arrow,
    atom,
    bound,
    generalize,
    imp,
    isLambdaVariable,
    lam,
    Mismatch,
    Printer,
    Rigid,
    Term,
    resetFills,
    Ty,
    Unifier,
    unknownsOf,
    Var,
    whnf,
} from './term';

/** A declaration after checking. */
export interface Declaration {
    readonly module: string;
    readonly name: string;
    readonly kind: 'axiom' | 'inference';
    /** What it proves: a closed proposition, its variables bound by "for every"s. */
    readonly prop: Term;
    /** Declarations its proof refers to directly (empty for axioms), as `Module.name` keys. */
    readonly uses: readonly string[];
    /** For inferences: how the proof was checked, step by step. */
    readonly proof?: Derivation;
    /** For inferences: whether the claim was written out (`name : P = proof`) rather than inferred. */
    readonly stated?: boolean;
    /** For inferences: the proof written out in full, as the kernel checked it (see kernel.ts). */
    readonly certificate?: KProof;
    readonly loc: Loc;
    /** Where the declaration's name is written. */
    readonly nameLoc: Loc;
}

/**
 * A checked proof, step by step. Every node records what it proves at that point in the proof, with
 * unknowns as the proof filled them in (read them with a `Printer` once checking is done).
 */
export type Derivation =
    /** A reference to a declaration, or to an assumption when `decl` is undefined. */
    | { readonly k: 'use'; readonly text: string; readonly term: Term; readonly loc: Loc; readonly decl?: Declaration }
    /** `fn arg` */
    | {
          readonly k: 'apply';
          readonly text: string;
          readonly term: Term;
          readonly loc: Loc;
          readonly fn: Derivation;
          readonly arg: Derivation;
      }
    /** `\param. body` */
    | {
          readonly k: 'assume';
          readonly text: string;
          readonly term: Term;
          readonly loc: Loc;
          readonly param: string;
          /** Where the assumption's name is written. */
          readonly paramLoc: Loc;
          readonly body: Derivation;
      };

/** A checked module: its declarations, all public. */
export interface Module {
    readonly name: string;
    readonly file: string;
    readonly imports: readonly string[];
    /** Where each import's module name is written, parallel to `imports`. */
    readonly importLocs: readonly Loc[];
    readonly decls: ReadonlyMap<string, Declaration>;
}

export const keyOf = (d: { module: string; name: string }) => `${d.module}.${d.name}`;

// --- Propositions: types, then terms ---

/** Types while inferring them: `t`, gaps, and not-yet-known types. */
type TyI = { k: 't' } | { k: 'fn'; from: TyI; to: TyI } | { k: 'tv'; ref?: TyI };

const T: TyI = { k: 't' };
const tv = (): TyI => ({ k: 'tv' });
const prunetv = (t: TyI): TyI => {
    while (t.k === 'tv' && t.ref) t = t.ref;
    return t;
};
const occursTy = (v: TyI, t: TyI): boolean => {
    t = prunetv(t);
    return t === v || (t.k === 'fn' && (occursTy(v, t.from) || occursTy(v, t.to)));
};
function unifyTy(a: TyI, b: TyI): 'ok' | 'clash' | 'self' {
    a = prunetv(a);
    b = prunetv(b);
    if (a === b) return 'ok';
    if (a.k === 'tv') {
        if (occursTy(a, b)) return 'self';
        a.ref = b;
        return 'ok';
    }
    if (b.k === 'tv') return unifyTy(b, a);
    if (a.k === 't' && b.k === 't') return 'ok';
    if (a.k === 'fn' && b.k === 'fn') {
        const r = unifyTy(a.from, b.from);
        return r === 'ok' ? unifyTy(a.to, b.to) : r;
    }
    return 'clash';
}
/** A finished type: anything still unknown is a proposition. */
const final = (t: TyI): Ty => {
    t = prunetv(t);
    return t.k === 'fn' ? arrow(final(t.from), final(t.to)) : 't';
};

/**
 * Turns proposition syntax into a closed term. Every variable must be bound by a lambda. A lambda where a
 * proposition is expected (a claim, either side of `=>`, a lambda's body) means "for every"; a lambda handed to
 * something, or applied, is a gap.
 *
 * Types are inferred only to rule out self-application: `X X` would allow Curry's paradox, so it is rejected.
 * Symbols (atoms) can be used at any type.
 */
export function propositionTerm(e: PropExpr): Term {
    const types = new Map<PropExpr, TyI>();
    const where = (x: PropExpr) => x.loc;

    const typeError = (r: 'clash' | 'self', x: PropExpr, what: string): never => {
        throw new UnilogError(
            r === 'self'
                ? `${what} is applied to itself, which is not allowed (it would make a paradox provable)`
                : `${what} is used inconsistently: as a proposition in one place and as something with a gap in another`,
            where(x),
        );
    };
    const describe = (x: PropExpr) => (x.k === 'var' || x.k === 'atom' ? `'${x.name}'` : 'This');

    // Pass 1: types of bound variables, recorded on each lambda.
    const asProp = (x: PropExpr, env: ReadonlyMap<string, TyI>): void => {
        if (x.k === 'lam') {
            const p = tv();
            types.set(x, p);
            asProp(x.body, new Map([...env, [x.param, p]]));
            return;
        }
        if (x.k === 'imp') {
            asProp(x.from, env);
            asProp(x.to, env);
            return;
        }
        const r = unifyTy(asValue(x, env), T);
        if (r !== 'ok') typeError(r, x, describe(x));
    };
    const asValue = (x: PropExpr, env: ReadonlyMap<string, TyI>): TyI => {
        switch (x.k) {
            case 'atom':
                return tv();
            case 'var': {
                const ty = env.get(x.name);
                if (!ty)
                    throw new UnilogError(
                        `'${x.name}' is not bound: write '\\${x.name}.' to say which proposition it varies over ` +
                            `(at the front means "for every ${x.name}")`,
                        x.loc,
                    );
                return ty;
            }
            case 'lam': {
                const p = tv();
                types.set(x, p);
                asProp(x.body, new Map([...env, [x.param, p]]));
                return { k: 'fn', from: p, to: T };
            }
            case 'imp':
                asProp(x, env);
                return T;
            case 'app': {
                const f = asValue(x.fn, env);
                const a = asValue(x.arg, env);
                const r = tv();
                const u = unifyTy(f, { k: 'fn', from: a, to: r });
                if (u !== 'ok') typeError(u, x, describe(x.fn));
                return r;
            }
        }
    };
    asProp(e, new Map());

    // Pass 2: the term. `scope` holds bound names, innermost last.
    const ty = (x: PropExpr) => final(types.get(x) ?? T);
    const index = (name: string, scope: readonly string[]) => scope.length - 1 - scope.lastIndexOf(name);
    const prop = (x: PropExpr, scope: readonly string[]): Term => {
        if (x.k === 'lam') return all(x.param, ty(x), prop(x.body, [...scope, x.param]));
        if (x.k === 'imp') return imp(prop(x.from, scope), prop(x.to, scope));
        return value(x, scope);
    };
    const value = (x: PropExpr, scope: readonly string[]): Term => {
        switch (x.k) {
            case 'atom':
                return atom(x.name);
            case 'var':
                return bound(index(x.name, scope));
            case 'lam':
                return lam(x.param, ty(x), prop(x.body, [...scope, x.param]));
            case 'imp':
                return prop(x, scope);
            case 'app':
                return app(value(x.fn, scope), value(x.arg, scope));
        }
    };
    return prop(e, []);
}

// --- Proofs ---

/** Prints proof syntax. Consecutive assumptions are written together: \F G X. body. */
const showProof = (e: ProofExpr): string => {
    switch (e.k) {
        case 'ref':
            return e.path.join('.');
        case 'app': {
            const arg = e.arg.k === 'ref' ? showProof(e.arg) : `(${showProof(e.arg)})`;
            const fn = e.fn.k === 'lam' ? `(${showProof(e.fn)})` : showProof(e.fn);
            return `${fn} ${arg}`;
        }
        case 'lam': {
            const params = [e.param];
            let body = e.body;
            while (body.k === 'lam') {
                params.push(body.param);
                body = body.body;
            }
            return `\\${params.join(' ')}. ${showProof(body)}`;
        }
    }
};

/** A checked step: the derivation (for display) and the work behind it (for the kernel). */
interface Done {
    readonly d: Derivation;
    readonly w: Work;
}

/** The proposition a declaration proves, in the kernel's form. */
export const kernelProposition = (d: Declaration): KTerm => kernelTerm(d.prop);

/** A proof that proves something other than what was needed at that point. Turned into a message by the caller. */
class Mismatched {
    constructor(
        readonly proved: Term,
        readonly needed: Term,
        readonly mismatch: Mismatch,
    ) {}
}

/**
 * Checks one module's declarations in order.
 *
 * `imported` maps each imported module's name to the checked module. Names resolve to, in order:
 * an assumption in scope, an earlier declaration in this module, then a declaration in exactly one imported
 * module. A qualified name `Mod.name` refers to `name` in the imported module `Mod`.
 *
 * Only earlier declarations are visible, so no proof can refer to itself: there is no recursion, every
 * proof is finite, and checking always terminates.
 */
export function checkModule(
    name: string,
    file: string,
    decls: readonly Decl[],
    imported: ReadonlyMap<string, Module>,
): Module {
    const local = new Map<string, Declaration>();
    const imports: string[] = [];
    const importLocs: Loc[] = [];
    /** All names this module declares, for clearer errors on forward references. */
    const declaredHere = new Set(decls.flatMap((d) => (d.k === 'import' ? [] : [d.name])));
    /** The declaration being checked, for a clearer error on self-reference. */
    let current = '';
    /** Matching state for the declaration being checked. */
    let u = new Unifier();

    for (const decl of decls) {
        if (decl.k === 'import') {
            const path = decl.path.join('.');
            if (local.size > 0) throw new UnilogError('Imports must come before declarations', decl.loc);
            if (imports.includes(path)) throw new UnilogError(`Module ${path} is imported twice`, decl.loc);
            imports.push(path);
            importLocs.push(decl.pathLoc);
            continue;
        }
        if (local.has(decl.name)) throw new UnilogError(`'${decl.name}' is already declared in this module`, decl.loc);
        current = decl.name;
        u = new Unifier();
        resetFills();

        if (decl.k === 'axiom') {
            const prop = propositionTerm(decl.prop);
            kernel(decl.name, decl.loc, () => checkProposition(kernelTerm(prop)));
            local.set(decl.name, {
                module: name,
                name: decl.name,
                kind: 'axiom',
                prop,
                uses: [],
                loc: decl.loc,
                nameLoc: decl.nameLoc,
            });
            continue;
        }

        const uses = new Set<string>();
        let done: Done;
        let proves: Term;
        if (decl.prop) {
            const claim = propositionTerm(decl.prop);
            try {
                done = check(decl.proof, claim, new Map(), uses);
            } catch (e) {
                if (!(e instanceof Mismatched)) throw e;
                const p = new Printer();
                throw new UnilogError(
                    `The proof does not prove the stated claim.\n` +
                        `  claimed: ${p.print(e.needed)}\n` +
                        `  proved:  ${p.print(e.proved)}\n` +
                        explainMismatch(p, e.mismatch),
                    decl.loc,
                );
            }
            proves = claim;
        } else {
            done = infer(decl.proof, new Map(), uses);
            proves = done.d.term;
        }
        undetermined(decl.loc);
        // An inferred claim is closed over what is left unknown; the certificate proves it for each.
        const generalized = decl.prop ? [] : unknownsOf(proves);
        const prop = decl.prop ? proves : generalize(proves);
        const cert = certificate(done.w, generalized);
        kernel(decl.name, decl.loc, () => checkProof(cert, kernelTerm(prop), kernelEnv));
        local.set(decl.name, {
            module: name,
            name: decl.name,
            kind: 'inference',
            prop,
            uses: [...uses],
            proof: done.d,
            stated: decl.prop !== undefined,
            certificate: cert,
            loc: decl.loc,
            nameLoc: decl.nameLoc,
        });
    }

    return { name, file, imports, importLocs, decls: local };

    /** The propositions the kernel may cite: every declaration this module can see, by `Module.name`. */
    function kernelEnv(key: string): KTerm | undefined {
        const at = key.lastIndexOf('.');
        const mod = key.slice(0, at);
        const d = mod === name ? local.get(key.slice(at + 1)) : imported.get(mod)?.decls.get(key.slice(at + 1));
        return d && kernelProposition(d);
    }

    /** Runs a kernel check. The checker accepted the declaration, so a rejection is a bug in the checker. */
    function kernel(declName: string, loc: Loc, run: () => void): void {
        try {
            run();
        } catch (e) {
            if (!(e instanceof KernelError)) throw e;
            throw new UnilogError(
                `Internal error: the proof kernel rejected '${declName}' (${e.message}). ` +
                    `This is a bug in Unilog's checker, not in your proof; please report it.`,
                loc,
            );
        }
    }

    /** Reports gaps the checker could not work out. */
    function undetermined(loc: Loc): void {
        const [first] = u.unresolved();
        if (!first) return;
        const p = new Printer();
        const call = [first.head, ...first.args].map((t) => p.print(t)).join(' ');
        throw new UnilogError(
            `Cannot tell where the gap is in ${p.print(first.head)}: ${call} must be ${p.print(first.rhs)}, ` +
                `and more than one gap would fit. State the claim, so the checker doesn't have to guess.`,
            loc,
        );
    }

    /** Matches what a proof proved against what was needed there, or throws. */
    function expect(proved: Term, needed: Term): void {
        const m = u.unify(needed, proved);
        if (m) throw new Mismatched(proved, needed, m);
    }

    /** Resolves a name to an assumption or a declaration, with its leading "for every"s instantiated. */
    function lookup(
        e: ProofExpr & { k: 'ref' },
        env: ReadonlyMap<string, Term>,
        uses: Set<string>,
        instantiate = true,
    ): { term: Term; decl?: Declaration; w: Work } {
        const use = (d: Declaration) => {
            uses.add(keyOf(d));
            const w: Work = { k: 'decl', key: keyOf(d) };
            if (!instantiate) return { term: d.prop, decl: d, w };
            const { term, vars } = u.specialize(d.prop);
            return { term, decl: d, w: instantiated(w, vars) };
        };
        if (e.path.length === 1) {
            const id = e.path[0] as string;
            // In a proof, a variable (uppercase) is an assumption, and anything else is a declaration.
            if (isVariableName(id)) {
                const assumed = env.get(id);
                if (!assumed)
                    throw new UnilogError(
                        `'${id}' is not an assumption in scope (assumptions are introduced with '\\${id}. ...')`,
                        e.loc,
                    );
                const w: Work = { k: 'hyp', name: id };
                if (!instantiate) return { term: assumed, w };
                const { term, vars } = u.specialize(assumed);
                return { term, w: instantiated(w, vars) };
            }
            const mine = local.get(id);
            if (mine) return use(mine);
            const found = imports.flatMap((m) => {
                const d = imported.get(m)?.decls.get(id);
                return d ? [d] : [];
            });
            if (found.length > 1)
                throw new UnilogError(
                    `'${id}' is ambiguous: it is declared in ${found.map((d) => d.module).join(' and ')}. ` +
                        `Write ${found.map((d) => `${d.module}.${id}`).join(' or ')}.`,
                    e.loc,
                );
            const [d] = found;
            if (!d) {
                if (id === current)
                    throw new UnilogError(`'${id}' cannot be used in its own proof (no recursion)`, e.loc);
                if (declaredHere.has(id))
                    throw new UnilogError(
                        `'${id}' is declared later in this module; declarations can only use earlier ones`,
                        e.loc,
                    );
                throw new UnilogError(`'${id}' is not declared in this module or in any module it imports`, e.loc);
            }
            return use(d);
        }
        const mod = e.path.slice(0, -1).join('.');
        const id = e.path[e.path.length - 1] as string;
        if (!imports.includes(mod)) throw new UnilogError(`Module ${mod} is not imported`, e.loc);
        const d = imported.get(mod)?.decls.get(id);
        if (!d) throw new UnilogError(`Module ${mod} has no declaration '${id}'`, e.loc);
        return use(d);
    }

    /** Works out what a proof proves. */
    function infer(e: ProofExpr, env: ReadonlyMap<string, Term>, uses: Set<string>): Done {
        const text = showProof(e);
        switch (e.k) {
            case 'ref': {
                const { w, ...found } = lookup(e, env, uses);
                return { d: { k: 'use', text, loc: e.loc, ...found }, w };
            }
            case 'lam': {
                const assumed = u.fresh();
                const body = infer(e.body, new Map([...env, [e.param, assumed]]), uses);
                return {
                    d: {
                        k: 'assume',
                        text,
                        term: imp(assumed, body.d.term),
                        loc: e.loc,
                        param: e.param,
                        paramLoc: e.paramLoc,
                        body: body.d,
                    },
                    w: { k: 'lam', name: e.param, prop: assumed, body: body.w },
                };
            }
            case 'app':
                return applied(e, env, uses);
        }
    }

    /**
     * Checks that a proof proves `needed`. A "for every X" in `needed` is proved for a fixed symbol X about which
     * nothing is known; an assumption takes its proposition from `needed`, so it can be universal.
     */
    function check(e: ProofExpr, needed: Term, env: ReadonlyMap<string, Term>, uses: Set<string>): Done {
        const target = whnf(needed);
        if (target.k === 'all' && e.k === 'ref') {
            // A universal needed, and a name that proves a universal: compare them as they are first, binder
            // against binder. Instantiating first would lose how the two variables correspond.
            const found = lookup(e, env, uses, false);
            const mark = u.mark();
            if (u.unify(target, found.term) === undefined)
                return {
                    d: {
                        k: 'use',
                        text: showProof(e),
                        loc: e.loc,
                        term: target,
                        ...(found.decl && { decl: found.decl }),
                    },
                    w: found.w,
                };
            u.rollback(mark);
        }
        if (target.k === 'all') {
            const x: Rigid = u.enter(target.hint, target.ty);
            try {
                const inner = check(e, whnf(openAll(target, x)), env, uses);
                return { d: inner.d, w: { k: 'gen', fixed: x, body: inner.w } };
            } finally {
                u.leave();
            }
        }
        if (e.k === 'lam') {
            let from: Term;
            let to: Term;
            if (target.k === 'imp') ({ from, to } = target);
            else {
                from = u.fresh();
                to = u.fresh();
                const m = u.unify(target, imp(from, to));
                if (m) throw new Mismatched(imp(from, to), target, m);
            }
            const body = check(e.body, to, new Map([...env, [e.param, from]]), uses);
            return {
                d: {
                    k: 'assume',
                    text: showProof(e),
                    term: imp(from, body.d.term),
                    loc: e.loc,
                    param: e.param,
                    paramLoc: e.paramLoc,
                    body: body.d,
                },
                w: { k: 'lam', name: e.param, prop: from, body: body.w },
            };
        }
        if (e.k === 'app') return applied(e, env, uses, target);
        const r = infer(e, env, uses);
        expect(r.d.term, target);
        return r;
    }

    /**
     * `f a b ...`: works out f, then takes each argument in turn as a premise. When the result is known (`needed`),
     * it is matched first: that settles gaps the arguments alone would leave open, as in `exists_intro`.
     */
    function applied(
        e: ProofExpr & { k: 'app' },
        env: ReadonlyMap<string, Term>,
        uses: Set<string>,
        needed?: Term,
    ): Done {
        const calls: (ProofExpr & { k: 'app' })[] = [];
        let head: ProofExpr = e;
        while (head.k === 'app') {
            calls.unshift(head);
            head = head.fn;
        }
        const headDone = infer(head, env, uses);
        const headD = headDone.d;
        let current = headD.term;
        const premises: { needs: Term; gives: Term; vars: Var[] }[] = [];
        for (const call of calls) {
            const { term: t, vars } = u.specialize(current);
            let from: Term;
            let to: Term;
            if (t.k === 'imp') ({ from, to } = t);
            else {
                from = u.fresh();
                to = u.fresh();
                const m = u.unify(t, imp(from, to));
                if (m) {
                    const p = new Printer();
                    throw new UnilogError(
                        `Cannot apply ${showProof(call.fn)} to ${showProof(call.arg)}: ` +
                            `${showProof(call.fn)} proves ${p.print(t)}, which is not an implication`,
                        call.loc,
                    );
                }
            }
            premises.push({ needs: from, gives: to, vars });
            current = to;
        }
        let last: Var[] = [];
        if (needed) {
            const s = u.specialize(current);
            last = s.vars;
            expect(s.term, needed);
        }

        let fnD = headD;
        let fnW = headDone.w;
        calls.forEach((call, i) => {
            const { needs, gives, vars } = premises[i] as { needs: Term; gives: Term; vars: Var[] };
            let argD: Done;
            // Printed before checking the argument, so a message shows the rule as it was.
            const p = new Printer();
            const before = p.print(fnD.term);
            try {
                argD = check(call.arg, needs, env, uses);
            } catch (err) {
                if (!(err instanceof Mismatched)) throw err;
                throw new UnilogError(
                    `Cannot apply ${showProof(call.fn)} to ${showProof(call.arg)}.\n` +
                        `  ${showProof(call.fn)} proves: ${before}\n` +
                        `  ${showProof(call.arg)} proves: ${p.print(err.proved)}\n` +
                        explainMismatch(p, err.mismatch),
                    call.loc,
                );
            }
            fnD = { k: 'apply', text: showProof(call), term: gives, loc: call.loc, fn: fnD, arg: argD.d };
            fnW = { k: 'app', fn: instantiated(fnW, vars), arg: argD.w };
        });
        if (!needed) {
            const s = u.specialize(fnD.term);
            last = s.vars;
            fnD = { ...fnD, term: s.term };
        }
        return { d: fnD, w: instantiated(fnW, last) };
    }
}

const openAll = (t: Term & { k: 'all' }, x: Term) => whnf(app(lam(t.hint, t.ty, t.body), x));

function explainMismatch(p: Printer, m: Mismatch): string {
    if (m.occurs) return `  ${p.print(m.a)} would have to contain itself: ${p.print(m.a)} = ${p.print(m.b)}`;
    if (m.escape)
        return isLambdaVariable(m.escape)
            ? `  ${p.print(m.a)} can't depend on ${p.print(m.escape)}, the variable of a lambda it is outside of: ` +
                  `${p.print(m.a)} = ${p.print(m.b)}`
            : `  ${p.print(m.escape)} stands for every ${m.escape.hint || 'value'} here, so it can't be part of ` +
                  `${p.print(m.a)}, which is about something outside: ${p.print(m.a)} = ${p.print(m.b)}`;
    if (m.type) return `  ${p.print(m.a)} does not match ${p.print(m.b)}: one is a proposition and the other has a gap`;
    const describe = (t: Term) => {
        const w = whnf(t);
        return w.k === 'rigid'
            ? `${p.print(w)} (stands for every ${w.hint || 'value'}, so it must stay general)`
            : p.print(t);
    };
    return `  ${describe(m.a)} does not match ${describe(m.b)}`;
}
