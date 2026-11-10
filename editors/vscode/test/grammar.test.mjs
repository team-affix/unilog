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

/**
 * The most specific scope of each non-blank token, over one or more lines tokenized in sequence (so a region
 * like a proof can continue onto the next line). Plain text (no scope, or only a meta.* region) is ''.
 */
function scopes(...lines) {
    let state = vsctm.INITIAL;
    const out = [];
    for (const line of lines) {
        const { tokens, ruleStack } = grammar.tokenizeLine(line, state);
        state = ruleStack;
        for (const t of tokens) {
            const text = line.slice(t.startIndex, t.endIndex).trim();
            if (text === '') continue;
            const scope = t.scopes.at(-1);
            out.push([text, scope === 'source.unilog' || scope.startsWith('meta.') ? '' : scope]);
        }
    }
    return out;
}

const PROP_VAR = 'entity.name.type.parameter.unilog';
const ASSUMPTION = 'variable.parameter.assumption.unilog';

const cases = [
    [
        ['import Kant.Ethics.'],
        [['import', 'keyword.control.import.unilog'], ['Kant.Ethics', 'entity.name.namespace.unilog'], ['.', 'punctuation.terminator.unilog']],
    ],
    [
        ['b : likes X cake => likes X sugar.  # a rule'],
        [
            ['b', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['likes', ''],
            ['X', PROP_VAR],
            ['cake', ''],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['likes', ''],
            ['X', PROP_VAR],
            ['sugar', ''],
            ['.', 'punctuation.terminator.unilog'],
            ['# a rule', 'comment.line.number-sign.unilog'],
        ],
    ],
    [
        // Proposition variables (A, B) and assumptions (P) get different scopes, on the same line.
        ['and_comm : and A B => and B A = P => pair (snd P) (fst P).'],
        [
            ['and_comm', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['and', ''],
            ['A', PROP_VAR],
            ['B', PROP_VAR],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['and', ''],
            ['B', PROP_VAR],
            ['A', PROP_VAR],
            ['=', 'keyword.operator.equals.unilog'],
            ['P', ASSUMPTION],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['pair', ''],
            ['(', 'punctuation.parenthesis.unilog'],
            ['snd', ''],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['fst', ''],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
    [
        ['mine = Kant.Ethics.duty (P).'],
        [
            ['mine', 'entity.name.function.unilog'],
            ['=', 'keyword.operator.equals.unilog'],
            ['Kant.Ethics.', 'entity.name.namespace.unilog'],
            ['duty', 'variable.other.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
    [
        // A proof continuing onto the next line: still a proof, so Nnp and P are assumptions.
        // After the '.', the next declaration's variable is a proposition variable again.
        ['double_negation : ((P => false) => false) => P =', '    Nnp => case lem (P => P).', 'next : X.'],
        [
            ['double_negation', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['P', PROP_VAR],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['false', ''],
            [')', 'punctuation.parenthesis.unilog'],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['false', ''],
            [')', 'punctuation.parenthesis.unilog'],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['P', PROP_VAR],
            ['=', 'keyword.operator.equals.unilog'],
            ['Nnp', ASSUMPTION],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['case lem', ''],
            ['(', 'punctuation.parenthesis.unilog'],
            ['P', ASSUMPTION],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
            ['next', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['X', PROP_VAR],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
];

for (const [lines, expected] of cases) assert.deepEqual(scopes(...lines), expected, lines.join('\n'));
console.log(`grammar: ${cases.length} cases passed`);
