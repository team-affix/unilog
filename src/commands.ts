import fs from 'fs';
import path from 'path';
import { Declaration, keyOf, propositionTerm } from './check';
import { UnilogError } from './errors';
import { parseProposition } from './parser';
import { instantiate, instantiateRigid, Printer, unify } from './term';
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
            out.push(`${d.kind === 'axiom' ? 'axiom  ' : 'theorem'} ${keyOf(d)} : ${prop(d.scheme.body)}`);
    if (l.errors.length > 0) return { ok: false, out, err: l.errors };

    const mine = decls.filter((d) => isOwn(l, d));
    const myAxioms = mine.filter((d) => d.kind === 'axiom');
    const imported = modules.length - l.own.size;
    out.push(
        `ok: ${l.own.size} module(s) checked, declaring ${mine.length - myAxioms.length} theorem(s) and ` +
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
                            proposition: prop(a.scheme.body),
                        })),
                    },
                    null,
                    2,
                ),
            );
            return;
        }
        if (list.length === 0) out.push(`${name} rests on no axioms`);
        for (const a of list) out.push(`${keyOf(a)} : ${prop(a.scheme.body)}`);
    });
}

/** `unilog show <Mod.name> [dir]`: what a declaration proves. */
export function show(name: string, dir: string): Output {
    return guarded((out) => {
        const d = new Workspace(targetOf(dir).roots).declaration(name);
        out.push(`${d.kind} ${keyOf(d)} : ${prop(d.scheme.body)}`);
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
    theorems: number;
    declarations: {
        name: string;
        kind: 'axiom' | 'theorem';
        proposition: string;
        /** Declarations its proof refers to directly. */
        uses: string[];
        /** Every axiom it ultimately rests on. */
        restsOn: string[];
    }[];
}

/**
 * `unilog summary [dir]`: every module in the project and everything it imports, with axiom and theorem
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
            theorems: decls.filter((d) => d.kind === 'theorem').length,
            declarations: decls.map((d) => ({
                name: keyOf(d),
                kind: d.kind,
                proposition: prop(d.scheme.body),
                uses: [...d.uses],
                restsOn: l.ws.axiomsOf(keyOf(d)).map(keyOf),
            })),
        };
    });
    if (json) return { ok: true, out: [JSON.stringify({ modules }, null, 2)], err: [] };

    const width = Math.max(6, ...modules.map((m) => m.name.length));
    const out = [`${'module'.padEnd(width)}  ${'source'.padEnd(8)}  axioms  theorems`];
    for (const m of modules)
        out.push(
            `${m.name.padEnd(width)}  ${(m.own ? 'checked' : 'imported').padEnd(8)}  ${String(m.axioms).padStart(6)}  ${String(m.theorems).padStart(8)}`,
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
     * Match declarations whose proposition merely fits the query's shape, with the query's variables
     * standing for anything (e.g. `wrong X` finds `wrong bob_party`). Without it, a declaration must prove
     * the query in full generality (e.g. `wrong X` only matches a proof that everything is wrong).
     */
    pattern?: boolean;
    /**
     * Match only declarations of exactly the query's generality: the declaration proves the query and the
     * query proves the declaration. `likes Y cake => likes Y sugar` matches `likes X cake => likes X sugar`
     * (only the variable names differ) but not its special case `likes jake cake => likes jake sugar`.
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
    if (options.pattern && options.exact)
        return { ok: false, out: [], err: ['--pattern and --exact cannot be combined'] };
    let l: Loaded;
    let expr: ReturnType<typeof parseProposition>;
    try {
        expr = parseProposition(query);
        l = load(dir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out: [], err: [e.message] };
    }
    if (l.errors.length > 0) return { ok: false, out: [], err: l.errors };

    // Every check works on fresh copies: a declaration's variables are instantiated anew for each test,
    // and the query is rebuilt whenever its variables are allowed to bind, so no test can affect another.
    // "a proves b" = a's variables may bind, b's are rigid (fixed but arbitrary).
    const declProvesQuery = (d: Declaration) => unify(instantiate(d.scheme), propositionTerm(expr, true)) === undefined;
    const queryProvesDecl = (d: Declaration) => unify(propositionTerm(expr), instantiateRigid(d.scheme)) === undefined;
    const shapeMatches = (d: Declaration) => unify(instantiate(d.scheme), propositionTerm(expr)) === undefined;
    const matches = options.pattern
        ? shapeMatches
        : options.exact
          ? (d: Declaration) => declProvesQuery(d) && queryProvesDecl(d)
          : declProvesQuery;
    const found = sorted(l)
        .flatMap((m) => [...m.decls.values()])
        .filter(matches);

    const shown = new Printer().print(propositionTerm(expr));
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
                            proposition: prop(d.scheme.body),
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
        out: found.map((d) => `${d.kind === 'axiom' ? 'axiom  ' : 'theorem'} ${keyOf(d)} : ${prop(d.scheme.body)}`),
        err: [],
    };
}

export interface TheoremsOptions {
    /** Include every module it imports, transitively. */
    transitive?: boolean;
    count?: boolean;
    json?: boolean;
}

/**
 * `unilog theorems <Module> [dir]`: the theorems declared in a module (or with `transitive`, in it and
 * everything it imports), with what each proves. Proofs are not shown. Checks the modules involved,
 * since what a theorem proves is inferred.
 */
export function theorems(module: string, dir: string, options: TheoremsOptions = {}): Output {
    return guarded((out) => {
        const ws = new Workspace(targetOf(dir).roots);
        const modules = [ws.load(module)];
        if (options.transitive)
            // Visits modules appended during the loop too, so this walks the whole import graph.
            for (const mod of modules)
                for (const imp of mod.imports) if (!modules.some((m) => m.name === imp)) modules.push(ws.load(imp));
        const list = modules.flatMap((m) => [...m.decls.values()].filter((d) => d.kind === 'theorem'));
        if (options.count) {
            out.push(String(list.length));
        } else if (options.json) {
            out.push(
                JSON.stringify(
                    {
                        module,
                        transitive: !!options.transitive,
                        count: list.length,
                        theorems: list.map((d) => ({
                            name: keyOf(d),
                            module: d.module,
                            proposition: prop(d.scheme.body),
                        })),
                    },
                    null,
                    2,
                ),
            );
        } else {
            for (const d of list) out.push(`${keyOf(d)} : ${prop(d.scheme.body)}`);
            out.push(
                options.transitive
                    ? `${list.length} theorem(s) in ${module} and the ${modules.length - 1} module(s) it imports`
                    : `${list.length} theorem(s) in ${module}`,
            );
        }
    });
}
