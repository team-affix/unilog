# Design: quantifiers

Status: proposal, not implemented.

## 1. The problem

Today a variable in a proposition is universal without saying so, and only for the whole declaration:

```
b : likes X cake => likes X sugar;        # means: for every X, ...
```

So these cannot be said:

1. **A universal premise.** "If everyone is mortal, then socrates is mortal." In
   `mortal M => mortal socrates`, M is chosen by whoever uses the declaration, so the premise only
   demands that *someone* is mortal.
2. **A quantifier anywhere but the front.** "Someone loves everyone", "nobody is perfect".
3. **Other quantifiers** (exists, none, at least one, most, ...), with rules written once that work for
   every proposition.
4. **Rewriting anywhere.** Substituting equals for equals in any position of any proposition.

We also want every variable to be visibly quantified.

## 2. The proposal

Two additions:

1. **`{X} P`**: "for every X, P". The one built-in quantifier. It can appear anywhere a proposition can.
   Every variable must be bound by a `{...}` or a lambda; there are no implicit variables.
2. **Lambdas in propositions**: `\X. P` is "P with a gap at X". `(\X. loves X mom) socrates` is the same
   proposition as `loves socrates mom`. Any other quantifier is an ordinary atom in front of a lambda:
   `exists \X. loves X mom`. Its meaning comes from rules users write.

Nothing else is built in: no other quantifiers, no connectives, no equality, no standard library.

## 3. Syntax

```
{X} P               for every X, P
{X Y Z} P           for every X, Y and Z, P   (same as {X} {Y} {Z} P)
\X. P               P with a gap at X
\X Y. P             same as \X. \Y. P
q \X. P             q applied to \X. P: a quantifier written by a user
```

- `{...}` and `\X.` bodies run as far right as possible. Use parentheses to stop them:
  `({X} mortal X) => mortal socrates`.
- A variable (a name starting with an uppercase letter or `_`) in a proposition must be bound by an
  enclosing `{...}` or lambda. A free variable is an error, with a fix that binds it.
- Variables range over all terms, including propositions and lambdas.
- In proofs nothing changes: uppercase names are assumptions introduced by `\P.`.

Every existing declaration converts by binding its variables at the front:

```
b : {X} likes X cake => likes X sugar;
add_succ : {M N K} add M N K => add (s M) N (s K);
```

## 4. `{...}`: the two rules

**Using** a proof of `{X} P`: X is replaced by an *unknown*, which matching then fills in. This is what
happens today whenever a declaration is used. Now it also happens for assumptions and for the results of
applications, wherever a `{...}` appears.

**Proving** `{X} P`: when a proof is checked against a known `{X} P` (a stated claim, or the premise a
proof is passed to), X is replaced by a *fixed symbol*: a fresh symbol that matches only itself, about
which nothing is known. The proof must prove P for that symbol. This is what happens today when a stated
claim's variables must stay general.

**Escape check.** A fixed symbol belongs to the `{...}` that created it. An unknown created outside that
`{...}` may not be filled with anything containing the fixed symbol. This single check keeps both rules
sound.

## 5. Lambdas: propositions with a gap

`(\X. P) A` is the same proposition as P with A in place of X. The checker always works with, and shows,
the result of filling the gap.

**The one restriction: a gap may only be filled with something that contains no lambda.**

Without it the logic is unsound (Curry's paradox): with `C = \X. (X X => false)`, the proposition `C C`
is the same as `C C => false`, and from that `false` can be proved with no axioms. The restriction rejects
`C C`, because it fills C's gap with a lambda. It also guarantees that filling gaps always finishes: each
step removes a lambda and adds none.

What it costs: a gap cannot hold another gapped proposition. Gaps are filled with things like `socrates`,
`s z` or `likes jake cake`, which covers quantifiers, predicate variables and rewriting.

**Matching with a gap.** Matching `B T`, where B is an unknown, against a proposition has one answer
when B is already known (from a stated claim or another premise). Then the gap is filled and matching
proceeds as usual. When B is not known, the answer is usually ambiguous (in `loves lois superman`, is the
gap at lois, at superman, or nowhere?), and the checker asks for the claim to be stated rather than
guess. Two cases are unambiguous and solved directly:

- T is a fixed symbol: the gap is wherever that symbol occurs.
- Two fillings of the same B are both known, such as `B X` and `B Y` in a rewrite rule: the gap is
  exactly where the two propositions differ.

## 6. When a claim must be stated

The checker never guesses a `{...}` or a gap:

- An assumption gets a proposition containing `{...}` only from a stated claim.
- A gap is found only when it is determined, as in section 5.

The error in either case says which claim to state. Declarations without a stated claim keep working
when nothing needs guessing: `c = b a;` shows as `c : likes jake sugar`, and leftover variables are bound
at the front, as `c : {A} ...`.

## 7. Worked examples

### A universal premise, used twice

```
pair : {A B} A => B => both A B;

premise : ({M} mortal M) => both (mortal socrates) (mortal plato) = \F. pair F F;
```

F proves `{M} mortal M`. Each use replaces M with a new unknown, so the first F proves `mortal socrates`
and the second `mortal plato`. Passing a proof to that premise requires a general one:

```
all_mortal : {X} mortal X;
socrates_mortal : mortal socrates;

ok = premise all_mortal;           # accepted
bad = premise socrates_mortal;     # rejected: socrates does not match the fixed symbol
```

### A quantifier defined by a user

A package (any package; there is no standard one) can define `exists`:

```
exists_intro : {B T} B T => exists B;
exists_elim  : {B C} exists B => ({Y} B Y => C) => C;
```

Using them:

```
lois_loves_superman : loves lois superman;
someone_loves_superman : exists \X. loves X superman = exists_intro lois_loves_superman;
```

The claim gives B = `\X. loves X superman`. The premise `B T` becomes `loves T superman`, which matches
`loves lois superman`, so T = lois.

```
loved_if_anyone : {X} loves X superman => loved superman;
superman_loved : loved superman = exists_elim someone_loves_superman loved_if_anyone;
```

The handler is checked against `loves y superman => C` for a fixed symbol y. A handler that only works
for lois is rejected (lois does not match y), and so is one whose conclusion mentions the person
(`{X} loves X superman => happy X`), because C may not contain y.

The same pattern gives any other quantifier its rules: `none`, `at_least_one`, `most`, ... Each is an
atom followed by a lambda, and its rules are axioms that `unilog axioms` lists.

### Rewriting anywhere

```
refl  : {X} eq X X;
subst : {B X Y} eq X Y => B X => B Y;

kent_is_superman : eq clark_kent superman;
kent_loves_lois  : loves clark_kent lois;
superman_loves_lois : loves superman lois = subst kent_is_superman kent_loves_lois;
```

The claim and the premise together determine B = `\Z. loves Z lois`: the gap is where the two differ.
The same rule works in any position, and inside quantified propositions.

### Nesting in any order

```
everyone_loves_someone : {X} exists \Y. loves X Y;
someone_loves_everyone : exists \X. {Y} loves X Y;
```

## 8. Encodings without axioms

Connectives can also be written with `{...}` and `=>` alone, so their rules come from `{...}` and need
no axioms:

| Meaning | Written with `{...}` |
|---|---|
| false | `{C} C` |
| A and B | `{C} (A => B => C) => C` |
| A or B | `{C} (A => C) => (B => C) => C` |
| some X with P | `{C} ({X} P => C) => C` |

Whether to use these or atoms with axioms is the user's choice.

## 9. `unilog find`

A query is an ordinary proposition, quantified like any other.

- Default: declarations whose proof would be accepted for the query.
  `unilog find "wrong bob"` finds `{X} wrong X`; `unilog find "{X} wrong X"` finds only declarations that
  prove it for every X.
- `--exact`: the same proposition, up to renaming bound variables.
- `--unify` is removed.

## 10. Editor

- Quantifiers are light yellow (about `#E8DE8A`): the `{`, `}` braces, and any atom directly followed by
  a lambda (`exists` in `exists \X. ...`). This is distinct from atoms (blue), variables (teal), proof names
  (purple) and assumptions (orange).
- A free variable gets a quick fix that binds it.
- Hover shows what each `{...}` and each gap was filled with at that step.

## 11. What stays true

- **Checking always terminates:** no recursion; filling gaps always finishes (section 5); matching is
  first-order apart from gaps, which are solved only when determined.
- **Soundness:** the rules are natural deduction for intuitionistic logic, with a universal quantifier
  over all terms and with predicate variables. The lambda restriction rules out Curry's paradox. Nothing
  proves `{C} C` from no axioms.
- **Every inference rests on axioms listed by `unilog axioms`.**

## 12. Limits

- **A gap cannot be filled with a lambda** (section 5). Rules about rules about gapped propositions
  (third-order) cannot be written directly.
- **Claims must be stated where something would otherwise be guessed** (section 6).
- **No computation:** relations, not functions, as today.

## 13. Migration and versions

- Breaking change: version 0.3.0. Files must bind their variables.
- The examples, docs and tests are converted by hand.

## 14. Implementation outline

1. **Grammar:** `{ NAME+ } prop` and `\NAME+. prop` in propositions; free-variable check after parsing.
2. **Terms:**
   - binder nodes for `{...}` and lambdas;
   - filling gaps (with the no-lambda check), keeping propositions in filled form;
   - unknowns and fixed symbols that carry the depth they were made at, for the escape check.
3. **Checker:**
   - check a proof against a known claim where there is one, and infer otherwise;
   - replace the leading `{...}` of anything used with unknowns;
   - bind leftover unknowns at the front of inferred claims.
4. **Matching:**
   - first-order, with bound variables compared up to renaming;
   - gaps solved when determined, and an error otherwise;
   - the escape check.
5. **Everything else:** `find`, printing, hover, editor grammar and coloring, examples, docs and tests.
