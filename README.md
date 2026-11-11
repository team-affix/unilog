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
# install (Node.js 20+), using the .tgz from the latest release: https://github.com/team-affix/unilog/releases
npm install -g https://github.com/team-affix/unilog/releases/download/v0.1.6/team-affix-unilog-0.1.6.tgz

unilog init                                # create a unilog.json here (--roots . deps --check MyPkg to fill it in)
unilog check                               # check the project in this directory, as configured by unilog.json
unilog check -v                            # ...and print what every declaration proves
unilog axioms Ethics.Main                  # axioms declared in a module (--transitive: and all it imports)
unilog inferences Ethics.Main              # inferences in a module and what they prove (--transitive)
unilog find false                          # which declarations prove a proposition (exit 1 if none)
unilog rests-on Ethics.Main.keep_promises  # every axiom that claim rests on
unilog proof Ethics.Main.keep_promises     # the whole proof as a tree, down to the axioms
unilog show Ethics.Main.keep_promises      # what it proves
unilog summary                             # axiom/inference counts per module (--json: the full graph)
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

## Editor support

A VS Code extension lives in [`editors/vscode`](editors/vscode): errors as you type, hover to see what any declaration or proof step proves, inferred claims shown inline, go to definition and an outline. It is a client for the Unilog language server (`unilog-lsp`, in [`src/lsp`](src/lsp)), which uses the same checker as `unilog check`. Other editors that speak the Language Server Protocol can run `unilog-lsp --stdio`.

## Releases

Built files are never committed. They are attached to [GitHub Releases](https://github.com/team-affix/unilog/releases) by CI:

| File | What it is |
|---|---|
| `unilog-<version>.cjs` | the CLI as a single file: `node unilog-<version>.cjs check` |
| `unilog-lsp-<version>.cjs` | the language server as a single file, for any LSP editor: `node unilog-lsp-<version>.cjs --stdio` |
| `unilog-vscode-<version>.vsix` | the VS Code extension: `code --install-extension unilog-vscode-<version>.vsix` |
| `team-affix-unilog-<version>.tgz` | the npm package (both commands and the library): `npm i -g ./team-affix-unilog-<version>.tgz` |

To release: set the same new version in `package.json` and `editors/vscode/package.json`, and merge to `main`. After every test job passes, CI tags the merge commit `v<version>` and publishes the release. Merges that don't change the version publish nothing. (Pushing a matching `v*` tag by hand also releases.)

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
src/lsp/          language server: editor features (features.ts) and protocol wiring (server.ts)
editors/vscode/   VS Code extension (client, syntax highlighting, packaging)
examples/         a Logic prelude and the design examples; checked by the tests
```
