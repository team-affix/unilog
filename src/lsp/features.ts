import fs from 'fs';
import path from 'path';
import { Declaration, Derivation, keyOf, Module } from '../check';
import { Loc, UnilogError } from '../errors';
import { Printer } from '../term';
import { PROJECT_FILE, SourceProvider, targetOf, Workspace } from '../workspace';

/**
 * Editor features, as plain functions over a checked project. The language server (server.ts) only
 * wires these to the protocol. Positions here are 0-based (line, character), as in the protocol.
 */

export interface Position {
    line: number;
    character: number;
}
export interface Range {
    start: Position;
    end: Position;
}

/** A located message for the editor. */
export interface Problem {
    file: string;
    range: Range;
    message: string;
}

/** A checked project: the workspace with every module that checked, and the errors from those that didn't. */
export interface Analysis {
    projectDir: string;
    ws: Workspace;
    errors: UnilogError[];
}

/** Converts a Unilog location (1-based, end exclusive) to a range. A location without an end covers one character. */
export function rangeOf(loc: Loc): Range {
    const start = { line: loc.line - 1, character: loc.col - 1 };
    const end = loc.end
        ? { line: loc.end.line - 1, character: loc.end.col - 1 }
        : { ...start, character: start.character + 1 };
    return { start, end };
}

const before = (a: Position, b: Position) => a.line < b.line || (a.line === b.line && a.character < b.character);
const notAfter = (a: Position, b: Position) => !before(b, a);

/** Whether `pos` is inside `loc` (end inclusive, so a cursor right after a name still counts). */
export function contains(loc: Loc, pos: Position): boolean {
    const r = rangeOf(loc);
    return notAfter(r.start, pos) && notAfter(pos, r.end);
}

/**
 * The project a file belongs to: the nearest directory at or above it with a unilog.json, stopping at
 * `stopAt` (e.g. the editor's workspace folder). Without one, `stopAt`, or else the file's directory.
 */
export function findProjectDir(file: string, stopAt?: string): string {
    const stop = stopAt ? path.resolve(stopAt) : undefined;
    let dir = path.dirname(path.resolve(file));
    for (;;) {
        if (fs.existsSync(path.join(dir, PROJECT_FILE))) return dir;
        const parent = path.dirname(dir);
        if (dir === stop || parent === dir) break;
        dir = parent;
    }
    return stop ?? path.dirname(path.resolve(file));
}

/**
 * Checks a project, as `unilog check` would, reading sources through `sources` (so unsaved editor text is
 * used). `alsoCheck` are extra files to check, e.g. open documents outside the project's `check` paths.
 */
export function analyze(projectDir: string, sources?: SourceProvider, alsoCheck: string[] = []): Analysis {
    const errors: UnilogError[] = [];
    let target;
    try {
        target = targetOf(projectDir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { projectDir, ws: new Workspace([projectDir], sources), errors: [e] };
    }
    const ws = new Workspace(target.roots, sources);
    for (const file of [...target.files, ...alsoCheck]) {
        try {
            ws.loadFile(file);
        } catch (e) {
            if (!(e instanceof UnilogError)) throw e;
            if (!errors.some((x) => x.message === e.message)) errors.push(e);
        }
    }
    return { projectDir, ws, errors };
}

/** The analysis's errors, located for the editor. Errors with no location go on the project's unilog.json. */
export function problems(a: Analysis): Problem[] {
    return a.errors.map((e) => ({
        file: e.loc?.file ? path.resolve(e.loc.file) : path.join(a.projectDir, PROJECT_FILE),
        range: e.loc ? rangeOf(e.loc) : { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
        message: e.reason + (e.context ? `\n(${e.context})` : ''),
    }));
}

/** The innermost proof step containing `pos`, if any. */
function stepAt(d: Derivation, pos: Position): Derivation | undefined {
    if (!contains(d.loc, pos)) return undefined;
    const children = d.k === 'apply' ? [d.fn, d.arg] : d.k === 'assume' ? [d.body] : [];
    for (const c of children) {
        const inner = stepAt(c, pos);
        if (inner) return inner;
    }
    return d;
}

const code = (text: string) => '```unilog\n' + text + '\n```';
const describe = (d: Declaration, p: Printer) => `${d.kind} ${keyOf(d)} : ${p.print(d.scheme.body)}`;

/** Markdown describing what is under the cursor: a declaration, a proof step, or an imported module. */
export function hover(a: Analysis, file: string, pos: Position): string | undefined {
    const mod = a.ws.moduleAt(file);
    if (!mod) return undefined;
    const p = new Printer();

    const imported = mod.importLocs.findIndex((loc) => contains(loc, pos));
    if (imported >= 0) {
        const m = a.ws.loaded().find((x) => x.name === mod.imports[imported]);
        if (!m) return undefined;
        const decls = [...m.decls.values()];
        const axioms = decls.filter((d) => d.kind === 'axiom').length;
        return `module **${m.name}**: ${axioms} axiom(s), ${decls.length - axioms} theorem(s)\n\n${m.file}`;
    }

    for (const d of mod.decls.values()) {
        if (contains(d.nameLoc, pos))
            return code(describe(d, p)) + (d.kind === 'theorem' && !d.stated ? '\n\n(inferred)' : '');
        if (!d.proof || !contains(d.loc, pos)) continue;
        const step = stepAt(d.proof, pos);
        if (!step) continue;
        const here = code(`${step.text} : ${p.print(step.term)}`);
        if (step.k !== 'use') return here;
        if (!step.decl) return `${here}\n\nassumption`;
        return `${here}\n\n${code(describe(step.decl, p))}`;
    }
    return undefined;
}

/** An inline hint: text the editor shows but which is not part of the file. */
export interface InlayHint {
    position: Position;
    label: string;
    /** Inserting this at `position` writes the hint into the file. */
    insert: string;
}

/** For each theorem without a written claim, a hint after its name showing what it proves. */
export function inlayHints(a: Analysis, file: string): InlayHint[] {
    const mod = a.ws.moduleAt(file);
    if (!mod) return [];
    return [...mod.decls.values()]
        .filter((d) => d.kind === 'theorem' && !d.stated && d.proof)
        .map((d) => {
            const prop = new Printer().print((d.proof as Derivation).term);
            return { position: rangeOf(d.nameLoc).end, label: `: ${prop}`, insert: ` : ${prop}` };
        });
}

/** Where the thing under the cursor is declared: a referenced declaration, or an imported module. */
export function definition(a: Analysis, file: string, pos: Position): { file: string; range: Range } | undefined {
    const mod = a.ws.moduleAt(file);
    if (!mod) return undefined;
    const imported = mod.importLocs.findIndex((loc) => contains(loc, pos));
    if (imported >= 0) {
        const m = a.ws.loaded().find((x) => x.name === mod.imports[imported]);
        return (
            m && {
                file: path.resolve(m.file),
                range: { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
            }
        );
    }
    for (const d of mod.decls.values()) {
        if (!d.proof || !contains(d.loc, pos)) continue;
        const step = stepAt(d.proof, pos);
        if (step?.k === 'use' && step.decl)
            return { file: path.resolve(step.decl.nameLoc.file), range: rangeOf(step.decl.nameLoc) };
    }
    return undefined;
}

/** One entry in a module's outline. */
export interface OutlineEntry {
    name: string;
    kind: 'axiom' | 'theorem';
    detail: string;
    range: Range;
    selectionRange: Range;
}

/** The declarations of the module in `file`, for the editor's outline. */
export function outline(a: Analysis, file: string): OutlineEntry[] {
    const mod: Module | undefined = a.ws.moduleAt(file);
    if (!mod) return [];
    return [...mod.decls.values()].map((d) => ({
        name: d.name,
        kind: d.kind,
        detail: new Printer().print(d.scheme.body),
        range: rangeOf(d.loc),
        selectionRange: rangeOf(d.nameLoc),
    }));
}
