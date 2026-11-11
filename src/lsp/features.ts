import fs from 'fs';
import path from 'path';
import { Declaration, Derivation, keyOf, Module } from '../check';
import { Loc, UnilogError } from '../errors';
import { parse } from '../parser';
import { Printer, prune } from '../term';
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

/**
 * Whether the character at `pos` is inside `loc`. With `cursor`, a position just after the end also counts,
 * as a text cursor right after a name refers to that name (a mouse always points at a character).
 */
export function contains(loc: Loc, pos: Position, cursor = false): boolean {
    const r = rangeOf(loc);
    return notAfter(r.start, pos) && (cursor ? notAfter(pos, r.end) : before(pos, r.end));
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

/** The proof steps containing `pos`, outermost first; the last is the innermost. Empty if none does. */
function stepsAt(d: Derivation, pos: Position, cursor: boolean): Derivation[] {
    if (!contains(d.loc, pos, cursor)) return [];
    const children = d.k === 'apply' ? [d.fn, d.arg] : d.k === 'assume' ? [d.body] : [];
    for (const c of children) {
        const inner = stepsAt(c, pos, cursor);
        if (inner.length > 0) return [d, ...inner];
    }
    return [d];
}

/** Proof steps (`proof : proposition`), in the editor's step grammar, which colors the proof part as a proof. */
const steps = (lines: string[]) => '```unilog-step\n' + lines.join('\n') + '\n```';
/** A declaration as a step line (its name is a proof), then whether it is an axiom or an inference. */
const describe = (d: Declaration, p: Printer) => `${steps([`${keyOf(d)} : ${p.print(d.scheme.body)}`])}\n\n${d.kind}`;

/** What a hover shows, and the range of source it describes (which the editor highlights). */
export interface HoverInfo {
    markdown: string;
    range: Range;
}

/**
 * What is under the mouse, and the exact range it covers:
 * - a declaration's name: what it proves;
 * - a rule's name applied to arguments, e.g. `pair` in `pair (snd P) (fst P)`: every step of applying it, one
 *   argument at a time, then its general statement;
 * - an assumption where it is introduced (`P` in `P => ...`): what it is assumed to prove;
 * - any other proof step: what that step alone proves. The gap after `pair` is the step `pair (snd P)`; the
 *   gap after `(snd P)` is the whole `pair (snd P) (fst P)`. A parenthesized step includes its parentheses;
 * - an import: the module.
 */
export function hover(a: Analysis, file: string, pos: Position): HoverInfo | undefined {
    const mod = a.ws.moduleAt(file);
    if (!mod) return undefined;
    const p = new Printer();

    const imported = mod.importLocs.findIndex((loc) => contains(loc, pos));
    if (imported >= 0) {
        const m = a.ws.loaded().find((x) => x.name === mod.imports[imported]);
        if (!m) return undefined;
        const decls = [...m.decls.values()];
        const axioms = decls.filter((d) => d.kind === 'axiom').length;
        return {
            markdown: `module **${m.name}**: ${axioms} axiom(s), ${decls.length - axioms} inference(s)\n\n${m.file}`,
            range: rangeOf(mod.importLocs[imported] as Loc),
        };
    }

    for (const d of mod.decls.values()) {
        if (contains(d.nameLoc, pos))
            return {
                markdown: describe(d, p) + (d.kind === 'inference' && !d.stated ? ' (claim inferred)' : ''),
                range: rangeOf(d.nameLoc),
            };
        if (!d.proof || !contains(d.loc, pos)) continue;
        const path = stepsAt(d.proof, pos, false);
        const step = path.at(-1);
        if (!step) continue;
        // Name variables as in the inference's own statement (and its inlay hint), so every step agrees with it.
        p.print(d.proof.term);

        if (step.k === 'assume' && contains(step.paramLoc, pos)) {
            const assumed = prune(step.term);
            const proves = assumed.k === 'imp' ? p.print(assumed.from) : '?';
            return { markdown: steps([`${step.param} : ${proves}`]) + '\n\nassumption', range: rangeOf(step.paramLoc) };
        }
        const range = rangeOf(step.loc);
        if (step.k !== 'use') return { markdown: steps([`${step.text} : ${p.print(step.term)}`]), range };
        if (!step.decl) return { markdown: steps([`${step.text} : ${p.print(step.term)}`]) + '\n\nassumption', range };

        // A rule's name: climb while it is the function being applied, to show it taking each argument in turn.
        const spine: Derivation[] = [step];
        for (let i = path.length - 2; i >= 0; i--) {
            const parent = path[i] as Derivation;
            if (parent.k !== 'apply' || parent.fn !== spine[spine.length - 1]) break;
            spine.push(parent);
        }
        const applied = steps(spine.map((s) => `${s.text} : ${p.print(s.term)}`));
        // The rule's general statement has its own variables: name them independently.
        return { markdown: `${applied}\n\n${describe(step.decl, new Printer())}`, range };
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

/** For each inference without a written claim, a hint after its name showing what it proves. */
export function inlayHints(a: Analysis, file: string): InlayHint[] {
    const mod = a.ws.moduleAt(file);
    if (!mod) return [];
    return [...mod.decls.values()]
        .filter((d) => d.kind === 'inference' && !d.stated && d.proof)
        .map((d) => {
            const prop = new Printer().print((d.proof as Derivation).term);
            return { position: rangeOf(d.nameLoc).end, label: `: ${prop}`, insert: ` : ${prop}` };
        });
}

/** Where the thing under the cursor is declared: a referenced declaration, or an imported module. */
export function definition(a: Analysis, file: string, pos: Position): { file: string; range: Range } | undefined {
    const mod = a.ws.moduleAt(file);
    if (!mod) return undefined;
    const imported = mod.importLocs.findIndex((loc) => contains(loc, pos, true));
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
        if (!d.proof || !contains(d.loc, pos, true)) continue;
        const step = stepsAt(d.proof, pos, true).at(-1);
        if (step?.k === 'use' && step.decl)
            return { file: path.resolve(step.decl.nameLoc.file), range: rangeOf(step.decl.nameLoc) };
    }
    return undefined;
}

/** One entry in a module's outline. */
export interface OutlineEntry {
    name: string;
    kind: 'axiom' | 'inference';
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

/** Where a declaration starts, and whether it is an axiom or an inference. */
export interface DeclarationMarker {
    name: string;
    kind: 'axiom' | 'inference';
    /** The 0-based line of the declaration's name. */
    line: number;
}

/**
 * The axioms and inferences declared in some source text, for gutter markers. Only parses, so it also works
 * while the file has checking errors (a file that doesn't parse has no markers).
 */
export function declarationMarkers(text: string, file: string): DeclarationMarker[] {
    let decls;
    try {
        decls = parse(text, file);
    } catch (e) {
        if (e instanceof UnilogError) return [];
        throw e;
    }
    return decls.flatMap((d) => (d.k === 'import' ? [] : [{ name: d.name, kind: d.k, line: d.nameLoc.line - 1 }]));
}
