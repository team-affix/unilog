# Unilog

A minimal constructive logic for stating beliefs and checking what follows from them.

```
a : likes jake cake.                  # a belief (axiom)
b : likes X cake => likes X sugar.    # a rule: for any X
c = b a.                              # a proof; the checker infers: likes jake sugar
```

- **Constructive logic via Curry–Howard.** Propositions are types, proofs are programs, and checking is type inference. You never write types or quantifiers.
- **Tiny, sound core.** Its only connective is `=>`. There is no recursion, and checking always terminates. `and`, `or`, `false` and even excluded middle are ordinary axioms in packages.
- **Adversarial transparency.** Everything is public, and `unilog axioms` lists every belief a claim ultimately rests on.
- **Built for [DPM](https://github.com/team-affix/apm).** Packages import each other by name, and a project checks against its installed `deps/`.

## Usage

```bash
npm i -g @team-affix/unilog

dpm install            # fetch dependencies into deps/
unilog check           # check this project's <name>/ files against deps/
unilog check -v        # ...and print what every declaration proves
unilog axioms Ethics.Main.keep_promises   # every axiom that claim rests on
unilog show Ethics.Main.keep_promises     # what it proves
```

`unilog check <dir>` on a directory without `dpm.json` checks every `.ul` file in it, e.g. `unilog check examples`.

See the [language reference](docs/language.md) and [`examples/`](examples).

## Development

```bash
npm ci
npm test        # vitest
npm run lint
npm run build
```

```
src/term.ts       propositions, unification, generalization, printing
src/parser.ts     tokenizer and parser
src/check.ts      inference for each declaration (the trusted core)
src/workspace.ts  modules, imports, search roots, axiom tracing
src/commands.ts   check / axioms / show
src/cli.ts        command-line entry point
examples/         a Logic prelude and the design examples; checked by the tests
```
