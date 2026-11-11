# The Unilog language

Unilog is a small constructive logic for stating beliefs and checking what follows from them. A proposition is a symbolic expression, and a proof of a proposition is a term that builds it from axioms. The checker infers what each proof proves and rejects anything that doesn't follow.

Under the hood it is the Curry–Howard correspondence with Hindley–Milner type inference: propositions are types, proofs are programs, and checking is type inference. You never write types or quantifiers. Uppercase names are variables, and the checker works out the rest.

## Propositions

```
likes jake cake            # application: ((likes jake) cake)
likes X cake               # X is a variable: "for any X"
likes X cake => likes X sugar
(A => B) => A => B         # => is right-associative
```

- **Atoms** start with a lowercase letter or digit: `jake`, `likes`, `0`. They are global symbols, so `cake` means the same thing in every package.
- **Variables** start with an uppercase letter or `_`: `X`, `Person`, `_`. Within one declaration, the same name is the same variable. Each `_` is a fresh one.
- **Application** is juxtaposition and is curried: `likes jake cake` is `likes` applied to `jake`, then to `cake`. A variable can stand for a partial application: `P X` matches `likes jake cake` with `P = likes jake` and `X = cake`.
- **Implication** is `=>`. A proof of `A => B` turns proofs of `A` into proofs of `B`.

## Declarations

Every declaration ends with `;`. A `.` has only two jobs: it joins the parts of a qualified name (`Kant.Ethics.duty`, no spaces around it), and it ends the head of an assumption (`\P. body`, as in the lambda calculus). `#` starts a comment.

```
a : likes jake cake;                      # axiom: a stated belief
c = b a;                                  # inference: what it proves is inferred
c : likes jake sugar = b a;               # inference with its claim stated and checked
import Kant.Ethics;                       # module import (before any declaration)
```

Declaration names start with a lowercase letter or digit.

**Variables are quantified once, around the whole declaration**, including variables that first appear in a later premise: `gift : likes X Y => has Z Y => gives Z X Y` means "for all X, Y and Z". A variable inside a premise that is itself an implication is bound by the declaration too: `(A => A) => b` means "for any particular A, given A => A, conclude b". A premise cannot have its own "for all".

**Axioms** are beliefs you state without proof. Variables in an axiom are universally quantified, and each use gets fresh copies:

```
b : likes X cake => likes X sugar;        # for every X
```

**Inferences** are proofs. A stated claim's variables must hold *in general*: `c : likes X sugar = b a;` is rejected, because `b a` only proves it for `jake`. A claim may be a special case of what the proof shows, and the declaration then proves exactly the claim. This is the same rule `unilog find` uses by default ("the proof proves the claim"). A claim is never checked by unification, which would let it claim more than the proof shows.

## Proofs

```
b a                     # apply b to a: a's proposition must match b's premise
\F G X. G (F X)       # assumptions: "assume F, G and X; then G (F X)"
Kant.Ethics.duty        # a qualified reference to an imported declaration
```

- **Application** `b a` unifies `a`'s proposition with `b`'s premise, and proves `b`'s conclusion with the resulting substitution.
- **Currying.** A rule with several premises takes them one at a time: given `gift : likes X Y => has Z Y => gives Z X Y`, `gift a` proves `has Z cake => gives Z jake cake`, an implication still waiting for its second premise. `gift a h` is the same as `(gift a) h`.
- **Assumption** `\P. body` proves `A => B` when `body` proves `B` using `P` as a proof of `A`: "assume P; then body". This is how you prove implications by hypothetical reasoning. What `P` must prove is inferred from how it is used: `and_comm = \P. pair (snd P) (fst P);` proves `and A B => and B A`.
  - Several at once: `\F G X. body` means `\F. \G. \X. body`.
  - An assumption is introduced by a single name, never a pattern. The proof it stands for can be used but not taken apart; that's what elimination rules like `fst`, `snd` and `case` are for.
  - `=>` always means implication. In a proof, the old forms `P => body` and `\P -> body` are errors that say to write `\P. body`, and an old `.` at the end of a declaration is an error that says to use `;`.
- **Names.** Uppercase means a variable everywhere. In a proof, an uppercase name is an assumption (introduced by an enclosing `\P.`), and a lowercase name is a declaration: an earlier one in the same module, or one in exactly one imported module. Use a qualified name (`Mod.name`) when two imports define the same name.

## What keeps it sound

- **No recursion.** A declaration can only use declarations before it, never itself. Every proof is finite, checking always terminates, and nothing can prove itself (`loop = loop;` is rejected).
- **Occurs check.** A proposition cannot be unified with something containing itself, so self-application (`\X. X X`) and fixed-point combinators like `\F. (\X. F (X X)) (\X. F (X X))` are rejected. Those are where untyped lambda calculus gets its paradoxes.
- **Assumptions are fixed, not general.** Inside its proof, an assumption stands for one particular proposition: it cannot be used as a proof of `a` in one place and of `b` in another. Only finished top-level declarations are general, and they have no open assumptions.
- **Assumptions stay inside their proof.** `P` can only be used inside the `\P. ...` that introduces it.

Formally, proofs are the simply typed lambda calculus with type variables, which by the Curry–Howard correspondence is natural deduction for intuitionistic implicational logic: an assumption `\P. body` is ⇒-introduction, and application `f a` is ⇒-elimination (modus ponens), plus your axioms. The proof terms are never run; the checker only infers what they prove.
- **Nothing private.** Every declaration in every module is public, so everything a claim rests on can be inspected and cited. `unilog axioms` lists the full set of axioms behind any declaration.
- **Logic lives in packages.** The core knows only `=>`. `and`, `or` and `false` are axioms in a package ([`examples/Logic/Prelude.ul`](../examples/Logic/Prelude.ul)). Excluded middle is not built in: a classical thinker states it as an axiom, and it then appears in the axiom list of everything that relies on it.

## Modules and projects

The module `Kant.Ethics` is the file `Kant/Ethics.ul` under a **search root**. An import is a module name, never a path, and every import (in every module, including dependencies) is looked up in the search roots of the project being checked. So a package's files work unchanged wherever they end up: as the project itself, or next to their own dependencies in a dependency folder.

A project is configured by a `unilog.json` file in its directory (like Agda's `.agda-lib`):

```json
{
    "roots": [".", "deps"],
    "check": ["Ethics"]
}
```

- `roots`: where modules are looked up, in order, relative to the project. Put your own source's parent directory here, along with wherever your dependencies are installed. Roots that don't exist yet are skipped.
- `check`: the folders (or `.ul` files) that `unilog check` checks. Modules they import are checked as far as they're used.

`unilog init` writes one: `unilog init --roots . deps --check Ethics`, or with no options a starting one (`"roots": ["."]`, `"check": []`) to fill in. `unilog check` refuses to run while `check` is empty.

Only the `unilog.json` of the project you run `unilog` in applies. One that comes with a dependency is ignored.

A directory without a `unilog.json` checks every `.ul` file in it, with the directory as its only root.

- Imports are not re-exported: a module sees only what it imports itself.
- Import cycles are errors, and so is a module found in more than one root.
- An error in an imported module says how it was reached, e.g. `(in Alice.Beliefs, imported by Bob.Rebuttal)`, and is reported once however many modules import it.

## Relations, not functions

There are no functions that compute. Write relations as rules instead:

```
znat : nat z;
snat : nat X => nat (s X);
ltz : nat Y => lt z (s Y);
lts : lt X Y => lt (s X) (s Y);

one_lt_two : lt (s z) (s (s z)) = lts (ltz znat);
```

This keeps the core tiny and sound. Recursive functions would allow `loop = loop`, a "proof" of everything.

## Limits, and how to go past them

Variables are quantified only at the outside of a declaration. You can say "for every X, ..." but not "if (for every X, ...) then ...". When an argument needs nested quantifiers, encode a quantified logic as a package, with `forall` and `exists` as ordinary symbols, object variables as data, and rules for substitution and "not free in" as relations:

```
forall_elim  : proves G (forall X P) => subst P X T Q => proves G Q;
forall_intro : proves G P => notfree X G => proves G (forall X P);
```

This is how Metamath builds mathematics from a core that does little more than substitution. Nothing in the core changes. It only makes proofs longer, which is a job for proof-search tools.

## Analysis

Unilog provides general tools. Deciding what their results mean, for example in a debate, is up to whatever uses them.

| Command | What it does | Checks? |
|---|---|---|
| `unilog axioms <Module>` | Lists and counts the axioms declared in that module only. `--transitive` includes every module it imports, each counted once. `--count` prints just the number, `--json` the details. | No: it only parses, so it is fast and independent of `check` |
| `unilog inferences <Module>` | Lists the inferences declared in a module and what each proves, without proofs. `--transitive`, `--count` and `--json` as for `axioms`. | Yes |
| `unilog find <proposition>` | Lists the declarations, in the project and everything it imports, that prove a proposition. `unilog find false` finds contradictions. By default a declaration must prove the query in full generality (a proof of `wrong X` for every `X` also proves `wrong bob`). `--exact` matches only declarations of exactly the query's generality (variable names may differ). `--unify` instead matches anything that unifies with the query, i.e. has its shape, with variables on both sides free to bind (`wrong X` finds `wrong bob`). That finds candidates; it does not mean they prove the query. Queries can be whole implications, e.g. `unilog find "likes X cake => likes X sugar"`. Exits non-zero if nothing matches. It searches existing declarations; it does not search for new proofs. | Yes |
| `unilog rests-on <Mod.name>` | Every axiom a declaration ultimately rests on, following its proof through all modules, each with its module (`--json`). | Yes |
| `unilog proof <Mod.name>` | The whole proof as a tree. Each step shows its expression and what it proves at that point, with variables filled in. Leaves are axioms and assumptions. Inferences it uses are expanded into their own proofs, each once (later uses say "shown above"); an expanded inference's own steps show its general proof. `--shallow` doesn't expand them. `--flat` instead lists every declaration the proof relies on, each once, in declaration order (as written in each module, each module after those it imports), each as `name = proof : statement` (axioms as `name : statement`). Lines refer to each other by name, as in source. `--axioms-first` puts the axioms first. `--json` gives the tree (or the flat list) as data. | Yes |
| `unilog summary` | Axiom and inference counts for every module, checked and imported. `--json` gives every declaration with what it proves, what it uses directly, and every axiom it rests on. | Yes |

Counting axioms and tracing them answer different questions. For example, an argument that adds no beliefs of its own has 0 axioms in its module, but `rests-on` is what shows whose axioms its conclusion actually relies on. That's the right check if the argument also imports some other module of its own.
