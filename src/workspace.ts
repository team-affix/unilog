import fs from 'fs';
import path from 'path';
import { checkModule, Declaration, keyOf, Module } from './check';
import { Loc, UnilogError } from './errors';
import { parse } from './parser';
import { PropExpr } from './syntax';

export const EXTENSION = '.ul';

/**
 * Loads and checks modules by name.
 *
 * Module `Pkg.Sub.File` is the file `Pkg/Sub/File.ul` under one of the search roots. Every import, from
 * any module, is resolved against the same roots: those of the project being checked. A module never
 * looks for its imports relative to its own location, so a package's files work unchanged wherever they
 * sit (as the project itself, or alongside other packages in a dependency folder).
 */
/** An axiom as written in a module, read without checking. */
export interface DeclaredAxiom {
    /** `Module.name` */
    name: string;
    module: string;
    /** The proposition as written. */
    proposition: string;
    loc: Loc;
}

/** A module read without checking: only what it imports and which axioms it declares. */
export interface ModuleOutline {
    name: string;
    file: string;
    imports: string[];
    /** Where each import is written, parallel to `imports`. */
    importLocs: Loc[];
    axioms: DeclaredAxiom[];
}

/** Prints proposition syntax (as written, before checking). */
export function showProp(e: PropExpr): string {
    switch (e.k) {
        case 'atom':
        case 'var':
            return e.name;
        case 'app': {
            const fn = e.fn.k === 'imp' ? `(${showProp(e.fn)})` : showProp(e.fn);
            const arg = e.arg.k === 'app' || e.arg.k === 'imp' ? `(${showProp(e.arg)})` : showProp(e.arg);
            return `${fn} ${arg}`;
        }
        case 'imp':
            return `${e.from.k === 'imp' ? `(${showProp(e.from)})` : showProp(e.from)} => ${showProp(e.to)}`;
    }
}

/**
 * Where module source text comes from. The default reads the disk; an editor passes one that returns
 * the text of open (possibly unsaved) documents and falls back to the disk otherwise.
 */
export interface SourceProvider {
    exists(file: string): boolean;
    read(file: string): string;
}

export const diskSources: SourceProvider = {
    exists: (file) => fs.existsSync(file),
    read: (file) => fs.readFileSync(file, 'utf8'),
};

export class Workspace {
    private readonly modules = new Map<string, Module>();
    /**
     * Every checked declaration's key, in the order checked: each module's declarations in source order,
     * after the modules it imports. This is "declaration order" across modules.
     */
    private readonly declOrder = new Map<string, number>();
    /** Modules that failed, so each failure is reported once however many modules import it. */
    private readonly failed = new Map<string, UnilogError>();
    private readonly outlines = new Map<string, ModuleOutline>();
    private readonly loading: string[] = [];

    constructor(
        public readonly roots: readonly string[],
        private readonly sources: SourceProvider = diskSources,
    ) {}

    /** The file for a module name, or undefined if no root has it. */
    locate(name: string): string | undefined {
        const rel = path.join(...name.split('.')) + EXTENSION;
        const found = this.roots.map((r) => path.join(r, rel)).filter((f) => this.sources.exists(f));
        if (found.length > 1) throw new UnilogError(`Module ${name} is ambiguous: it exists at ${found.join(' and ')}`);
        return found[0];
    }

    /** The module name of a file under one of the roots. */
    moduleNameOf(file: string): string {
        const abs = path.resolve(file);
        for (const root of this.roots) {
            const rel = path.relative(path.resolve(root), abs);
            if (!rel.startsWith('..') && !path.isAbsolute(rel) && rel.endsWith(EXTENSION)) {
                const segments = rel.slice(0, -EXTENSION.length).split(path.sep);
                for (const s of segments)
                    if (!/^[A-Za-z0-9_']+$/.test(s))
                        throw new UnilogError(`'${file}' cannot be a module: '${s}' is not a valid module name part`);
                return segments.join('.');
            }
        }
        throw new UnilogError(`${file} is not under any search root (${this.roots.join(', ')})`);
    }

    /** Loads and checks a module and everything it imports. Each module is checked once. */
    load(name: string, from?: Loc): Module {
        const done = this.modules.get(name);
        if (done) return done;
        const failure = this.failed.get(name);
        if (failure) throw failure;
        if (this.loading.includes(name))
            throw new UnilogError(
                `Import cycle: ${[...this.loading.slice(this.loading.indexOf(name)), name].join(' -> ')}`,
                from,
            );

        const file = this.locate(name);
        if (!file) throw new UnilogError(`Module ${name} not found (looked in ${this.roots.join(', ')})`, from);

        this.loading.push(name);
        try {
            const decls = parse(this.sources.read(file), file);
            const imported = new Map<string, Module>();
            for (const d of decls)
                if (d.k === 'import') imported.set(d.path.join('.'), this.load(d.path.join('.'), d.loc));
            const mod = checkModule(name, file, decls, imported);
            this.modules.set(name, mod);
            for (const d of mod.decls.values()) this.declOrder.set(keyOf(d), this.declOrder.size);
            return mod;
        } catch (e) {
            if (!(e instanceof UnilogError)) throw e;
            // The innermost failing module adds how it was reached; modules further out pass it on unchanged.
            const err = e.context || this.loading.length < 2 ? e : new UnilogError(e.reason, e.loc, this.chain());
            this.failed.set(name, err);
            throw err;
        } finally {
            this.loading.pop();
        }
    }

    /**
     * Reads a module's imports and declared axioms without checking anything. Much cheaper than `load`,
     * and works on modules that would fail to check.
     */
    outline(name: string, from?: Loc): ModuleOutline {
        const cached = this.outlines.get(name);
        if (cached) return cached;
        const file = this.locate(name);
        if (!file) throw new UnilogError(`Module ${name} not found (looked in ${this.roots.join(', ')})`, from);
        const decls = parse(this.sources.read(file), file);
        const outline: ModuleOutline = {
            name,
            file,
            imports: decls.flatMap((d) => (d.k === 'import' ? [d.path.join('.')] : [])),
            importLocs: decls.flatMap((d) => (d.k === 'import' ? [d.loc] : [])),
            axioms: decls.flatMap((d) =>
                d.k === 'axiom'
                    ? [{ name: `${name}.${d.name}`, module: name, proposition: showProp(d.prop), loc: d.loc }]
                    : [],
            ),
        };
        this.outlines.set(name, outline);
        return outline;
    }

    /** Outlines of a module and every module it imports, transitively, each once, in visiting order. */
    outlineClosure(name: string): ModuleOutline[] {
        const seen = new Map<string, ModuleOutline>();
        const visit = (n: string, from?: Loc) => {
            if (seen.has(n)) return;
            const o = this.outline(n, from);
            seen.set(n, o);
            o.imports.forEach((imp, i) => visit(imp, o.importLocs[i]));
        };
        visit(name);
        return [...seen.values()];
    }

    /** The current import chain, innermost first: "in A, imported by B, imported by C". */
    private chain(): string {
        const [inner, ...outer] = [...this.loading].reverse();
        return [`in ${inner}`, ...outer.map((m) => `imported by ${m}`)].join(', ');
    }

    /** Loads the module in a file. */
    loadFile(file: string): Module {
        return this.load(this.moduleNameOf(file));
    }

    /** The checked module whose source is `file`, if any. */
    moduleAt(file: string): Module | undefined {
        const abs = path.resolve(file);
        return [...this.modules.values()].find((m) => path.resolve(m.file) === abs);
    }

    /** Every module checked so far. */
    loaded(): Module[] {
        return [...this.modules.values()];
    }

    /** A loaded declaration's position in declaration order (see `declOrder`). */
    orderOf(key: string): number {
        const n = this.declOrder.get(key);
        if (n === undefined) throw new UnilogError(`Declaration ${key} is not loaded`);
        return n;
    }

    /** Looks up `Mod.name` among loaded modules, loading `Mod` if needed. */
    declaration(qualified: string): Declaration {
        const at = qualified.lastIndexOf('.');
        if (at < 0) throw new UnilogError(`Expected a qualified name like Module.name, got '${qualified}'`);
        const mod = this.load(qualified.slice(0, at));
        const d = mod.decls.get(qualified.slice(at + 1));
        if (!d) throw new UnilogError(`Module ${mod.name} has no declaration '${qualified.slice(at + 1)}'`);
        return d;
    }

    /**
     * Every axiom a declaration ultimately rests on, following proofs through all modules.
     * This is the complete list of beliefs you must accept to accept the declaration.
     */
    axiomsOf(qualified: string): Declaration[] {
        const seen = new Set<string>();
        const axioms: Declaration[] = [];
        const visit = (d: Declaration) => {
            const key = keyOf(d);
            if (seen.has(key)) return;
            seen.add(key);
            if (d.kind === 'axiom') axioms.push(d);
            for (const use of d.uses) visit(this.declaration(use));
        };
        visit(this.declaration(qualified));
        return axioms.sort((a, b) => keyOf(a).localeCompare(keyOf(b)));
    }
}

/** Every `.ul` file under a directory, sorted. */
export function findSources(dir: string): string[] {
    const out: string[] = [];
    const walk = (d: string) => {
        for (const e of fs.readdirSync(d, { withFileTypes: true })) {
            const p = path.join(d, e.name);
            if (e.isDirectory()) walk(p);
            else if (e.isFile() && e.name.endsWith(EXTENSION)) out.push(p);
        }
    };
    walk(dir);
    return out.sort();
}

/** The project file. */
export const PROJECT_FILE = 'unilog.json';

/**
 * A project's configuration (`unilog.json` in the project directory). Paths are relative to that directory.
 *
 *   {
 *       "roots": [".", "deps"],     where modules are looked up, in order
 *       "check": ["Ethics"]         folders or .ul files that `unilog check` checks
 *   }
 *
 * Only the configuration of the project being checked applies. Modules it imports are found through its
 * roots, wherever they came from; any project files they were published with are ignored.
 */
export interface ProjectConfig {
    roots: string[];
    check: string[];
}

/** What to check: the search roots and the files to check, as paths. */
export interface Target {
    roots: string[];
    files: string[];
}

function stringList(value: unknown, field: string, file: string): string[] {
    if (!Array.isArray(value) || !value.every((v) => typeof v === 'string'))
        throw new UnilogError(`${file}: '${field}' must be an array of paths`);
    return value as string[];
}

/** The configuration `unilog init` writes: look up modules in the project directory, check nothing yet. */
export const DEFAULT_CONFIG: ProjectConfig = { roots: ['.'], check: [] };

/** Reads and validates a project file. */
export function readProjectConfig(file: string): ProjectConfig {
    let raw: unknown;
    try {
        raw = JSON.parse(fs.readFileSync(file, 'utf8'));
    } catch (e) {
        throw new UnilogError(`${file}: not valid JSON (${(e as Error).message})`);
    }
    if (typeof raw !== 'object' || raw === null || Array.isArray(raw))
        throw new UnilogError(`${file}: must be a JSON object with 'roots' and 'check'`);
    const obj = raw as Record<string, unknown>;
    const unknown = Object.keys(obj).filter((k) => k !== 'roots' && k !== 'check');
    if (unknown.length > 0) throw new UnilogError(`${file}: unknown field(s): ${unknown.join(', ')}`);
    return { roots: stringList(obj.roots, 'roots', file), check: stringList(obj.check, 'check', file) };
}

/**
 * Works out what to check in a directory.
 *
 * - With a `unilog.json`: its `check` paths, with its `roots` as search roots. Roots that don't exist
 *   (e.g. a dependency folder before anything is installed) are skipped.
 * - Without one: every `.ul` file under the directory, with the directory as the only root.
 */
export function targetOf(dir: string): Target {
    const file = path.join(dir, PROJECT_FILE);
    if (!fs.existsSync(file)) return { roots: [dir], files: findSources(dir) };

    const config = readProjectConfig(file);
    const roots = config.roots.map((r) => path.join(dir, r)).filter((r) => fs.existsSync(r));
    const files = config.check.flatMap((c) => {
        const p = path.join(dir, c);
        if (!fs.existsSync(p)) throw new UnilogError(`${file}: 'check' path does not exist: ${c}`);
        if (fs.statSync(p).isDirectory()) return findSources(p);
        if (!p.endsWith(EXTENSION)) throw new UnilogError(`${file}: 'check' file is not a ${EXTENSION} file: ${c}`);
        return [p];
    });
    if (config.check.length === 0)
        throw new UnilogError(`${file}: nothing to check yet. Add folders or .ul files to 'check'.`);
    return { roots, files: [...new Set(files)].sort() };
}
