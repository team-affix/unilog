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
    loadGrammar: async (scope) => {
        const file = { 'source.unilog': 'unilog.tmLanguage.json', 'source.unilog.step': 'unilog-step.tmLanguage.json' }[scope];
        if (!file) return null;
        return vsctm.parseRawGrammar(fs.readFileSync(new URL(`../syntaxes/${file}`, import.meta.url), 'utf8'), file);
    },
});
let grammar = await registry.loadGrammar('source.unilog');
const stepGrammar = await registry.loadGrammar('source.unilog.step');

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
            out.push([text, scope.startsWith('source.') || scope.startsWith('meta.') ? '' : scope]);
        }
    }
    return out;
}

const PROP_VAR = 'entity.name.type.parameter.unilog';
const ASSUMPTION = 'variable.parameter.assumption.unilog';
// Proof names, used or declared, share the theme's declaration color (entity.name.function).
const PROOF = 'entity.name.function.proof.unilog';
const LAMBDA = 'keyword.operator.lambda.unilog';
// Atoms (lowercase symbols in propositions) are colored too: they are what propositions are about.
const ATOM = 'constant.other.symbol.unilog';

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
            ['likes', ATOM],
            ['X', PROP_VAR],
            ['cake', ATOM],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['likes', ATOM],
            ['X', PROP_VAR],
            ['sugar', ATOM],
            ['.', 'punctuation.terminator.unilog'],
            ['# a rule', 'comment.line.number-sign.unilog'],
        ],
    ],
    [
        // Proposition variables (A, B) and assumptions (P) get different scopes, on the same line.
        ['and_comm : and A B => and B A = \\P -> pair (snd P) (fst P).'],
        [
            ['and_comm', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['and', ATOM],
            ['A', PROP_VAR],
            ['B', PROP_VAR],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['and', ATOM],
            ['B', PROP_VAR],
            ['A', PROP_VAR],
            ['=', 'keyword.operator.equals.unilog'],
            ['\\', LAMBDA],
            ['P', ASSUMPTION],
            ['->', 'keyword.operator.arrow.unilog'],
            ['pair', PROOF],
            ['(', 'punctuation.parenthesis.unilog'],
            ['snd', PROOF],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['fst', PROOF],
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
            ['duty', PROOF],
            ['(', 'punctuation.parenthesis.unilog'],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
    [
        // A proof continuing onto the next line: still a proof, so Nnp and P are assumptions.
        // After the '.', the next declaration's variable is a proposition variable again.
        ['double_negation : ((P => false) => false) => P =', '    \\Nnp -> case lem (\\P -> P).', 'next : X.'],
        [
            ['double_negation', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['(', 'punctuation.parenthesis.unilog'],
            ['P', PROP_VAR],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['false', ATOM],
            [')', 'punctuation.parenthesis.unilog'],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['false', ATOM],
            [')', 'punctuation.parenthesis.unilog'],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['P', PROP_VAR],
            ['=', 'keyword.operator.equals.unilog'],
            ['\\', LAMBDA],
            ['Nnp', ASSUMPTION],
            ['->', 'keyword.operator.arrow.unilog'],
            ['case', PROOF],
            ['lem', PROOF],
            ['(', 'punctuation.parenthesis.unilog'],
            ['\\', LAMBDA],
            ['P', ASSUMPTION],
            ['->', 'keyword.operator.arrow.unilog'],
            ['P', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
            ['next', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['X', PROP_VAR],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
    [
        // route_more is a proof (colored) where it is used in a proof, and route is just a symbol in a proposition.
        ['two_roads : road X Y => route X Z = \\Xy -> route_more Xy (route_road Yz).'],
        [
            ['two_roads', 'entity.name.function.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['road', ATOM],
            ['X', PROP_VAR],
            ['Y', PROP_VAR],
            ['=>', 'keyword.operator.arrow.unilog'],
            ['route', ATOM],
            ['X', PROP_VAR],
            ['Z', PROP_VAR],
            ['=', 'keyword.operator.equals.unilog'],
            ['\\', LAMBDA],
            ['Xy', ASSUMPTION],
            ['->', 'keyword.operator.arrow.unilog'],
            ['route_more', PROOF],
            ['Xy', ASSUMPTION],
            ['(', 'punctuation.parenthesis.unilog'],
            ['route_road', PROOF],
            ['Yz', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            ['.', 'punctuation.terminator.unilog'],
        ],
    ],
];

for (const [lines, expected] of cases) assert.deepEqual(scopes(...lines), expected, lines.join('\n'));

// Hover steps (`proof : proposition`) use the step grammar: the proof part is colored as a proof.
const stepCases = [
    [
        ['route_more Xy (route_road Yz) : route X Z'],
        [
            ['route_more', PROOF],
            ['Xy', ASSUMPTION],
            ['(', 'punctuation.parenthesis.unilog'],
            ['route_road', PROOF],
            ['Yz', ASSUMPTION],
            [')', 'punctuation.parenthesis.unilog'],
            [':', 'keyword.operator.colon.unilog'],
            ['route', ATOM],
            ['X', PROP_VAR],
            ['Z', PROP_VAR],
        ],
    ],
    [
        ['P : and A B'],
        [
            ['P', ASSUMPTION],
            [':', 'keyword.operator.colon.unilog'],
            ['and', ATOM],
            ['A', PROP_VAR],
            ['B', PROP_VAR],
        ],
    ],
];
grammar = stepGrammar;
for (const [lines, expected] of stepCases) assert.deepEqual(scopes(...lines), expected, lines.join('\n'));
console.log(`grammar: ${cases.length + stepCases.length} cases passed`);
