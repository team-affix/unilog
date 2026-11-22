# Developing Unilog

## Building and testing

```bash
npm ci
npm test        # generates the parser from the grammar, then runs vitest
npm run lint
npm run build
```

The parser is generated from [`grammar/Unilog.g4`](grammar/Unilog.g4) with [antlr-ng](https://github.com/mike-lischke/antlr-ng) (no Java needed) into `src/generated/`, which is not committed. `npm run grammar` regenerates it.

```
src/kernel.ts     the proof kernel: re-checks every certificate (the only code you need to trust)
src/term.ts       propositions, matching (unknowns, gaps, scopes), printing
grammar/Unilog.g4 the ANTLR grammar
src/parser.ts     parse tree -> AST, and the rules about names
src/check.ts      works out what each declaration proves, and writes its certificate
src/certificate.ts the checker's work, written out for the kernel
src/workspace.ts  modules, imports, search roots, axiom tracing
src/commands.ts   check / axioms / show
src/cli.ts        command-line entry point
src/lsp/          language server: editor features (features.ts) and protocol wiring (server.ts)
editors/vscode/   VS Code extension (client, syntax highlighting, packaging)
examples/         a Logic prelude and the design examples; checked by the tests
```

## Releases

Built files are never committed. They are attached to [GitHub Releases](https://github.com/team-affix/unilog/releases) by CI:

| File | What it is |
|---|---|
| `unilog-<version>.cjs` | the CLI as a single file: `node unilog-<version>.cjs check` |
| `unilog-lsp-<version>.cjs` | the language server as a single file, for any LSP editor: `node unilog-lsp-<version>.cjs --stdio` |
| `unilog-vscode-<version>.vsix` | the VS Code extension: `code --install-extension unilog-vscode-<version>.vsix` |
| `team-affix-unilog-<version>.tgz` | the npm package (both commands and the library): `npm i -g ./team-affix-unilog-<version>.tgz` |

Each release also has the same files without the version in their names (`unilog.cjs`, `unilog-lsp.cjs`, `unilog-vscode.vsix`, `unilog.tgz`), so `https://github.com/team-affix/unilog/releases/latest/download/<name>` always gets the newest.

To release: set the same new version in `package.json` and `editors/vscode/package.json`, and merge to `main`. After every test job passes, CI tags the merge commit `v<version>` and publishes the release. Merges that don't change the version publish nothing. (Pushing a matching `v*` tag by hand also releases.)
