import { Loc } from './errors';

/** Proposition syntax, before variables are turned into terms. */
export type PropExpr =
    | { k: 'atom'; name: string; loc: Loc }
    | { k: 'var'; name: string; loc: Loc }
    | { k: 'app'; fn: PropExpr; arg: PropExpr; loc: Loc }
    | { k: 'imp'; from: PropExpr; to: PropExpr; loc: Loc };

/** Proof syntax. */
export type ProofExpr =
    /** A reference to a declaration or an assumption. `path` has more than one segment when qualified (`Mod.name`). */
    | { k: 'ref'; path: string[]; loc: Loc }
    /** Applying a proof to another: `b a`. */
    | { k: 'app'; fn: ProofExpr; arg: ProofExpr; loc: Loc }
    /** Assuming a proof: `p => body` proves `A => B` if `body` proves B given p proves A. */
    | { k: 'lam'; param: string; body: ProofExpr; loc: Loc };

export type Decl =
    /** `pathLoc` is the module name's location. */
    | { k: 'import'; path: string[]; loc: Loc; pathLoc: Loc }
    /** `name : P.` A stated belief. `nameLoc` is the name's location. */
    | { k: 'axiom'; name: string; prop: PropExpr; loc: Loc; nameLoc: Loc }
    /** `name = proof.` or `name : P = proof.` */
    | { k: 'theorem'; name: string; prop?: PropExpr; proof: ProofExpr; loc: Loc; nameLoc: Loc };
