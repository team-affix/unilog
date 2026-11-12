import fs from 'fs';
import path from 'path';
import { Declaration, Derivation, keyOf, propositionTerm } from './check';
import { UnilogError } from './errors';
import { parseProposition } from './parser';
import { Printer, proves, Term } from './term';
import { DEFAULT_CONFIG, PROJECT_FILE, targetOf, Workspace } from './workspace';

/** The outcome of a command: lines for stdout and stderr, and whether it succeeded. */
export interface Output {
    ok: boolean;
    out: string[];
    err: string[];
}

function guarded(fn: (out: string[]) => void): Output {
    const out: string[] = [];
    try {
        fn(out);
        return { ok: true, out, err: [] };
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out, err: [e.message] };
    }
}

const prop = (t: Parameters<Printer['print']>[0]) => new Printer().print(t);

/** A loaded project: its workspace, which modules are its own code (from `check`), and any errors. */
interface Loaded {
    ws: Workspace;
    own: Set<string>;
    errors: string[];
}

function load(dir: string): Loaded {
    const target = targetOf(dir);
    const ws = new Workspace(target.roots);
    const own = new Set<string>();
    const errors: string[] = [];
    for (const file of target.files) {
        try {
            own.add(ws.moduleNameOf(file));
            ws.loadFile(file);
        } catch (e) {
            if (!(e instanceof UnilogError)) throw e;
            if (!errors.includes(e.message)) errors.push(e.message);
        }
    }
    return { ws, own, errors };
}

const isOwn = (l: Loaded, d: Declaration) => l.own.has(d.module);
const sorted = (l: Loaded) => l.ws.loaded().sort((a, b) => a.name.localeCompare(b.name));

export interface CheckOptions {
    /** Print every declaration and what it proves. */
    verbose?: boolean;
}

/**
 * `unilog check [dir]`: checks a project as configured by its `unilog.json` (see `targetOf`), or every `.ul`
 * file in a directory without one. Reports every failing module, not just the first.
 */
export function check(dir: string, options: CheckOptions = {}): Output {
    let l: Loaded;
    try {
        l = load(dir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out: [], err: [e.message] };
    }
    const out: string[] = [];
    const modules = sorted(l);
    const decls = modules.flatMap((m) => [...m.decls.values()]);
    if (options.verbose)
        for (const d of decls)
            out.push(`${d.kind === 'axiom' ? 'axiom    ' : 'inference'} ${keyOf(d)} : ${prop(d.prop)}`);
    if (l.errors.length > 0) return { ok: false, out, err: l.errors };

    const mine = decls.filter((d) => isOwn(l, d));
    const myAxioms = mine.filter((d) => d.kind === 'axiom');
    const imported = modules.length - l.own.size;
    out.push(
        `ok: ${l.own.size} module(s) checked, declaring ${mine.length - myAxioms.length} inference(s) and ` +
            `${myAxioms.length} axiom(s); ${imported} imported module(s)`,
    );
    return { ok: true, out, err: [] };
}

export interface AxiomsOptions {
    /** Include every module the module imports, transitively (each module counted once). */
    transitive?: boolean;
    /** Print only the number. */
    count?: boolean;
    /** Print JSON. */
    json?: boolean;
}

/**
 * `unilog axioms <Module> [dir]`: the axioms declared in a module, or with `transitive`, in the module and
 * everything it imports. Only parses: nothing is checked, so it is cheap and works independently of
 * `check`.
 */
export function axioms(module: string, dir: string, options: AxiomsOptions = {}): Output {
    return guarded((out) => {
        const ws = new Workspace(targetOf(dir).roots);
        const modules = options.transitive ? ws.outlineClosure(module) : [ws.outline(module)];
        const list = modules.flatMap((m) => m.axioms);
        if (options.count) {
            out.push(String(list.length));
        } else if (options.json) {
            out.push(
                JSON.stringify(
                    {
                        module,
                        transitive: !!options.transitive,
                        count: list.length,
                        modules: modules.map((m) => ({ name: m.name, axioms: m.axioms.length })),
                        axioms: list.map((a) => ({
                            name: a.name,
                            module: a.module,
                            proposition: a.proposition,
                            file: a.loc.file,
                            line: a.loc.line,
                        })),
                    },
                    null,
                    2,
                ),
            );
        } else {
            for (const a of list) out.push(`${a.name} : ${a.proposition}`);
            out.push(
                options.transitive
                    ? `${list.length} axiom(s) in ${module} and the ${modules.length - 1} module(s) it imports`
                    : `${list.length} axiom(s) in ${module}`,
            );
        }
    });
}

/**
 * `unilog rests-on <Mod.name> [dir]`: every axiom a declaration ultimately rests on, following its proof
 * through all modules. Unlike `axioms`, this checks the modules involved, since it follows proofs.
 */
export function restsOn(name: string, dir: string, json = false): Output {
    return guarded((out) => {
        const list = new Workspace(targetOf(dir).roots).axiomsOf(name);
        if (json) {
            out.push(
                JSON.stringify(
                    {
                        declaration: name,
                        axioms: list.map((a) => ({
                            name: keyOf(a),
                            module: a.module,
                            proposition: prop(a.prop),
                        })),
                    },
                    null,
                    2,
                ),
            );
            return;
        }
        if (list.length === 0) out.push(`${name} rests on no axioms`);
        for (const a of list) out.push(`${keyOf(a)} : ${prop(a.prop)}`);
    });
}

/** `unilog show <Mod.name> [dir]`: what a declaration proves. */
export function show(name: string, dir: string): Output {
    return guarded((out) => {
        const d = new Workspace(targetOf(dir).roots).declaration(name);
        out.push(`${d.kind} ${keyOf(d)} : ${prop(d.prop)}`);
    });
}

/** One module in a summary. */
export interface ModuleSummary {
    name: string;
    file: string;
    /** True for the project's own (checked) modules, false for imported ones. */
    own: boolean;
    imports: string[];
    axioms: number;
    inferences: number;
    declarations: {
        name: string;
        kind: 'axiom' | 'inference';
        proposition: string;
        /** Declarations its proof refers to directly. */
        uses: string[];
        /** Every axiom it ultimately rests on. */
        restsOn: string[];
    }[];
}

/**
 * `unilog summary [dir]`: every module in the project and everything it imports, with axiom and inference
 * counts. With --json, the full dependency graph between declarations, for analysis and visualization.
 */
export function summary(dir: string, json = false): Output {
    let l: Loaded;
    try {
        l = load(dir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out: [], err: [e.message] };
    }
    if (l.errors.length > 0) return { ok: false, out: [], err: l.errors };
    const modules: ModuleSummary[] = sorted(l).map((m) => {
        const decls = [...m.decls.values()];
        return {
            name: m.name,
            file: m.file,
            own: l.own.has(m.name),
            imports: [...m.imports],
            axioms: decls.filter((d) => d.kind === 'axiom').length,
            inferences: decls.filter((d) => d.kind === 'inference').length,
            declarations: decls.map((d) => ({
                name: keyOf(d),
                kind: d.kind,
                proposition: prop(d.prop),
                uses: [...d.uses],
                restsOn: l.ws.axiomsOf(keyOf(d)).map(keyOf),
            })),
        };
    });
    if (json) return { ok: true, out: [JSON.stringify({ modules }, null, 2)], err: [] };

    const width = Math.max(6, ...modules.map((m) => m.name.length));
    const out = [`${'module'.padEnd(width)}  ${'source'.padEnd(8)}  axioms  inferences`];
    for (const m of modules)
        out.push(
            `${m.name.padEnd(width)}  ${(m.own ? 'checked' : 'imported').padEnd(8)}  ${String(m.axioms).padStart(6)}  ${String(m.inferences).padStart(10)}`,
        );
    const total = (own: boolean) => modules.filter((m) => m.own === own).reduce((n, m) => n + m.axioms, 0);
    out.push(`axioms: ${total(true)} in checked code, ${total(false)} in imported modules`);
    return { ok: true, out, err: [] };
}

export interface InitOptions {
    /** Search roots to write (default: ["."]). */
    roots?: string[];
    /** Paths to check (default: none yet). */
    check?: string[];
}

/** `unilog init [dir]`: writes a starting `unilog.json`. Never overwrites an existing one. */
export function init(dir: string, options: InitOptions = {}): Output {
    return guarded((out) => {
        const file = path.join(dir, PROJECT_FILE);
        if (fs.existsSync(file)) throw new UnilogError(`${file} already exists`);
        const config = { roots: options.roots ?? DEFAULT_CONFIG.roots, check: options.check ?? DEFAULT_CONFIG.check };
        fs.mkdirSync(dir, { recursive: true });
        fs.writeFileSync(file, JSON.stringify(config, null, 4) + '\n');
        out.push(
            config.check.length === 0
                ? `Created ${file}. List the folders to check under "check", and add any dependency folders to "roots".`
                : `Created ${file}.`,
        );
    });
}

export interface FindOptions {
    /**
     * Match only declarations of exactly the query's generality: the declaration proves the query and the
     * query proves the declaration. `\\Y. likes Y cake => likes Y sugar` matches
     * `\\X. likes X cake => likes X sugar` (only the variable names differ) but not its special case
     * `likes jake cake => likes jake sugar`.
     */
    exact?: boolean;
    json?: boolean;
}

/**
 * `unilog find <proposition> [dir]`: which declarations, in the project and everything it imports, prove
 * the proposition. For example `unilog find false` finds any proof of a contradiction. Searches the
 * declarations that exist; it does not search for new proofs. Fails (non-zero exit) if none is found.
 */
export function find(query: string, dir: string, options: FindOptions = {}): Output {
    let l: Loaded;
    let q: Term;
    try {
        q = propositionTerm(parseProposition(query));
        l = load(dir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out: [], err: [e.message] };
    }
    if (l.errors.length > 0) return { ok: false, out: [], err: l.errors };

    const matches = options.exact
        ? (d: Declaration) => proves(d.prop, q) && proves(q, d.prop)
        : (d: Declaration) => proves(d.prop, q);
    const found = sorted(l)
        .flatMap((m) => [...m.decls.values()])
        .filter(matches);

    const shown = new Printer().print(q);
    if (options.json)
        return {
            ok: found.length > 0,
            out: [
                JSON.stringify(
                    {
                        query: shown,
                        found: found.map((d) => ({
                            name: keyOf(d),
                            kind: d.kind,
                            module: d.module,
                            own: l.own.has(d.module),
                            proposition: prop(d.prop),
                        })),
                    },
                    null,
                    2,
                ),
            ],
            err: [],
        };
    if (found.length === 0) return { ok: false, out: [], err: [`No declaration proves ${shown}`] };
    return {
        ok: true,
        out: found.map((d) => `${d.kind === 'axiom' ? 'axiom    ' : 'inference'} ${keyOf(d)} : ${prop(d.prop)}`),
        err: [],
    };
}

export interface InferencesOptions {
    /** Include every module it imports, transitively. */
    transitive?: boolean;
    count?: boolean;
    json?: boolean;
}

/**
 * `unilog inferences <Module> [dir]`: the inferences declared in a module (or with `transitive`, in it and
 * everything it imports), with what each proves. Proofs are not shown. Checks the modules involved,
 * since what an inference proves is inferred.
 */
export function inferences(module: string, dir: string, options: InferencesOptions = {}): Output {
    return guarded((out) => {
        const ws = new Workspace(targetOf(dir).roots);
        const modules = [ws.load(module)];
        if (options.transitive)
            // Visits modules appended during the loop too, so this walks the whole import graph.
            for (const mod of modules)
                for (const imp of mod.imports) if (!modules.some((m) => m.name === imp)) modules.push(ws.load(imp));
        const list = modules.flatMap((m) => [...m.decls.values()].filter((d) => d.kind === 'inference'));
        if (options.count) {
            out.push(String(list.length));
        } else if (options.json) {
            out.push(
                JSON.stringify(
                    {
                        module,
                        transitive: !!options.transitive,
                        count: list.length,
                        inferences: list.map((d) => ({
                            name: keyOf(d),
                            module: d.module,
                            proposition: prop(d.prop),
                        })),
                    },
                    null,
                    2,
                ),
            );
        } else {
            for (const d of list) out.push(`${keyOf(d)} : ${prop(d.prop)}`);
            out.push(
                options.transitive
                    ? `${list.length} inference(s) in ${module} and the ${modules.length - 1} module(s) it imports`
                    : `${list.length} inference(s) in ${module}`,
            );
        }
    });
}

/** One step of a proof, as shown by `unilog proof`. */
export interface ProofNode {
    /** The proof expression at this step, as written. */
    expr: string;
    /** What this step proves, with variables as the proof filled them in. */
    proposition: string;
    kind: 'axiom' | 'inference' | 'assumption' | 'apply' | 'assume';
    /** For axioms and inferences: the qualified name. */
    name?: string;
    /** For an expanded inference: its own proof expression. */
    proof?: string;
    /** An inference already expanded earlier in this tree, not expanded again. */
    shownAbove?: boolean;
    /**
     * How this step follows from its children: `apply` (the first child, an implication, applied to the
     * second, its premise), `assume` (the child was proved with `assumption` assumed, giving an
     * implication), or `same` (an inference defined as the child).
     */
    rule?: 'apply' | 'assume' | 'same';
    /** For `assume`: the name of the assumption. */
    assumption?: string;
    children: ProofNode[];
}

/** How a derivation's root follows from its children. */
function ruleOf(dv: Derivation): Pick<ProofNode, 'rule' | 'assumption'> {
    if (dv.k === 'apply') return { rule: 'apply' };
    if (dv.k === 'assume') return { rule: 'assume', assumption: dv.param };
    return { rule: 'same' };
}

export interface ProofOptions {
    /** Don't expand the inferences this proof uses into their own proofs. */
    shallow?: boolean;
    /**
     * Instead of a tree, list every declaration the proof relies on, each once, in declaration order (as
     * written, each module after those it imports). Lines refer to each other by name, as in the source.
     */
    flat?: boolean;
    /** With `flat`: list axioms before inferences (each group still in declaration order). */
    axiomsFirst?: boolean;
    json?: boolean;
}

/** One declaration in a flat proof listing. */
export interface ProofLine {
    name: string;
    kind: 'axiom' | 'inference';
    /** The declaration's own (general) statement. */
    proposition: string;
    /** For inferences: the proof expression, which refers to earlier lines by name. */
    proof?: string;
}

/**
 * Every declaration a proof relies on (with `shallow`: only those its own proof uses directly), plus the
 * declaration itself, in declaration order: as written in each module, each module after those it imports.
 * With `axiomsFirst`, axioms come first, each group still in declaration order.
 */
function proofLines(d: Declaration, ws: Workspace, shallow: boolean, axiomsFirst: boolean): ProofLine[] {
    const included = new Map<string, Declaration>();
    const visit = (x: Declaration) => {
        if (included.has(keyOf(x))) return;
        included.set(keyOf(x), x);
        for (const use of x.uses) if (!shallow || x === d) visit(ws.declaration(use));
    };
    visit(d);
    const ordered = [...included.values()].sort((a, b) => ws.orderOf(keyOf(a)) - ws.orderOf(keyOf(b)));
    const final = axiomsFirst
        ? [...ordered.filter((x) => x.kind === 'axiom'), ...ordered.filter((x) => x.kind === 'inference')]
        : ordered;
    return final.map((x) => {
        const l: ProofLine = { name: keyOf(x), kind: x.kind, proposition: prop(x.prop) };
        if (x.proof && (!shallow || x === d)) l.proof = x.proof.text;
        return l;
    });
}

/**
 * `unilog proof <Mod.name> [dir]`: a declaration's whole proof as a tree. Each step shows its expression and
 * what it proves at that point; leaves are axioms and assumptions. Inferences the proof uses are expanded
 * into their own proofs (each once; later uses say "shown above"), unless `shallow`.
 */
export function proof(name: string, dir: string, options: ProofOptions = {}): Output {
    return guarded((out) => {
        const ws = new Workspace(targetOf(dir).roots);
        const d = ws.declaration(name);
        const p = new Printer();
        const expanded = new Set<string>([keyOf(d)]);

        const childrenOf = (dv: Derivation): ProofNode[] =>
            dv.k === 'apply' ? [node(dv.fn), node(dv.arg)] : dv.k === 'assume' ? [node(dv.body)] : [node(dv)];

        const node = (dv: Derivation): ProofNode => {
            const base = { expr: dv.text, proposition: p.print(dv.term) };
            if (dv.k === 'apply')
                return { ...base, kind: 'apply', ...ruleOf(dv), children: [node(dv.fn), node(dv.arg)] };
            if (dv.k === 'assume') return { ...base, kind: 'assume', ...ruleOf(dv), children: [node(dv.body)] };
            const target = dv.decl;
            if (!target) return { ...base, kind: 'assumption', children: [] };
            const key = keyOf(target);
            if (target.kind === 'axiom') return { ...base, kind: 'axiom', name: key, children: [] };
            if (options.shallow || !target.proof) return { ...base, kind: 'inference', name: key, children: [] };
            if (expanded.has(key)) return { ...base, kind: 'inference', name: key, shownAbove: true, children: [] };
            expanded.add(key);
            return {
                ...base,
                kind: 'inference',
                name: key,
                proof: target.proof.text,
                ...ruleOf(target.proof),
                children: childrenOf(target.proof),
            };
        };

        const root: ProofNode = d.proof
            ? {
                  expr: keyOf(d),
                  // From the proof itself (not the generalized statement), so variable names match the steps below.
                  proposition: p.print(d.proof.term),
                  kind: 'inference',
                  name: keyOf(d),
                  proof: d.proof.text,
                  ...ruleOf(d.proof),
                  children: childrenOf(d.proof),
              }
            : { expr: keyOf(d), proposition: p.print(d.prop), kind: 'axiom', name: keyOf(d), children: [] };

        if (options.flat) {
            const lines = proofLines(d, ws, !!options.shallow, !!options.axiomsFirst);
            if (options.json) out.push(JSON.stringify(lines, null, 2));
            else
                for (const l of lines)
                    out.push(
                        l.proof
                            ? `${l.name} = ${l.proof} : ${l.proposition}`
                            : `${l.name} : ${l.proposition}   [${l.kind}]`,
                    );
            return;
        }
        if (options.json) {
            out.push(JSON.stringify(root, null, 2));
            return;
        }
        const tag = (n: ProofNode): string => {
            switch (n.kind) {
                case 'axiom':
                    return `   [axiom ${n.name}]`;
                case 'assumption':
                    return '   [assumption]';
                case 'inference':
                    if (n.shownAbove) return `   [inference ${n.name}, shown above]`;
                    return n.proof ? `   [inference ${n.name} = ${n.proof}]` : `   [inference ${n.name}]`;
                default:
                    return '';
            }
        };
        out.push(
            root.kind === 'axiom'
                ? `${root.name} : ${root.proposition}   [axiom]`
                : `${root.name} = ${root.proof} : ${root.proposition}`,
        );
        const draw = (nodes: ProofNode[], prefix: string) =>
            nodes.forEach((n, i) => {
                const last = i === nodes.length - 1;
                out.push(`${prefix}${last ? '└── ' : '├── '}${n.expr} : ${n.proposition}${tag(n)}`);
                draw(n.children, prefix + (last ? '    ' : '│   '));
            });
        draw(root.children, '');
    });
}
