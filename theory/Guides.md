Guides are symbolic expressions which instruct the interpreter on how to reach an inference. They typically appear on the right-hand side of an `infer` statement.

> Guides can be non-branching or branching.

#### Non-branching Guides

`[t TheoremTag]`
Produces the theorem whose tag is `TheoremTag`.

`[r RedirectTag]`
Performs a lookup of a redirect by its tag (`RedirectTag`) and calls the guide portion of the redirect.

`[mp ImpGuide JusGuide]`
Applies the Modus ponens rule of inference to the theorems produced by the guides `ImpGuide` and `JusGuide`.

Example 0:

```
axiom a0 [if y x];
axiom a1 x;

infer i0 [mp [t a0] [t a1]];
```

In the above example, we start off with two theorems in the belief set (tags `a0` and `a1`). Since these theorems are in the form of an implication `[if y x]` and justification `x`, we can apply modus ponens to reach theorem `y`. To do this, we used a combination of the `t` guide and `mp` guide.
