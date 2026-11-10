import { Loc, UnilogError } from './errors';
import { Decl, PropExpr, ProofExpr } from './syntax';
import { app, atom, fresh, generalize, imp, instantiate, Printer, rigid, Scheme, Term, unify, Var } from './term';

/** A declaration after checking. */
export interface Declaration {
    readonly module: string;
    readonly name: string;
    readonly kind: 'axiom' | 'theorem';
    /** What it proves, quantified over its variables. */
    readonly scheme: Scheme;
    /** Declarations its proof refers to directly (empty for axioms), as `Module.name` keys. */
    readonly uses: readonly string[];
    readonly loc: Loc;
}

/** A checked module: its declarations, all public. */
export interface Module {
    readonly name: string;
    readonly file: string;
    readonly imports: readonly string[];
    readonly decls: ReadonlyMap<string, Declaration>;
}

export const keyOf = (d: { module: string; name: string }) => `${d.module}.${d.name}`;

/** Turns proposition syntax into a term. Variables with the same name in one declaration are the same variable. */
function toTerm(e: PropExpr, vars: Map<string, Term>, makeVar: (name: string) => Term): Term {
    switch (e.k) {
        case 'atom':
            return atom(e.name);
        case 'var': {
            if (e.name === '_') return fresh();
            let v = vars.get(e.name);
            if (!v) vars.set(e.name, (v = makeVar(e.name)));
            return v;
        }
        case 'app':
            return app(toTerm(e.fn, vars, makeVar), toTerm(e.arg, vars, makeVar));
        case 'imp':
            return imp(toTerm(e.from, vars, makeVar), toTerm(e.to, vars, makeVar));
    }
}

const showProof = (e: ProofExpr): string => {
    switch (e.k) {
        case 'ref':
            return e.path.join('.');
        case 'app': {
            const arg = e.arg.k === 'ref' ? showProof(e.arg) : `(${showProof(e.arg)})`;
            const fn = e.fn.k === 'lam' ? `(${showProof(e.fn)})` : showProof(e.fn);
            return `${fn} ${arg}`;
        }
        case 'lam':
            return `${e.param} => ${showProof(e.body)}`;
    }
};

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

    for (const decl of decls) {
        if (decl.k === 'import') {
            const path = decl.path.join('.');
            if (local.size > 0) throw new UnilogError('Imports must come before declarations', decl.loc);
            if (imports.includes(path)) throw new UnilogError(`Module ${path} is imported twice`, decl.loc);
            imports.push(path);
            continue;
        }
        if (local.has(decl.name)) throw new UnilogError(`'${decl.name}' is already declared in this module`, decl.loc);

        const vars = new Map<string, Term>();
        if (decl.k === 'axiom') {
            const prop = toTerm(decl.prop, vars, (n) => fresh(n));
            local.set(decl.name, {
                module: name,
                name: decl.name,
                kind: 'axiom',
                scheme: generalize(prop),
                uses: [],
                loc: decl.loc,
            });
            continue;
        }

        const uses = new Set<string>();
        const inferred = infer(decl.proof, new Map(), uses);
        let proves: Term = inferred;
        if (decl.prop) {
            // The stated claim's variables are rigid: the proof must prove the claim for *every* value of them.
            const claim = toTerm(decl.prop, vars, (n) => rigid(n));
            const mismatch = unify(inferred, claim);
            if (mismatch) {
                const p = new Printer();
                throw new UnilogError(
                    `The proof does not prove the stated claim.\n` +
                        `  claimed: ${p.print(claim)}\n` +
                        `  proved:  ${p.print(inferred)}\n` +
                        explainMismatch(p, mismatch.a, mismatch.b, mismatch.occurs),
                    decl.loc,
                );
            }
            proves = claim;
        }
        local.set(decl.name, {
            module: name,
            name: decl.name,
            kind: 'theorem',
            scheme: generalize(proves),
            uses: [...uses],
            loc: decl.loc,
        });
    }

    return { name, file, imports, decls: local };

    function lookup(e: ProofExpr & { k: 'ref' }, env: ReadonlyMap<string, Var>, uses: Set<string>): Term {
        if (e.path.length === 1) {
            const id = e.path[0] as string;
            const assumed = env.get(id);
            if (assumed) return assumed;
            const mine = local.get(id);
            if (mine) {
                uses.add(keyOf(mine));
                return instantiate(mine.scheme);
            }
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
            if (!d) throw new UnilogError(`'${id}' is not declared (declarations can only use earlier ones)`, e.loc);
            uses.add(keyOf(d));
            return instantiate(d.scheme);
        }
        const mod = e.path.slice(0, -1).join('.');
        const id = e.path[e.path.length - 1] as string;
        if (!imports.includes(mod)) throw new UnilogError(`Module ${mod} is not imported`, e.loc);
        const d = imported.get(mod)?.decls.get(id);
        if (!d) throw new UnilogError(`Module ${mod} has no declaration '${id}'`, e.loc);
        uses.add(keyOf(d));
        return instantiate(d.scheme);
    }

    /** Infers what a proof proves. */
    function infer(e: ProofExpr, env: ReadonlyMap<string, Var>, uses: Set<string>): Term {
        switch (e.k) {
            case 'ref':
                return lookup(e, env, uses);
            case 'lam': {
                const assumed = fresh();
                const body = infer(e.body, new Map([...env, [e.param, assumed]]), uses);
                return imp(assumed, body);
            }
            case 'app': {
                const fn = infer(e.fn, env, uses);
                const arg = infer(e.arg, env, uses);
                const result = fresh();
                const p = new Printer();
                // Print before unifying, so the message shows the propositions as they were.
                const before = { fn: p.print(fn), arg: p.print(arg) };
                const mismatch = unify(fn, imp(arg, result));
                if (mismatch)
                    throw new UnilogError(
                        `Cannot apply ${showProof(e.fn)} to ${showProof(e.arg)}.\n` +
                            `  ${showProof(e.fn)} proves: ${before.fn}\n` +
                            `  ${showProof(e.arg)} proves: ${before.arg}\n` +
                            explainMismatch(p, mismatch.a, mismatch.b, mismatch.occurs),
                        e.loc,
                    );
                return result;
            }
        }
    }
}

function explainMismatch(p: Printer, a: Term, b: Term, occurs?: boolean): string {
    if (occurs) return `  ${p.print(a)} would have to contain itself: ${p.print(a)} = ${p.print(b)}`;
    const describe = (t: Term) =>
        t.k === 'rigid' ? `${p.print(t)} (a variable of the claim, so it must stay general)` : p.print(t);
    return `  ${describe(a)} does not match ${describe(b)}`;
}
