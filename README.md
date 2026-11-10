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
- **Independent of any package manager.** Modules import each other by name, resolved through the search roots in the project's `unilog.json`. Whatever installs dependencies, point a root at the folder it uses.

## Usage

```bash
npm i -g @team-affix/unilog

unilog check           # check the project in this directory, as configured by unilog.json
unilog check -v        # ...and print what every declaration proves
unilog axioms Ethics.Main                 # axioms declared in a module (--transitive: and all it imports)
unilog rests-on Ethics.Main.keep_promises # every axiom that claim rests on
unilog show Ethics.Main.keep_promises     # what it proves
unilog summary                            # axiom/theorem counts per module (--json: the full graph)
```

A project's `unilog.json` says where modules are found and what to check:

```json
{
    "roots": [".", "deps"],
    "check": ["Ethics"]
}
```

`unilog check <dir>` on a directory without one checks every `.ul` file in it, e.g. `unilog check examples`.

See the [language reference](docs/language.md) and [`examples/`](examples).

## Development

```bash
npm ci
npm test        # generates the parser from the grammar, then runs vitest
npm run lint
npm run build
```

The parser is generated from [`grammar/Unilog.g4`](grammar/Unilog.g4) with [antlr-ng](https://github.com/mike-lischke/antlr-ng) (no Java needed) into `src/generated/`, which is not committed. `npm run grammar` regenerates it.

```
src/term.ts       propositions, unification, generalization, printing
grammar/Unilog.g4 the ANTLR grammar
src/parser.ts     parse tree -> AST, and the rules about names
src/check.ts      inference for each declaration (the trusted core)
src/workspace.ts  modules, imports, search roots, axiom tracing
src/commands.ts   check / axioms / show
src/cli.ts        command-line entry point
examples/         a Logic prelude and the design examples; checked by the tests
```
