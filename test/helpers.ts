import fs from 'fs';
import os from 'os';
import path from 'path';
import { afterEach } from 'vitest';
import { checkModule, Module, parse, Printer } from '../src';

const created: string[] = [];
afterEach(() => {
    for (const d of created.splice(0)) fs.rmSync(d, { recursive: true, force: true });
});

export function tmpDir(): string {
    const d = fs.mkdtempSync(path.join(os.tmpdir(), 'unilog-test-'));
    created.push(d);
    return d;
}

/** Writes files (relative path -> content) under `dir`. */
export function write(dir: string, files: Record<string, string>): void {
    for (const [rel, content] of Object.entries(files)) {
        const p = path.join(dir, rel);
        fs.mkdirSync(path.dirname(p), { recursive: true });
        fs.writeFileSync(p, content);
    }
}

/** Checks a single standalone module from source. */
export function checkSource(src: string, imported = new Map<string, Module>()): Module {
    return checkModule('Test', 'test.ul', parse(src, 'test.ul'), imported);
}

/** What a declaration in a module proves, printed. */
export function proves(mod: Module, name: string): string {
    const d = mod.decls.get(name);
    if (!d) throw new Error(`no declaration ${name}`);
    return new Printer().print(d.prop);
}

/** Checks `src` and returns what `name` proves. */
export function provesIn(src: string, name: string): string {
    return proves(checkSource(src), name);
}
