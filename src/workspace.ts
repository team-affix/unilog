import fs from 'fs';
import path from 'path';
import { checkModule, Declaration, keyOf, Module } from './check';
import { Loc, UnilogError } from './errors';
import { parse } from './parser';

export const EXTENSION = '.ul';

/**
 * Loads and checks modules by name.
 *
 * Module `Pkg.Sub.File` is the file `Pkg/Sub/File.ul` under one of the search roots. For a DPM project
 * the roots are the project directory and its `deps/` directory. A package's files sit in a folder named
 * after the package in both places, so the same imports work whether a package is the project being
 * checked or a dependency of another one.
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

/** What to check in a directory. */
export interface Target {
    roots: string[];
    files: string[];
}

/**
 * Works out what to check in a directory.
 *
 * - A DPM project (has `dpm.json`): the files under `<name>/`, with roots `.` and `deps/`.
 *   Dependencies are checked as far as the project's files import them.
 * - Anything else: every `.ul` file under the directory, with the directory as the root.
 */
export function targetOf(dir: string): Target {
    const manifest = path.join(dir, 'dpm.json');
    if (fs.existsSync(manifest)) {
        const { name } = JSON.parse(fs.readFileSync(manifest, 'utf8')) as { name?: unknown };
        if (typeof name !== 'string') throw new UnilogError(`${manifest}: 'name' must be a string`);
        const source = path.join(dir, name);
        if (!fs.existsSync(source)) throw new UnilogError(`Missing source directory ${source}`);
        return { roots: [dir, path.join(dir, 'deps')], files: findSources(source) };
    }
    return { roots: [dir], files: findSources(dir) };
}
