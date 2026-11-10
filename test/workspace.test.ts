import fs from 'fs';
import path from 'path';
import { describe, expect, it } from 'vitest';
import { commands, keyOf, targetOf, Workspace } from '../src';
import { proves, tmpDir, write } from './helpers';

const KANT = {
    'Kant/Ethics.ul': `
        good_will : good will.
        duty : good X => ought (act_from X).
    `,
};

const ETHICS = {
    'Ethics/Main.ul': `
        import Kant.Ethics.
        acting = duty good_will.
    `,
};

describe('Workspace', () => {
    it('resolves imports by module name through the search roots', () => {
        const dir = tmpDir();
        write(dir, { ...KANT, ...ETHICS });
        const ws = new Workspace([dir]);
        expect(proves(ws.load('Ethics.Main'), 'acting')).toBe('ought (act_from will)');
    });

    it('checks a package identically as a project and as a dependency', () => {
        // As its own project: Ethics/ next to deps/Kant/
        const project = tmpDir();
        write(project, { 'dpm.json': JSON.stringify({ name: 'Ethics', deps: [] }), ...ETHICS });
        write(path.join(project, 'deps'), KANT);

        // As a dependency of App: everything under deps/
        const app = tmpDir();
        write(app, {
            'dpm.json': JSON.stringify({ name: 'App', deps: [] }),
            'App/Main.ul': 'import Ethics.Main.\nmine = Ethics.Main.acting.',
        });
        write(path.join(app, 'deps'), { ...KANT, ...ETHICS });

        // Same files, byte for byte, in both layouts
        expect(fs.readFileSync(path.join(project, 'Ethics/Main.ul'), 'utf8')).toBe(
            fs.readFileSync(path.join(app, 'deps/Ethics/Main.ul'), 'utf8'),
        );
        expect(commands.check(project)).toMatchObject({ ok: true });
        expect(commands.check(app)).toMatchObject({ ok: true });
        expect(commands.show('App.Main.mine', app).out).toEqual(['theorem App.Main.mine : ought (act_from will)']);
    });

    it('supports qualified references and reports ambiguity', () => {
        const dir = tmpDir();
        write(dir, {
            'A/M.ul': 'x : from_a.',
            'B/M.ul': 'x : from_b.',
            'C/M.ul': 'import A.M.\nimport B.M.\nqa = A.M.x.\nqb = B.M.x.',
            'D/M.ul': 'import A.M.\nimport B.M.\nbad = x.',
        });
        const ws = new Workspace([dir]);
        const c = ws.load('C.M');
        expect([proves(c, 'qa'), proves(c, 'qb')]).toEqual(['from_a', 'from_b']);
        expect(() => ws.load('D.M')).toThrow(/'x' is ambiguous[\s\S]*A\.M\.x or B\.M\.x/);
    });

    it('lets local declarations shadow imported ones', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : from_a.', 'B/M.ul': 'import A.M.\nx : mine.\ny = x.' });
        expect(proves(new Workspace([dir]).load('B.M'), 'y')).toBe('mine');
    });

    it('only allows qualified references to imported modules', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p.', 'B/M.ul': 'y = A.M.x.' });
        expect(() => new Workspace([dir]).load('B.M')).toThrow(/Module A\.M is not imported/);
    });

    it('does not re-export: imports are not transitive', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p.', 'B/M.ul': 'import A.M.\ny = x.', 'C/M.ul': 'import B.M.\nz = x.' });
        expect(() => new Workspace([dir]).load('C.M')).toThrow(/'x' is not declared/);
    });

    it('detects import cycles', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'import B.M.\na : p.', 'B/M.ul': 'import A.M.\nb : q.' });
        expect(() => new Workspace([dir]).load('A.M')).toThrow(/Import cycle: A\.M -> B\.M -> A\.M/);
    });

    it('reports missing modules with the importing location', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': '\nimport Nope.Gone.' });
        expect(() => new Workspace([dir]).load('A.M')).toThrow(/M\.ul:2:1: Module Nope\.Gone not found/);
    });

    it('requires imports before declarations', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p.', 'B/M.ul': 'y : q.\nimport A.M.' });
        expect(() => new Workspace([dir]).load('B.M')).toThrow(/Imports must come before declarations/);
    });

    it('checks each module once, however many modules import it', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p.', 'B/M.ul': 'import A.M.', 'C/M.ul': 'import A.M.\nimport B.M.' });
        const ws = new Workspace([dir]);
        ws.load('C.M');
        expect(
            ws
                .loaded()
                .map((m) => m.name)
                .sort(),
        ).toEqual(['A.M', 'B.M', 'C.M']);
    });

    it('rejects a file whose path is not a valid module name', () => {
        const dir = tmpDir();
        write(dir, { 'my-pkg/M.ul': 'x : p.' });
        expect(() => new Workspace([dir]).loadFile(path.join(dir, 'my-pkg/M.ul'))).toThrow(/not a valid module name/);
    });
});

describe('axiomsOf: everything a claim rests on', () => {
    it('follows proofs through every module, listing only the axioms actually used', () => {
        const dir = tmpDir();
        write(dir, {
            'Base/Facts.ul': 'a : likes jake cake.\nunused : irrelevant.',
            'Base/Rules.ul': 'b : likes X cake => likes X sugar.\nlift : P => believed P.',
            'Mine/Main.ul': 'import Base.Facts.\nimport Base.Rules.\nc = b a.\nd = lift c.\ne = p => p.',
        });
        const ws = new Workspace([dir]);
        expect(ws.axiomsOf('Mine.Main.d').map(keyOf)).toEqual(['Base.Facts.a', 'Base.Rules.b', 'Base.Rules.lift']);
        expect(ws.axiomsOf('Mine.Main.e')).toEqual([]);
        expect(ws.axiomsOf('Base.Facts.a').map(keyOf)).toEqual(['Base.Facts.a']);
    });
});

describe('targetOf', () => {
    it('checks <name>/ of a DPM project against . and deps/', () => {
        const dir = tmpDir();
        write(dir, { 'dpm.json': '{"name":"P","deps":[]}', 'P/A.ul': '', 'P/sub/B.ul': '', 'deps/Q/C.ul': '' });
        const t = targetOf(dir);
        expect(t.roots).toEqual([dir, path.join(dir, 'deps')]);
        expect(t.files.map((f) => path.relative(dir, f))).toEqual(['P/A.ul', 'P/sub/B.ul']);
    });

    it('checks every .ul file in a plain directory', () => {
        const dir = tmpDir();
        write(dir, { 'X/A.ul': '', 'notes.txt': '' });
        expect(targetOf(dir).files.map((f) => path.relative(dir, f))).toEqual(['X/A.ul']);
    });
});

describe('commands', () => {
    it('check reports success with counts', () => {
        const dir = tmpDir();
        write(dir, { ...KANT, ...ETHICS });
        expect(commands.check(dir)).toEqual({ ok: true, out: ['ok: 2 module(s), 1 theorem(s), 2 axiom(s)'], err: [] });
    });

    it('check --verbose lists every declaration', () => {
        const dir = tmpDir();
        write(dir, KANT);
        expect(commands.check(dir, true).out.slice(0, 2)).toEqual([
            'axiom   Kant.Ethics.good_will : good will',
            'axiom   Kant.Ethics.duty : good X => ought (act_from X)',
        ]);
    });

    it('check reports every failing module, each once', () => {
        const dir = tmpDir();
        write(dir, {
            'A/Bad.ul': 'x = nope.',
            'B/Bad.ul': 'y : p.\nz = y y.',
            'C/Uses.ul': 'import A.Bad.',
        });
        const r = commands.check(dir);
        expect(r.ok).toBe(false);
        expect(r.err).toHaveLength(2);
        expect(r.err.join('\n')).toMatch(/'nope' is not declared/);
        expect(r.err.join('\n')).toMatch(/Cannot apply y to y/);
    });

    it('axioms prints each axiom with its proposition', () => {
        const dir = tmpDir();
        write(dir, { ...KANT, ...ETHICS });
        expect(commands.axioms('Ethics.Main.acting', dir).out).toEqual([
            'Kant.Ethics.duty : good X => ought (act_from X)',
            'Kant.Ethics.good_will : good will',
        ]);
    });

    it('reports a missing source directory or declaration', () => {
        const dir = tmpDir();
        write(dir, { 'dpm.json': '{"name":"P"}' });
        expect(commands.check(dir)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/Missing source directory/)],
        });
        const d2 = tmpDir();
        write(d2, KANT);
        expect(commands.show('Kant.Ethics.nope', d2)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/no declaration 'nope'/)],
        });
    });
});
