# Design: explicit universal quantification

Status: proposal, not implemented.

## 1. The problem

Today a variable in a proposition is universal without saying so, and only for the whole declaration:

```
b : likes X cake => likes X sugar;        # means: for every X, ...
```

Two things cannot be said:

1. **A universal premise.** "If everyone is mortal, then socrates and plato are mortal." Writing
   `mortal M => both (mortal socrates) (mortal plato)` does not say it: M is chosen by whoever uses the
   declaration, so the premise only demands that *someone* is mortal.
2. **A quantifier anywhere but the front.** "Someone loves everyone", "nobody is perfect", and so on.

We also want every variable to be visibly quantified.

## 2. The proposal in one paragraph

Add one construct, `{X} P`, meaning "for every X, P". It can appear anywhere a proposition can: at the
front of a declaration, inside a premise, nested inside another `{...}`. Every variable in a proposition
must be bound by an enclosing `{...}`; there are no implicit variables any more. The checker knows the
rules of `{...}` (how to use one and how to prove one), and nothing else is built in. Other quantifiers
and connectives (exists, and, or, false, none, ...) are written in terms of `{...}` and `=>`, or as atoms
with axioms, exactly as today.

## 3. Syntax

```
{X} P               for every X, P
{X Y Z} P           for every X, Y and Z, P   (same as {X} {Y} {Z} P)
```

- The body runs as far right as possible, like `\P.` in proofs. Use parentheses to stop it:
  `({X} mortal X) => mortal socrates`.
- A variable (a name starting with an uppercase letter or `_`) in a proposition must be bound by an
  enclosing `{...}`. A free variable is an error, with a fix that adds it to a `{...}`.
- Variables range over all terms, including propositions, as today.
- In proofs nothing changes: uppercase names are assumptions introduced by `\P.`.

Every existing declaration converts by adding its variables at the front:

```
b : {X} likes X cake => likes X sugar;
add_succ : {M N K} add M N K => add (s M) N (s K);
```

## 4. The two rules of `{...}`

**Using** a proof of `{X} P`: X is replaced by an *unknown*, which matching then fills in. This is what
happens today whenever a declaration is used. Now it also happens for assumptions and for the results of
applications, wherever a `{...}` appears.

**Proving** `{X} P`: when a proof is checked against a known `{X} P` (a stated claim, or the premise a
proof is passed to), X is replaced by a *fixed symbol*: a fresh symbol that matches only itself, about
which nothing is known. The proof must prove P for that symbol. This is what happens today when a stated
claim's variables must stay general.

**Escape check.** A fixed symbol belongs to the `{...}` that created it. An unknown created outside that
`{...}` may not be filled with anything containing the fixed symbol. This single check is what keeps both
rules sound.

Matching never uses either rule: two propositions containing `{...}` match only if they are the same up
to renaming of their bound variables. Using and proving happen only where proofs are applied and checked.

## 5. When a claim must be stated

The checker never guesses a `{...}`. An assumption gets a proposition containing `{...}` only from a
stated claim:

```
premise : ({M} mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;
```

Here F gets `{M} mortal M` from the claim. Without the claim, F's proposition would be inferred, and an
inferred proposition never contains `{...}` except at the front. The error in that case says to state the
claim.

Declarations without a stated claim keep working: their claim is inferred, and any variables left over
are bound at the front, so `c = b a;` shows as `c : likes jake sugar`, and a general one as
`c : {A} ...`.

## 6. Worked examples

### A universal premise, used twice

```
pair : {A B} A => B => both A B;

premise : ({M} mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;
```

F proves `{M} mortal M`. Each use of F replaces M with a new unknown, so the first F proves
`mortal socrates` and the second `mortal plato`.

Passing a proof to that premise requires a general one:

```
all_mortal : {X} mortal X;
socrates_mortal : mortal socrates;

ok = premise all_mortal;           # accepted: checked against mortal c for a fixed symbol c
bad = premise socrates_mortal;     # rejected: socrates does not match the fixed symbol
```

### Existence, written with `{...}`

"Someone loves superman" is: for any conclusion C, if C follows from *whoever* loves superman, then C.

```
someone_loves_superman : {C} ({X} loves X superman => C) => C;
```

Proving it (give the handler one person you know):

```
lois_loves_superman : loves lois superman;
someone_loves_superman : {C} ({X} loves X superman => C) => C = \K. K lois_loves_superman;
```

Using it (give a conclusion and a handler that works for anyone):

```
loved_if_anyone : {X} loves X superman => loved superman;
superman_loved : loved superman = someone_loves_superman loved_if_anyone;
```

The handler is checked against `loves x superman => C` for a fixed symbol x, so:

- a handler that only works for lois (`loves lois superman => loved superman`) is rejected:
  lois does not match x;
- a handler whose conclusion mentions the person (`{X} loves X superman => happy X`) is rejected:
  C would have to contain x, which the escape check forbids.

### Nesting in any order

```
everyone_loves_someone : {X} {C} ({Y} loves X Y => C) => C;     # for every X, someone X loves
someone_loves_everyone : {C} ({X} ({Y} loves X Y) => C) => C;   # someone loves every Y
```

## 7. Other quantifiers and connectives

None are built in. Each can be written with `{...}` and `=>`, in which case its rules come from those of
`{...}` and need no axioms:

| Meaning | Written with `{...}` |
|---|---|
| false | `{C} C` |
| A and B | `{C} (A => B => C) => C` |
| A or B | `{C} (A => C) => (B => C) => C` |
| some X with P | `{C} ({X} P => C) => C` |
| no X with P | `{X} P => {C} C` |

Or, as today, a package can make any of them an atom and state its rules as axioms (`examples/Logic`
does this for `and`, `or` and `false`). Either way it is the user's choice, and with atoms the axioms show
up in `unilog axioms`.

The encoded forms are long. If that becomes a burden, a later, separate proposal could add named
abbreviations. Nothing in this design depends on it.

## 8. `unilog find`

A query is an ordinary proposition, quantified like any other.

- Default: declarations whose proof would be accepted for the query.
  `unilog find "wrong bob"` finds `{X} wrong X`; `unilog find "{X} wrong X"` finds only declarations that
  prove it for every X.
- `--exact`: the same proposition, up to renaming bound variables.
- `--unify` is removed.

## 9. Editor

- `{`, `}` are colored light yellow (about `#E8DE8A`), distinct from atoms (blue), variables (teal),
  proof names (purple) and assumptions (orange).
- A free variable gets a quick fix that binds it.
- Hover shows what each `{...}` was filled in with at that step.

## 10. What stays true

- Checking always terminates: no recursion, and matching is still first-order (plus renaming of bound
  variables).
- Soundness: the rules are natural deduction for intuitionistic logic with a universal quantifier over
  all terms, including propositions. Nothing proves `{C} C` from no axioms.
- Every inference rests on axioms listed by `unilog axioms`.

## 11. What this does not do

- **General rules about "any proposition with a gap"** cannot be written. For example, one rule that
  turns *any* example into an existence claim, or substitution of equals in any position of any
  proposition (substitution through a variable P, as in `{P X Y} eq X Y => P X => P Y`, reaches only a
  relation's last argument). That would need lambdas inside propositions, a larger change that can be
  proposed separately if the need shows up.
- **Inference is incomplete where `{...}` is nested:** such declarations must state their claim.

## 12. Migration and versions

- Breaking change: version 0.3.0. Files must bind their variables.
- The examples, docs and tests are converted by hand.

## 13. Implementation outline

1. Grammar: `{ NAME+ } prop` in propositions; free-variable check after parsing.
2. Terms: a binder node; unknowns and fixed symbols carry the depth of the `{...}` they were made in,
   for the escape check.
3. Checker: check a proof against a known claim where there is one (that is where fixed symbols come
   from), infer otherwise; replace the leading `{...}` of anything used with unknowns; bind leftover
   unknowns at the front of inferred claims.
4. Matching: first-order, with bound variables compared by renaming, plus the escape check.
5. `find`, printing, hover, editor grammar and coloring, examples, docs and tests.
