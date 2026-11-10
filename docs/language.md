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

Every declaration ends with `.` followed by whitespace or the end of the file. `#` starts a comment.

```
a : likes jake cake.                      # axiom: a stated belief
c = b a.                                  # theorem: what it proves is inferred
c : likes jake sugar = b a.               # theorem with its claim stated and checked
import Kant.Ethics.                       # module import (before any declaration)
```

Declaration names start with a lowercase letter or digit.

**Axioms** are beliefs you state without proof. Variables in an axiom are universally quantified, and each use gets fresh copies:

```
b : likes X cake => likes X sugar.        # for every X
```

**Theorems** are proofs. A stated claim's variables must hold *in general*: `c : likes X sugar = b a.` is rejected, because `b a` only proves it for `jake`. A claim may be a special case of what the proof shows, and the declaration then proves exactly the claim.

## Proofs

```
b a                     # apply b to a: a's proposition must match b's premise
f => g => x => g (f x)  # assumptions: "assume f, g and x; then g (f x)"
Kant.Ethics.duty        # a qualified reference to an imported declaration
```

- **Application** `b a` unifies `a`'s proposition with `b`'s premise, and proves `b`'s conclusion with the resulting substitution.
- **Assumption** `p => body` proves `A => B` when `body` proves `B` using `p` as a proof of `A`. This is how you prove implications by hypothetical reasoning.
- **Names** refer to, in order: an assumption in scope, an earlier declaration in the same module, or a declaration in exactly one imported module. Use a qualified name (`Mod.name`) when two imports define the same name.

## What keeps it sound

- **No recursion.** A declaration can only use declarations before it, never itself. Every proof is finite, checking always terminates, and nothing can prove itself (`loop = loop.` is rejected).
- **Occurs check.** A proposition cannot be unified with something containing itself, so self-application (`x => x x`) is rejected.
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

Only the checked project's `unilog.json` applies. One that comes with a dependency is ignored.

A directory without a `unilog.json` checks every `.ul` file in it, with the directory as its only root.

- Imports are not re-exported: a module sees only what it imports itself.
- Import cycles are errors, and so is a module found in more than one root.

## Relations, not functions

There are no functions that compute. Write relations as rules instead:

```
znat : nat z.
snat : nat X => nat (s X).
ltz : nat Y => lt z (s Y).
lts : lt X Y => lt (s X) (s Y).

one_lt_two : lt (s z) (s (s z)) = lts (ltz znat).
```

This keeps the core tiny and sound. Recursive functions would allow `loop = loop`, a "proof" of everything.

## Limits, and how to go past them

Variables are quantified only at the outside of a declaration. You can say "for every X, ..." but not "if (for every X, ...) then ...". When an argument needs nested quantifiers, encode a quantified logic as a package, with `forall` and `exists` as ordinary symbols, object variables as data, and rules for substitution and "not free in" as relations:

```
forall_elim  : proves G (forall X P) => subst P X T Q => proves G Q.
forall_intro : proves G P => notfree X G => proves G (forall X P).
```

This is how Metamath builds mathematics from a core that does little more than substitution. Nothing in the core changes. It only makes proofs longer, which is a job for proof-search tools.
