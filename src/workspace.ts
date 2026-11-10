import fs from 'fs';
import path from 'path';
import { checkModule, Declaration, keyOf, Module } from './check';
import { Loc, UnilogError } from './errors';
import { parse } from './parser';

export const EXTENSION = '.ul';

/**
 * Loads and checks modules by name.
 *
 * Module `Pkg.Sub.File` is the file `Pkg/Sub/File.ul` under one of the search roots. Every import, from
 * any module, is resolved against the same roots: those of the project being checked. A module never
 * looks for its imports relative to its own location, so a package's files work unchanged wherever they
 * sit (as the project itself, or alongside other packages in a dependency folder).
 */
export class Workspace {
    private readonly modules = new Map<string, Module>();
    private readonly loading: string[] = [];

    constructor(public readonly roots: readonly string[]) {}

    /** The file for a module name, or undefined if no root has it. */
    locate(name: string): string | undefined {
        const rel = path.join(...name.split('.')) + EXTENSION;
        const found = this.roots.map((r) => path.join(r, rel)).filter((f) => fs.existsSync(f));
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
        if (this.loading.includes(name))
            throw new UnilogError(
                `Import cycle: ${[...this.loading.slice(this.loading.indexOf(name)), name].join(' -> ')}`,
                from,
            );

        const file = this.locate(name);
        if (!file) throw new UnilogError(`Module ${name} not found (looked in ${this.roots.join(', ')})`, from);

        this.loading.push(name);
        try {
            const decls = parse(fs.readFileSync(file, 'utf8'), file);
            const imported = new Map<string, Module>();
            for (const d of decls)
                if (d.k === 'import') imported.set(d.path.join('.'), this.load(d.path.join('.'), d.loc));
            const mod = checkModule(name, file, decls, imported);
            this.modules.set(name, mod);
            return mod;
        } finally {
            this.loading.pop();
        }
    }

    /** Loads the module in a file. */
    loadFile(file: string): Module {
        return this.load(this.moduleNameOf(file));
    }

    /** Every module checked so far. */
    loaded(): Module[] {
        return [...this.modules.values()];
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
    if (!Array.isArray(value) || value.length === 0 || !value.every((v) => typeof v === 'string'))
        throw new UnilogError(`${file}: '${field}' must be a non-empty array of paths`);
    return value as string[];
}

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
    return { roots, files: [...new Set(files)].sort() };
}
