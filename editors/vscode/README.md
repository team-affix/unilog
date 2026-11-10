# Unilog for VS Code

Language support for [Unilog](https://github.com/team-affix/unilog) (`.ul` files):

- **Errors as you type**, from the same checker as `unilog check`, including in files you haven't saved.
- **Hover** a declaration to see what it proves, or any step of a proof to see what that step proves there, with variables filled in.
- **Inferred claims inline**: a theorem written without its claim (`c = b a.`) shows what it proves next to its name. Double-click the hint to write it into the file.
- **Go to definition** for any name in a proof, and for imported modules (including ones in your dependency folders).
- **Outline** of each module's axioms and theorems.
- **Gutter markers**: a hollow amber diamond beside each axiom (assumed, not proved) and a solid teal circle beside each theorem (derived), so a file's beliefs are visible at a glance.
- **Colors**: names of proofs (declared or used) share your theme's declaration color, assumptions and proposition variables each get their own, and plain symbols stay uncolored.
- Syntax highlighting.

Projects are found by their `unilog.json`; see the [language reference](https://github.com/team-affix/unilog/blob/main/docs/language.md).

## Development

```bash
npm install
npm run build       # generates the parser and bundles the extension and language server into out/
npm test            # syntax-highlighting tests
npm run package     # builds unilog-<version>.vsix; install with: code --install-extension unilog-0.1.0.vsix
```

Press F5 in VS Code (with this folder open) to launch a window with the extension loaded, opening `../../examples`.
