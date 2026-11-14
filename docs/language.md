# The Unilog language

Unilog is a small constructive logic for stating beliefs and checking what follows from them. A proposition is a symbolic expression, and a proof of a proposition is a term that builds it from axioms. The checker works out what each proof proves and rejects anything that doesn't follow.

Under the hood it is the Curry–Howard correspondence: propositions are types, proofs are programs, and checking is type checking. You never write types.

## Propositions

```
likes jake cake                       # application: ((likes jake) cake)
\X. likes X cake => likes X sugar     # for every X
(\M. mortal M) => mortal socrates     # a universal premise
exists \X. loves X mom                # exists is just a symbol, applied to a proposition with a gap
```

- **Atoms** start with a lowercase letter or digit: `jake`, `likes`, `0`. They are global symbols, so `cake` means the same thing in every package.
- **Variables** start with an uppercase letter or `_`: `X`, `Person`. Every variable must be bound by a lambda. A free variable is an error that says how to bind it.
- **Application** is juxtaposition and is curried: `likes jake cake` is `likes` applied to `jake`, then to `cake`.
- **Implication** is `=>`. A proof of `A => B` turns proofs of `A` into proofs of `B`.
- **Lambdas.** `\X. P` is P with a gap at X, and `(\X. loves X mom) socrates` is the same proposition as `loves socrates mom`. A lambda's body runs as far right as possible; use parentheses to stop it. `\X Y. P` is `\X. \Y. P`, and `\X. f X` is the same as `f`. What a lambda means depends on where it is:
  - **Where a proposition is expected** (a claim, either side of `=>`, a lambda's body) it means **"for every X, P"**. This is the only built-in quantifier.
  - **Handed to something** (an argument, or applied) it is a **gap**: in `exists \X. loves X mom`, `exists` is an ordinary atom and its meaning comes from rules someone writes (see [Quantifiers](#quantifiers)).

## Declarations

Every declaration ends with `;`. A `.` has only two jobs: it joins the parts of a qualified name (`Kant.Ethics.duty`, no spaces around it), and it ends the head of a lambda (`\X. body`). `#` starts a comment.

```
a : likes jake cake;                      # axiom: a stated belief
b : \X. likes X cake => likes X sugar;    # axiom: for every X
c = b a;                                  # inference: what it proves is worked out
c : likes jake sugar = b a;               # inference with its claim stated and checked
import Kant.Ethics;                       # module import (before any declaration)
```

Declaration names start with a lowercase letter or digit.

**Axioms** are beliefs you state without proof.

**Inferences** are proofs. A stated claim must follow *as stated*: `c : \X. likes X sugar = b a;` is rejected, because `b a` only proves it for `jake`. A claim may be a special case of what the proof shows, and the declaration then proves exactly the claim. An inference without a stated claim proves what the checker works out, with any variables left over bound at the front: `id = \P. P;` proves `\A. A => A`.

## Proofs

```
b a                     # apply b to a: a's proposition must match b's premise
\F G X. G (F X)         # assumptions: "assume F, G and X; then G (F X)"
Kant.Ethics.duty        # a qualified reference to an imported declaration
```

- **Application** `b a` matches `a`'s proposition with `b`'s premise, and proves `b`'s conclusion as matching filled it in.
- **Currying.** A rule with several premises takes them one at a time: given `gift : \X Y Z. likes X Y => has Z Y => gives Z X Y`, `gift a` proves `\Z. has Z cake => gives Z jake cake`, an implication still waiting for its second premise.
- **Assumption** `\P. body` proves `A => B` when `body` proves `B` using `P` as a proof of `A`: "assume P; then body". What `P` must prove comes from the stated claim, or is worked out from how it is used: `and_comm = \P. pair (snd P) (fst P);` proves `\A B. and A B => and B A`.
  - Several at once: `\F G X. body` means `\F. \G. \X. body`.
  - An assumption is introduced by a single name, never a pattern. The proof it stands for can be used but not taken apart; that's what elimination rules like `fst`, `snd` and `case` are for.
  - `=>` always means implication. In a proof, the old forms `P => body` and `\P -> body` are errors that say to write `\P. body`, and an old `.` at the end of a declaration is an error that says to use `;`.
- **Names.** Uppercase means a variable everywhere. In a proof, an uppercase name is an assumption (introduced by an enclosing `\P.`), and a lowercase name is a declaration: an earlier one in the same module, or one in exactly one imported module. Use a qualified name (`Mod.name`) when two imports define the same name.

## Quantifiers

**For every.** Using a proof of `\X. P` fills X in by matching, separately at each use. Proving `\X. P` means proving P for a fixed X about which nothing is known. Both work anywhere, including inside a premise:

```
pair : \A B. A => B => both A B;
premise : (\M. mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;
```

F proves `\M. mortal M`, so each use of F picks its own M. Passing a proof to that premise requires a general one: `premise all_mortal` is accepted for `all_mortal : \X. mortal X`, and `premise socrates_mortal` is rejected.

**Every other quantifier is defined by users**, as a symbol applied to a proposition with a gap, with rules written once for every proposition. For example, a package can define `exists`:

```
exists_intro : \B T. B T => exists B;
exists_elim  : \B C. exists B => (\Y. B Y => C) => C;

someone_loves_superman : exists \X. loves X superman = exists_intro lois_loves_superman;
```

The claim gives B = `\X. loves X superman`, so the premise `B T` is `loves T superman`, which `lois_loves_superman` proves with T = `lois`. In `exists_elim`, the handler `(\Y. B Y => C)` must work for every Y, and C can't depend on Y: a handler that only works for lois, or one that concludes something about Y, is rejected. `none`, `at_least_one`, `most` and any other quantifier work the same way, and their rules are axioms that `unilog axioms` lists.

**Rules can look inside lambdas.** `\P Q. (exists \X. both (P X) (Q X)) => exists P` matches `exists \X. both (dog X) (barks X)` with P = `dog` and Q = `barks`. A rule can also require that a body does *not* mention its variable: in `\C. (exists \X. C) => C`, C sits inside the lambda but is not applied to X, so it matches `exists \X. happy jake` but not `exists \X. happy X`. The quantifier itself can be a variable: `\Q B C. (\X. B X => C X) => (\X. C X => B X) => Q B => Q C` says that any quantifier gives the same verdict on two predicates that hold of the same things.

**Equality** is two rules a package can state, and everything else about it follows:

```
refl  : \X. eq X X;
subst : \X Y P. eq X Y => P X => P Y;

sym : \X Y. eq X Y => eq Y X = \E. subst E refl;
superman_loves_lois : loves superman lois = subst kent_is_superman kent_loves_lois;
```

`P` is a proposition with a gap, so substitution works in any position, including inside a universal.

**Where the gap is.** When a rule needs a proposition with a gap (like B in `exists_intro`), the checker finds it from what it knows: the stated claim, a fixed symbol, or two fillings of the same gap (in `subst`, the gap is where the before and after differ). It tries each possible gap and takes the one that fits everything. If more than one fits, it does not guess: it asks for the claim to be stated. `exists_intro lois_loves_superman` alone is rejected, because the gap could be at `lois`, at `superman`, or nowhere.

## What keeps it sound

- **No recursion.** A declaration can only use declarations before it, never itself. Every proof is finite, checking always terminates, and nothing can prove itself (`loop = loop;` is rejected).
- **Occurs check.** A proposition cannot be matched with something containing itself, so self-application in proofs (`\X. X X`) and fixed-point combinators like `\F. (\X. F (X X)) (\X. F (X X))` are rejected.
- **Types rule out self-application in propositions too.** The checker infers a type for every term in a proposition: `t` for propositions and symbols, `A -> B` for something with a gap. Without this, `C = \X. (X X => false)` would make `C C` mean `C C => false`, and `false` would be provable from nothing (Curry's paradox). `\C. C C` is rejected: "'C' is applied to itself". Types are never written; the only other type error is using one variable both as a proposition and as something with a gap.
- **Fixed symbols stay in their scope.** While a "for every X" is being proved, X is a fixed symbol, and nothing from outside may be filled with it. That is what rejects the `exists_elim` handler whose conclusion mentions Y.
- **Assumptions are fixed unless universal.** Inside its proof, an assumption stands for one particular proposition, unless the stated claim makes it universal (like F above).
- **Assumptions stay inside their proof.** `P` can only be used inside the `\P. ...` that introduces it.

- **Every proof is checked twice.** After the checker accepts a declaration, it writes the proof out in full, with everything it filled in made explicit (a *certificate*), and a small independent kernel ([`src/kernel.ts`](../src/kernel.ts)) re-checks it. The kernel never searches or guesses; it only substitutes and compares, using six rules (declaration, assumption, `=>` in and out, "for every" in and out). It shares no code with the checker, so a bug in the checker can't let a wrong proof through: the kernel would reject it, and Unilog reports an internal error. The declaration is still rejected, but the editor underlines it in yellow (a warning) rather than red, because the mistake is Unilog's, not yours. To trust Unilog's verdicts, the kernel is the only code you need to read.

Formally, this is natural deduction for intuitionistic higher-order logic: an assumption `\P. body` is ⇒-introduction, application `f a` is ⇒-elimination (modus ponens), and the universal has its introduction and elimination rules, plus your axioms. It is the architecture of Isabelle, HOL and LF (see [the design](design/quantifiers.md)). The proof terms are never run; the checker only works out what they prove.

- **Nothing private.** Every declaration in every module is public, so everything a claim rests on can be inspected and cited. `unilog axioms` lists the full set of axioms behind any declaration.
- **Logic lives in packages.** The core knows only `=>` and "for every". `and`, `or`, `false`, `exists` and equality are rules in packages ([`examples/Logic/Prelude.ul`](../examples/Logic/Prelude.ul), [`examples/Identity/Equality.ul`](../examples/Identity/Equality.ul)). Excluded middle is not built in: a classical thinker states it as an axiom, and it then appears in the axiom list of everything that relies on it.

## Modules and projects

The module `Kant.Ethics` is the file `Kant/Ethics.ul` under a **search root**. An import is a module name, never a path, and every import (in every module, including dependencies) is looked up in the search roots of the project being checked. So a package's files work unchanged wherever they end up: as the project itself, or next to their own dependencies in a dependency folder.

A project is configured by a `unilog.json` file in its directory (like Agda's `.agda-lib`):

```json
{
    "roots": [".", "deps/*"],
    "check": ["Ethics"]
}
```

- `roots`: where modules are looked up, relative to the project. Put your own source's parent directory here, along with wherever your dependencies are installed. `deps/*` means every folder in `deps/`: that's how [DPM](https://github.com/team-affix/apm) installs packages, one folder per package (`deps/<cid>/`), each laid out like a project. Roots that don't exist yet are skipped. If two roots provide the same module, it is an error: "Module … is ambiguous".
- `check`: the folders (or `.ul` files) that `unilog check` checks. Modules they import are checked as far as they're used.

`unilog init` writes one: `unilog init --roots . 'deps/*' --check Ethics`, or with no options a starting one (`"roots": ["."]`, `"check": []`) to fill in. `unilog check` refuses to run while `check` is empty.

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
| `unilog find <proposition>` | Lists the declarations, in the project and everything it imports, that prove a proposition. `unilog find false` finds contradictions. A query is an ordinary proposition, with its variables bound like any other: a declaration proving `\X. wrong X` is found by `unilog find "wrong bob"` and by `unilog find "\X. wrong X"`, and one proving only `wrong bob` by the first query only. `--exact` matches only declarations of exactly the query's generality (variable names may differ). Exits non-zero if nothing matches. It searches existing declarations; it does not search for new proofs. | Yes |
| `unilog rests-on <Mod.name>` | Every axiom a declaration ultimately rests on, following its proof through all modules, each with its module (`--json`). | Yes |
| `unilog proof <Mod.name>` | The whole proof as a tree. Each step shows its expression and what it proves at that point, with variables filled in. Leaves are axioms and assumptions. Inferences it uses are expanded into their own proofs, each once (later uses say "shown above"); an expanded inference's own steps show its general proof. `--shallow` doesn't expand them. `--flat` instead lists every declaration the proof relies on, each once, in declaration order (as written in each module, each module after those it imports), each as `name = proof : statement` (axioms as `name : statement`). Lines refer to each other by name, as in source. `--axioms-first` puts the axioms first. `--json` gives the tree (or the flat list) as data. | Yes |
| `unilog summary` | Axiom and inference counts for every module, checked and imported. `--json` gives every declaration with what it proves, what it uses directly, and every axiom it rests on. | Yes |

Counting axioms and tracing them answer different questions. For example, an argument that adds no beliefs of its own has 0 axioms in its module, but `rests-on` is what shows whose axioms its conclusion actually relies on. That's the right check if the argument also imports some other module of its own.
