/**
 * Turns the checker's record of a proof (`Work`) into a certificate for the kernel (kernel.ts): every unknown
 * the checker filled in is replaced by what it was filled with, every fixed symbol becomes a reference to the
 * `gen` that introduced it, and every assumption becomes an index.
 */
import { KProof, KTerm } from './kernel';
import { norm, Rigid, Term, Ty, Var } from './term';

/** What the checker did, step by step, with the unknowns and fixed symbols it used. */
export type Work =
    | { readonly k: 'decl'; readonly key: string }
    | { readonly k: 'hyp'; readonly name: string }
    | { readonly k: 'lam'; readonly name: string; readonly prop: Term; readonly body: Work }
    | { readonly k: 'app'; readonly fn: Work; readonly arg: Work }
    | { readonly k: 'gen'; readonly fixed: Rigid; readonly body: Work }
    | { readonly k: 'inst'; readonly of: Work; readonly value: Term };

/** Instantiates `w` with each unknown in turn. */
export const instantiated = (w: Work, vars: readonly Var[]): Work =>
    vars.reduce<Work>((of, value) => ({ k: 'inst', of, value }), w);

/** Something a `gen` introduced: a fixed symbol, or an unknown left over and bound at the front of the claim. */
type Fixed = Rigid | Var;

/** A term with nothing left to fill in. Unknowns nobody constrained are filled with an arbitrary value. */
export function kernelTerm(t: Term, gens: readonly Fixed[] = []): KTerm {
    const go = (u: Term): KTerm => {
        switch (u.k) {
            case 'atom':
                return { k: 'atom', name: u.name };
            case 'bound':
                return { k: 'bvar', i: u.i };
            case 'app':
                return { k: 'app', fn: go(u.fn), arg: go(u.arg) };
            case 'imp':
                return { k: 'imp', from: go(u.from), to: go(u.to) };
            case 'lam':
            case 'all':
                return { k: u.k, ty: u.ty, body: go(u.body) };
            case 'rigid':
            case 'var': {
                let i = gens.length - 1;
                while (i >= 0 && gens[i] !== u && !(gens[i]?.k === 'rigid' && u.k === 'rigid' && gens[i]?.id === u.id))
                    i--;
                if (i >= 0) return { k: 'gvar', i: gens.length - 1 - i };
                if (u.k === 'var') return arbitrary(u.ty);
                throw new Error(`Internal error: fixed symbol ${u.hint} is used outside its scope`);
            }
        }
    };
    return go(norm(t));
}

/** Any value of a type: an unknown nothing constrained can be anything. */
const arbitrary = (ty: Ty): KTerm =>
    ty === 't' ? { k: 'atom', name: '_' } : { k: 'lam', ty: ty.from, body: arbitrary(ty.to) };

/**
 * The certificate for a proof. `generalized` are the unknowns bound at the front of an inferred claim, in
 * order: the certificate proves the claim for a fixed symbol for each.
 */
export function certificate(work: Work, generalized: readonly Var[] = []): KProof {
    const go = (w: Work, gens: readonly Fixed[], hyps: readonly string[]): KProof => {
        switch (w.k) {
            case 'decl':
                return { k: 'decl', name: w.key };
            case 'hyp': {
                const i = hyps.lastIndexOf(w.name);
                if (i < 0) throw new Error(`Internal error: assumption ${w.name} is not in scope`);
                return { k: 'hyp', i: hyps.length - 1 - i };
            }
            case 'lam':
                return { k: 'lam', prop: kernelTerm(w.prop, gens), body: go(w.body, gens, [...hyps, w.name]) };
            case 'app':
                return { k: 'app', fn: go(w.fn, gens, hyps), arg: go(w.arg, gens, hyps) };
            case 'gen':
                return { k: 'gen', ty: w.fixed.ty, body: go(w.body, [...gens, w.fixed], hyps) };
            case 'inst':
                return { k: 'inst', of: go(w.of, gens, hyps), term: kernelTerm(w.value, gens) };
        }
    };
    const body = go(work, generalized, []);
    return generalized.reduceRight<KProof>((inner, v) => ({ k: 'gen', ty: v.ty, body: inner }), body);
}
