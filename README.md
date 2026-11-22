# Unilog

A minimal constructive logic for stating beliefs and checking what follows from them.

```
a : likes jake cake;                      # a belief (axiom)
b : \X. likes X cake => likes X sugar;    # a rule: for every X
c = b a;                                  # a proof; the checker works out: likes jake sugar
```

- **Constructive logic via Curry–Howard.** Propositions are types, proofs are programs, and checking is type checking. You never write types.
- **Tiny, sound core.** It knows only `=>` and "for every" (`\X.`), usable anywhere, including inside premises. There is no recursion, and checking always terminates. `and`, `or`, `false`, `exists`, equality and even excluded middle are ordinary rules in packages.
- **Quantifiers are yours to define.** Any quantifier is a symbol applied to a proposition with a gap (`exists \X. loves X mom`), and its rules are written once for every proposition.
- **Every proof is checked twice.** A small independent kernel ([`src/kernel.ts`](src/kernel.ts)) re-checks every proof the checker accepts, from a certificate with everything made explicit. To trust a verdict, that file is all you need to read.
- **Adversarial transparency.** Everything is public, and `unilog axioms` lists every belief a claim ultimately rests on.
- **Independent of any package manager.** Modules import each other by name, resolved through the search roots in the project's `unilog.json`. Whatever installs dependencies, point a root at the folder it uses.

## Usage

```bash
# install the latest release (Node.js 20+); run it again to upgrade
npm install -g https://github.com/team-affix/unilog/releases/latest/download/unilog.tgz

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

A VS Code extension lives in [`editors/vscode`](editors/vscode). To install the latest:

```bash
curl -fsSLO https://github.com/team-affix/unilog/releases/latest/download/unilog-vscode.vsix && code --install-extension unilog-vscode.vsix
```

It gives you errors as you type, hover to see what any declaration or proof step proves, inferred claims shown inline, go to definition and an outline. It is a client for the Unilog language server (`unilog-lsp`, in [`src/lsp`](src/lsp)), which uses the same checker as `unilog check`. Other editors that speak the Language Server Protocol can run `unilog-lsp --stdio`.

## Development

Building, testing, the code layout and how releases are made: [DEVELOPMENT.md](DEVELOPMENT.md).
