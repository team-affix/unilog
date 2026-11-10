// Tokenizes sample lines with the same engine VS Code uses (vscode-textmate) and checks the scopes.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { createRequire } from 'node:module';
import vsctm from 'vscode-textmate';
import oniguruma from 'vscode-oniguruma';

const require = createRequire(import.meta.url);
const wasm = fs.readFileSync(require.resolve('vscode-oniguruma/release/onig.wasm')).buffer;
await oniguruma.loadWASM(wasm);

const registry = new vsctm.Registry({
    onigLib: Promise.resolve({
        createOnigScanner: (p) => new oniguruma.OnigScanner(p),
        createOnigString: (s) => new oniguruma.OnigString(s),
    }),
    loadGrammar: async () =>
        vsctm.parseRawGrammar(fs.readFileSync(new URL('../syntaxes/unilog.tmLanguage.json', import.meta.url), 'utf8'), 'g.json'),
});
const grammar = await registry.loadGrammar('source.unilog');

/** The scope (beyond source.unilog) of each non-blank token in a line. */
function scopes(line) {
    const { tokens } = grammar.tokenizeLine(line, vsctm.INITIAL);
    return tokens
        .map((t) => [line.slice(t.startIndex, t.endIndex).trim(), t.scopes.slice(1).join(' ')])
        .filter(([text]) => text.trim() !== '');
}

const cases = [
    [
        'import Kant.Ethics.',
        [['import', 'keyword.control.import.unilog'], ['Kant.Ethics', 'entity.name.namespace.unilog'], ['.', 'punctuation.terminator.unilog']],
    ],
    [
        'b : likes X cake => likes X sugar.  # a rule',
        [
            ['b', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['likes', ''],
            ['X', 'variable.parameter.unilog'],
            ['cake', ''],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['likes', ''],
            ['X', 'variable.parameter.unilog'],
            ['sugar', ''],
            ['.', 'punctuation.terminator.unilog'],
            ['# a rule', 'comment.line.number-sign.unilog'],
        ],
    ],
    [
        'mine = Kant.Ethics.duty (p).',
        [
            ['mine', 'entity.name.function.unilog'],
            ['=', 'keyword.operator.equals.unilog'],
            ['Kant.Ethics.', 'entity.name.namespace.unilog'],
            ['duty', 'variable.other.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['p', ''],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
    [
        // A continuation line starting with an assumption is a variable, not a declaration name.
        '    Nnp => case lem.',
        [
            ['Nnp', 'variable.parameter.unilog'],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['case lem', ''],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
];

for (const [line, expected] of cases) assert.deepEqual(scopes(line), expected, line);
console.log(`grammar: ${cases.length} cases passed`);
