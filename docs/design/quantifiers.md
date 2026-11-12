# Design: quantifiers

Status: proposal, not implemented.

## Goal

- A premise can be universal: "if everyone is mortal, then …".
- Quantifiers can sit anywhere, in any order: "someone loves everyone".
- Users define their own quantifiers (exists, none, most, …) and rewrite rules (substitution of equals),
  each written once for every proposition.
- Every variable is visibly bound.

## Overview

| Added | Meaning |
|---|---|
| `{X} P` | for every X, P. The one built-in quantifier. |
| `\X. P` | P with a gap at X. `(\X. loves X mom) socrates` is `loves socrates mom`. |
| Inferred types | Keep lambdas sound. Never written by users. |

Any other quantifier is an atom in front of a lambda (`exists \X. loves X mom`); its meaning comes from
rules users write. Nothing else is built in: no connectives, no equality, no standard library.

This is the architecture of Isabelle, HOL and LF (see [Prior art](#prior-art)).

## Syntax

```
{X Y} P        for every X and Y, P
\X Y. P        P with gaps at X and Y
```

- Bodies run as far right as possible; parenthesize to stop them: `({X} mortal X) => mortal socrates`.
- Every variable in a proposition must be bound by `{...}` or a lambda. A free variable is an error with
  a fix.
- Proofs are unchanged.

```
b : {X} likes X cake => likes X sugar;
```

## The rules of `{X}`

- **Using** a proof of `{X} P`: X becomes an unknown, filled in by matching (as declarations work
  today, now also for assumptions and results).
- **Proving** `{X} P`: the proof is checked with X as a fresh *fixed symbol* that matches only itself
  (as stated claims work today).
- **Escape check:** an unknown made outside a `{X}` can't be filled with that `{X}`'s fixed symbol.

## Lambdas and types

The checker infers a type for every term, from how it is used:

- `t`: any term, propositions included (`socrates`, `loves jake mom`);
- `A -> B`: something with a gap (`loves : t -> t -> t`, `\X. loves X mom : t -> t`,
  `exists : (t -> t) -> t`).

Types exist only to forbid self-application. Without them, `C = \X. (X X => false)` makes `C C` mean
`C C => false`, and `false` becomes provable from nothing (Curry's paradox). With them, `X X` has no type
and is rejected. Typed lambdas also always finish reducing.

Users never write types. The only new error is inconsistent use, such as `loves` applied to two arguments
in one place and used as a whole proposition in another.

## Finding gaps

When B is an unknown, matching `B T` against a proposition is solved only when the answer is unique:

- B is already known, e.g. from the stated claim;
- T is a fixed symbol: the gap is wherever it occurs;
- two fillings `B X` and `B Y` are known: the gap is where they differ.

Otherwise the checker asks for the claim to be stated rather than guessing. (This is the decidable
fragment of higher-order unification used by Twelf and λProlog.)

## Examples

**A universal premise, used twice:**

```
pair : {A B} A => B => both A B;
premise : ({M} mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;

all_mortal : {X} mortal X;
socrates_mortal : mortal socrates;

ok  = premise all_mortal;          # accepted: holds for every X
bad = premise socrates_mortal;     # rejected: socrates is not the fixed symbol
```

**A user-defined quantifier:**

```
exists_intro : {B T} B T => exists B;
exists_elim  : {B C} exists B => ({Y} B Y => C) => C;

lois_loves_superman : loves lois superman;
someone_loves_superman : exists \X. loves X superman = exists_intro lois_loves_superman;
```

The claim gives B = `\X. loves X superman`, so `B T` is `loves T superman`, so T = lois.

**Rewriting anywhere:**

```
refl  : {X} eq X X;
subst : {B X Y} eq X Y => B X => B Y;

superman_loves_lois : loves superman lois = subst kent_is_superman kent_loves_lois;
```

B = `\Z. loves Z lois`: the gap is where `loves clark_kent lois` and `loves superman lois` differ.

**Nesting:**

```
{X} exists \Y. loves X Y        # everyone loves someone
exists \X. {Y} loves X Y        # someone loves everyone
```

## Tools

- **`unilog find`:** a query is an ordinary closed proposition. The default lists declarations whose
  proof would be accepted for it; `--exact` lists the same proposition up to renaming. `--unify` is
  removed.
- **Editor:** quantifiers are light yellow (`#E8DE8A`): the `{ }` braces, and any atom directly
  followed by a lambda. A free variable gets a quick fix that binds it.

## Guarantees and limits

- Checking terminates; the logic is sound (higher-order intuitionistic logic, as in Isabelle/HOL).
- Every inference rests on axioms listed by `unilog axioms`.
- Claims must be stated where the checker would otherwise guess.
- No computation: relations, not functions, as today.

## Migration and implementation

Breaking change, version 0.3.0; examples, docs and tests are converted by hand.

1. **Grammar:** `{...}` and lambdas in propositions; the free-variable check.
2. **Type inference** for terms (Hindley–Milner, which the checker already uses for proofs).
3. **Checker:** the `{X}` rules, the escape check, and gap finding.
4. **Everything else:** `find`, printing, hover, editor grammar and colors, examples, docs and tests.

## Prior art

Every major system with custom quantifiers makes a quantifier a constant applied to a lambda, and uses
types to stay sound:

- **HOL:** `∀x. P x` is `∀ (λx. P x)`. Lambda is the only binder, and ∀/∃ are defined.
  ([Benzmüller et al.](https://arxiv.org/pdf/1802.08454))
- **Isabelle:** one built-in universal, `⋀`, like our `{X}`; every logic on top defines its quantifiers
  as constants over lambdas. ([Paulson](https://arxiv.org/pdf/cs/9301106))
- **LF / Twelf:** `all : (i -> o) -> o`, with decidable pattern unification.
  ([Pientka](https://cs.mcgill.ca/~bpientka/papers/red-elim.pdf),
  [Nadathur & Linnell](https://www-users.cse.umn.edu/~ngopalan/papers/pattern-unif.pdf))
- **Lean 4:** `∃ x, p x` is `Exists (fun x => p x)`; custom quantifiers are notation.
  ([Ullrich & de Moura](https://arxiv.org/pdf/2001.10490))
- **Metamath** is the exception: no lambdas. It defines substitution explicitly and adds
  distinct-variable side conditions. ([df-eu](https://us.metamath.org//mpegif//df-eu.html))
