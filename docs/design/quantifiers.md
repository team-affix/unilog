# Design: quantifiers

Status: implemented in 0.3.0. The [language reference](../language.md) describes the result.

## Goal

- A premise can be universal: "if everyone is mortal, then …".
- Quantifiers can sit anywhere, in any order: "someone loves everyone".
- Users define their own quantifiers (exists, none, most, …) and rewrite rules (substitution of equals),
  each written once for every proposition.
- Every variable is visibly bound.

## Overview

One addition: **lambdas in propositions.** `\X. P` is P with a gap at X, and
`(\X. loves X mom) socrates` is the same proposition as `loves socrates mom`.

- **A gap nothing consumes means "for every X".** `\X. mortal X` used as a claim says everyone is mortal.
- **A gap handed to a symbol means whatever that symbol's rules say.** In `exists \X. loves X mom`,
  `exists` is an ordinary atom, and its meaning comes from rules users write.
- **Types are inferred, never written,** and keep lambdas sound.

Which of the two a lambda is depends only on where it is written: as an argument, or applied, it is a gap;
anywhere a proposition is expected (a claim, either side of `=>`, a lambda's body) it means "for every".

There are no built-in quantifiers, connectives or equality, and no standard library.

## Syntax

```
\X Y. P        P with gaps at X and Y
q \X. P        the symbol q applied to a gap (q is an ordinary atom)
```

- Bodies run as far right as possible; parenthesize to stop them: `(\X. mortal X) => mortal socrates`.
- Every variable in a proposition must be bound by a lambda. A free variable is an error that says how to bind it.
- Proofs are unchanged.

```
b : \X. likes X cake => likes X sugar;
```

## Universal: a gap nothing consumes

Wherever a proposition is expected (a claim, either side of `=>`, a lambda's body) and a lambda appears, it
is claimed for every filling of the gap. This is decided by where the lambda is written, so it is never a
guess.

- **Using** such a proof: X becomes an unknown, filled in by matching (as declarations work today, now
  also for assumptions and results).
- **Proving** it: the proof is checked with X as a fresh *fixed symbol* that matches only itself (as stated
  claims work today).
- **Escape check:** an unknown made outside the lambda can't be filled with its fixed symbol.

## Types

The checker infers a type for every term, from how it is used:

- `t`: any term, propositions included (`socrates`, `loves jake mom`);
- `A -> B`: something with a gap (`loves : t -> t -> t`, `\X. loves X mom : t -> t`,
  `exists : (t -> t) -> t`).

Types exist only to forbid self-application. Without them, `C = \X. (X X => false)` makes `C C` mean
`C C => false`, and `false` becomes provable from nothing (Curry's paradox). With them, `X X` has no type.
Typed lambdas also always finish reducing.

The only new error is inconsistent use, such as `loves` applied to two arguments in one place and used as
a whole proposition in another.

## Finding gaps

When B is an unknown, matching `B T` against a proposition is solved only when the answer is unique:

- B is already known, e.g. from the stated claim;
- T is a fixed symbol: the gap is wherever it occurs;
- two fillings `B X` and `B Y` are known: the gap is where they differ.

In general the checker tries each possible gap (each choice of which occurrences become the gap) against
everything else it knows, and takes the one that fits. If more than one fits, it asks for the claim to be
stated rather than guessing. (The cases above are the decidable fragment of higher-order unification used
by Twelf and λProlog.) When a universal is needed and a name proving a universal is given, the two are
compared binder against binder first.

## Examples

**A universal premise, used twice:**

```
pair : \A B. A => B => both A B;
premise : (\M. mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;

all_mortal : \X. mortal X;
socrates_mortal : mortal socrates;

ok  = premise all_mortal;          # accepted: holds for every X
bad = premise socrates_mortal;     # rejected: socrates is not the fixed symbol
```

**A user-defined quantifier:**

```
exists_intro : \B T. B T => exists B;
exists_elim  : \B C. exists B => (\Y. B Y => C) => C;

lois_loves_superman : loves lois superman;
someone_loves_superman : exists \X. loves X superman = exists_intro lois_loves_superman;
```

The claim gives B = `\X. loves X superman`, so `B T` is `loves T superman`, so T = lois.

In `exists_elim`, `exists B` hands B to `exists`, so it is not universal, while `(\Y. B Y => C)` is a
premise with a gap nothing consumes, so it means "for every Y".

**Rewriting anywhere:**

```
refl  : \X. eq X X;
subst : \B X Y. eq X Y => B X => B Y;

superman_loves_lois : loves superman lois = subst kent_is_superman kent_loves_lois;
```

B = `\Z. loves Z lois`: the gap is where `loves clark_kent lois` and `loves superman lois` differ.

**Nesting:**

```
\X. exists \Y. loves X Y        # everyone loves someone
exists \X. \Y. loves X Y        # someone loves everyone
```

In the second line, the inner `\Y.` is in the body of the gap handed to `exists`, where a proposition is
expected and nothing consumes it, so it is universal.

## Tools

- **`unilog find`:** a query is an ordinary closed proposition. The default lists declarations whose
  proof would be accepted for it; `--exact` lists the same proposition up to renaming. `--unify` is
  removed.
- **Editor:** light yellow (`#E8DE8A`) marks quantification: the `\` and `.` of a lambda in a
  proposition, and any symbol directly followed by a lambda (`exists`). A forgotten `exists` silently becomes
  "for every", so a lambda with no yellow symbol in front of it is a universal.

## Guarantees and limits

- Checking terminates; the logic is sound (higher-order intuitionistic logic, as in Isabelle/HOL).
- Every inference rests on axioms listed by `unilog axioms`.
- Claims must be stated where the checker would otherwise guess.
- No computation: relations, not functions, as today.

## Migration and implementation

Breaking change, version 0.3.0; examples, docs and tests are converted by hand.

1. **Grammar:** lambdas in propositions; the free-variable check.
2. **Type inference** for terms (Hindley–Milner, which the checker already uses for proofs), including
   which lambdas are universal.
3. **Checker:** using and proving universals, the escape check, and gap finding.
4. **Everything else:** `find`, printing, hover, editor grammar and colors, examples, docs and tests.

## Prior art

Every major system with custom quantifiers makes a quantifier a constant applied to a lambda, and uses
types to stay sound:

- **HOL:** `∀x. P x` is the constant `∀` applied to `λx. P x`; lambda is the only binder. Its generality
  comes from free variables in theorems and an abstraction rule, much like a gap nothing consumes here.
  ([Benzmüller et al.](https://arxiv.org/pdf/1802.08454))
- **Isabelle:** one built-in universal `⋀`; every logic on top defines its quantifiers as constants over
  lambdas. ([Paulson](https://arxiv.org/pdf/cs/9301106))
- **LF / Twelf:** `all : (i -> o) -> o`, with decidable pattern unification.
  ([Pientka](https://cs.mcgill.ca/~bpientka/papers/red-elim.pdf),
  [Nadathur & Linnell](https://www-users.cse.umn.edu/~ngopalan/papers/pattern-unif.pdf))
- **Lean 4:** `∃ x, p x` is `Exists (fun x => p x)`; custom quantifiers are notation.
  ([Ullrich & de Moura](https://arxiv.org/pdf/2001.10490))
- **Metamath** is the exception: no lambdas. It defines substitution explicitly and adds
  distinct-variable side conditions. ([df-eu](https://us.metamath.org//mpegif//df-eu.html))
