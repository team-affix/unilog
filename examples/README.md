# Examples

Each folder is a small package of beliefs and what follows from them. Run the commands below from this
directory (or pass `examples` as the directory from the repository root). With the VS Code extension
installed, open this folder to hover over any step of a proof.

| Folder | What it shows |
|---|---|
| `Logic/` | The connectives (`and`, `or`, `false`) as plain axioms (`Prelude`), classical logic as one extra belief (`Classical`), and laws proved from them (`Laws`): currying, contraposition, De Morgan, distributivity |
| `Arithmetic/` | Numbers built from `z` and `s`, with addition and multiplication as rules: `2 + 2 = 4` is a checked calculation |
| `Family/` | A family tree: ancestors, siblings, and a derived rule that a grandparent is an ancestor |
| `Aristotle/` | Socrates is mortal, and the syllogism form Barbara proved from no axioms at all |
| `Identity/` | Equality as rules. Frege's morning star and evening star, and Superman's glasses |
| `Ethics/`, `Debate/` | A Kantian and a utilitarian, each consistent alone, contradict each other about the murderer at the door |
| `Knowledge/` | Knowledge as justified true belief, and where Gettier's objection would bite |
| `Science/` | Reasoning backwards with modus tollens: dry ground means no rain |
| `Games/`, `Travel/` | Rock-paper-scissors and routes on a map, as rules |
| `Demo/` | The first examples from the language's design |

## Things to try

```bash
unilog check -v                                           # everything checks; -v shows what each declaration proves

unilog find false                                         # finds the one contradiction: Debate.Clash.clash
unilog rests-on Debate.Clash.clash                        # the five beliefs it rests on, from both sides
unilog axioms Debate.Clash --count                        # 0: the debate adds no beliefs of its own

unilog rests-on Aristotle.Syllogisms.barbara              # rests on no axioms: it is pure logic
unilog rests-on Logic.Classical.double_negation           # needs excluded middle (Logic.Classical.lem)

unilog proof Identity.Venus.hesperus_seen_at_dawn --flat  # the whole argument, in order
unilog proof Arithmetic.Peano.two_times_two               # 2 * 2 = 4, step by step

unilog find "mortal X" --unify                            # everyone proved mortal: socrates, plato
unilog inferences Logic.Laws                              # the laws and what each states
unilog summary                                            # axiom and inference counts for every module
```

To see a belief give way, edit `Ethics/Kant.ul` so lying is wrong only when it harms someone
(`lying_is_wrong : lies X => harms X => wrong X;`). `unilog check` then reports exactly where
`Debate.Clash` stopped working: the contradiction depended on the old belief.
