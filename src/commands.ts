import { keyOf } from './check';
import { UnilogError } from './errors';
import { Printer } from './term';
import { targetOf, Workspace } from './workspace';

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

/**
 * `unilog check [dir]`: checks a DPM project (its `<name>/` files, importing from `.` and `deps/`), or every
 * `.ul` file in a plain directory. Reports every failing module, not just the first.
 */
export function check(dir: string, verbose = false): Output {
    const result: Output = { ok: true, out: [], err: [] };
    let target;
    try {
        target = targetOf(dir);
    } catch (e) {
        if (!(e instanceof UnilogError)) throw e;
        return { ok: false, out: [], err: [e.message] };
    }
    const ws = new Workspace(target.roots);
    for (const file of target.files) {
        try {
            ws.loadFile(file);
        } catch (e) {
            if (!(e instanceof UnilogError)) throw e;
            if (!result.err.includes(e.message)) result.err.push(e.message);
        }
    }
    const modules = ws.loaded().sort((a, b) => a.name.localeCompare(b.name));
    const decls = modules.flatMap((m) => [...m.decls.values()]);
    if (verbose)
        for (const d of decls)
            result.out.push(`${d.kind === 'axiom' ? 'axiom  ' : 'theorem'} ${keyOf(d)} : ${prop(d.scheme.body)}`);
    if (result.err.length > 0) return { ...result, ok: false };
    const axioms = decls.filter((d) => d.kind === 'axiom').length;
    result.out.push(`ok: ${modules.length} module(s), ${decls.length - axioms} theorem(s), ${axioms} axiom(s)`);
    return result;
}

/** `unilog axioms <Mod.name> [dir]`: every axiom the declaration rests on. */
export function axioms(name: string, dir: string): Output {
    return guarded((out) => {
        const list = new Workspace(targetOf(dir).roots).axiomsOf(name);
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
