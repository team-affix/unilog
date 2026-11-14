import fs from 'fs';
import path from 'path';
import { describe, expect, it } from 'vitest';
import { commands, keyOf, targetOf, Workspace } from '../src';
import { proves, tmpDir, write } from './helpers';

const KANT = {
    'Kant/Ethics.ul': `
        good_will : good will;
        duty : \\X. good X => ought (act_from X);
    `,
};

const ETHICS = {
    'Ethics/Main.ul': `
        import Kant.Ethics;
        acting = duty good_will;
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
        const config = (pkg: string) => JSON.stringify({ roots: ['.', 'deps'], check: [pkg] });

        // As its own project: Ethics/ is in the project, its dependency Kant/ is under deps/.
        const project = tmpDir();
        write(project, { 'unilog.json': config('Ethics'), ...ETHICS });
        write(path.join(project, 'deps'), KANT);

        // As a dependency of App: Ethics/ now sits beside its own dependency Kant/, not above it.
        // It even carries a broken project file of its own, which is ignored: only App's roots apply.
        const app = tmpDir();
        write(app, {
            'unilog.json': config('App'),
            'App/Main.ul': 'import Ethics.Main;\nmine = Ethics.Main.acting;',
        });
        write(path.join(app, 'deps'), {
            ...KANT,
            ...ETHICS,
            'Ethics/unilog.json': JSON.stringify({ roots: ['nowhere'], check: ['nothing'] }),
        });

        // Same files, byte for byte, in both layouts
        expect(fs.readFileSync(path.join(project, 'Ethics/Main.ul'), 'utf8')).toBe(
            fs.readFileSync(path.join(app, 'deps/Ethics/Main.ul'), 'utf8'),
        );
        expect(commands.check(project)).toMatchObject({ ok: true });
        expect(commands.check(app)).toMatchObject({ ok: true });
        expect(commands.show('App.Main.mine', app).out).toEqual(['inference App.Main.mine : ought (act_from will)']);
    });

    it('supports qualified references and reports ambiguity', () => {
        const dir = tmpDir();
        write(dir, {
            'A/M.ul': 'x : from_a;',
            'B/M.ul': 'x : from_b;',
            'C/M.ul': 'import A.M;\nimport B.M;\nqa = A.M.x;\nqb = B.M.x;',
            'D/M.ul': 'import A.M;\nimport B.M;\nbad = x;',
        });
        const ws = new Workspace([dir]);
        const c = ws.load('C.M');
        expect([proves(c, 'qa'), proves(c, 'qb')]).toEqual(['from_a', 'from_b']);
        expect(() => ws.load('D.M')).toThrow(/'x' is ambiguous[\s\S]*A\.M\.x or B\.M\.x/);
    });

    it('lets local declarations shadow imported ones', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : from_a;', 'B/M.ul': 'import A.M;\nx : mine;\ny = x;' });
        expect(proves(new Workspace([dir]).load('B.M'), 'y')).toBe('mine');
    });

    it('only allows qualified references to imported modules', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p;', 'B/M.ul': 'y = A.M.x;' });
        expect(() => new Workspace([dir]).load('B.M')).toThrow(/Module A\.M is not imported/);
    });

    it('does not re-export: imports are not transitive', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p;', 'B/M.ul': 'import A.M;\ny = x;', 'C/M.ul': 'import B.M;\nz = x;' });
        expect(() => new Workspace([dir]).load('C.M')).toThrow(/'x' is not declared/);
    });

    it('detects import cycles', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'import B.M;\na : p;', 'B/M.ul': 'import A.M;\nb : q;' });
        expect(() => new Workspace([dir]).load('A.M')).toThrow(/Import cycle: A\.M -> B\.M -> A\.M/);
    });

    it('reports missing modules with the importing location', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': '\nimport Nope.Gone;' });
        expect(() => new Workspace([dir]).load('A.M')).toThrow(/M\.ul:2:1: Module Nope\.Gone not found/);
    });

    it('requires imports before declarations', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p;', 'B/M.ul': 'y : q;\nimport A.M;' });
        expect(() => new Workspace([dir]).load('B.M')).toThrow(/Imports must come before declarations/);
    });

    it('checks each module once, however many modules import it', () => {
        const dir = tmpDir();
        write(dir, { 'A/M.ul': 'x : p;', 'B/M.ul': 'import A.M;', 'C/M.ul': 'import A.M;\nimport B.M;' });
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
        write(dir, { 'my-pkg/M.ul': 'x : p;' });
        expect(() => new Workspace([dir]).loadFile(path.join(dir, 'my-pkg/M.ul'))).toThrow(/not a valid module name/);
    });
});

describe('axiomsOf: everything a claim rests on', () => {
    it('follows proofs through every module, listing only the axioms actually used', () => {
        const dir = tmpDir();
        write(dir, {
            'Base/Facts.ul': 'a : likes jake cake;\nunused : irrelevant;',
            'Base/Rules.ul': 'b : \\X. likes X cake => likes X sugar;\nlift : \\P. P => believed P;',
            'Mine/Main.ul': 'import Base.Facts;\nimport Base.Rules;\nc = b a;\nd = lift c;\ne = \\P. P;',
        });
        const ws = new Workspace([dir]);
        expect(ws.axiomsOf('Mine.Main.d').map(keyOf)).toEqual(['Base.Facts.a', 'Base.Rules.b', 'Base.Rules.lift']);
        expect(ws.axiomsOf('Mine.Main.e')).toEqual([]);
        expect(ws.axiomsOf('Base.Facts.a').map(keyOf)).toEqual(['Base.Facts.a']);
    });
});

describe('targetOf', () => {
    it('uses unilog.json: its check paths, with its roots as search roots', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['P', 'Extra/One.ul'] }),
            'P/A.ul': '',
            'P/sub/B.ul': '',
            'Extra/One.ul': '',
            'Extra/Two.ul': '',
            'deps/Q/C.ul': '',
        });
        const t = targetOf(dir);
        expect(t.roots).toEqual([path.join(dir, '.'), path.join(dir, 'deps')]);
        expect(t.files.map((f) => path.relative(dir, f))).toEqual(['Extra/One.ul', 'P/A.ul', 'P/sub/B.ul']);
    });

    it('skips roots that do not exist yet (e.g. nothing installed)', () => {
        const dir = tmpDir();
        write(dir, { 'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['P'] }), 'P/A.ul': 'a : p;' });
        expect(targetOf(dir).roots).toEqual([path.join(dir, '.')]);
        expect(commands.check(dir).ok).toBe(true);
    });

    it('allows any layout: the roots are whatever the project says', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['src', 'vendor/libs'], check: ['src'] }),
            'src/Mine/M.ul': 'import Lib.L;\nt = l;',
            'vendor/libs/Lib/L.ul': 'l : from_lib;',
        });
        expect(commands.check(dir)).toMatchObject({ ok: true });
        expect(commands.show('Mine.M.t', dir).out).toEqual(['inference Mine.M.t : from_lib']);
    });

    it('checks every .ul file in a directory without unilog.json', () => {
        const dir = tmpDir();
        write(dir, { 'X/A.ul': '', 'notes.txt': '' });
        expect(targetOf(dir)).toEqual({ roots: [dir], files: [path.join(dir, 'X/A.ul')] });
    });

    it.each([
        ['invalid JSON', '{', /not valid JSON/],
        ['a non-object', '[]', /must be a JSON object/],
        ['missing roots', JSON.stringify({ check: ['P'] }), /'roots' must be an array of paths/],
        ['non-string paths', JSON.stringify({ roots: [1], check: ['P'] }), /'roots' must be an array of paths/],
        ['nothing to check', JSON.stringify({ roots: ['.'], check: [] }), /nothing to check yet/],
        ['an unknown field', JSON.stringify({ roots: ['.'], check: ['P'], name: 'x' }), /unknown field\(s\): name/],
        [
            'a missing check path',
            JSON.stringify({ roots: ['.'], check: ['Nope'] }),
            /'check' path does not exist: Nope/,
        ],
        ['a check file that is not .ul', JSON.stringify({ roots: ['.'], check: ['P/readme.md'] }), /not a \.ul file/],
    ])('rejects a unilog.json with %s', (_, config, msg) => {
        const dir = tmpDir();
        write(dir, { 'unilog.json': config, 'P/A.ul': '', 'P/readme.md': '' });
        expect(() => targetOf(dir)).toThrow(msg);
        expect(commands.check(dir)).toMatchObject({ ok: false, err: [expect.stringMatching(msg)] });
    });
});

describe('init', () => {
    it('writes a starting unilog.json with both fields', () => {
        const dir = tmpDir();
        expect(commands.init(dir).ok).toBe(true);
        expect(JSON.parse(fs.readFileSync(path.join(dir, 'unilog.json'), 'utf8'))).toEqual({ roots: ['.'], check: [] });
    });

    it('creates the directory if needed', () => {
        const dir = path.join(tmpDir(), 'new', 'project');
        expect(commands.init(dir).ok).toBe(true);
        expect(fs.existsSync(path.join(dir, 'unilog.json'))).toBe(true);
    });

    it('never overwrites an existing unilog.json', () => {
        const dir = tmpDir();
        write(dir, { 'unilog.json': '{"roots":["src"],"check":["src"]}' });
        expect(commands.init(dir)).toMatchObject({ ok: false, err: [expect.stringMatching(/already exists/)] });
        expect(fs.readFileSync(path.join(dir, 'unilog.json'), 'utf8')).toBe('{"roots":["src"],"check":["src"]}');
    });

    it('a fresh project says what to fill in, then checks once it is filled in', () => {
        const dir = tmpDir();
        commands.init(dir);
        expect(commands.check(dir)).toMatchObject({ ok: false, err: [expect.stringMatching(/nothing to check yet/)] });
        write(dir, { 'unilog.json': JSON.stringify({ roots: ['.'], check: ['Mine'] }), 'Mine/A.ul': 'a : p;' });
        expect(commands.check(dir).ok).toBe(true);
    });
});

describe('commands', () => {
    it('check reports success with counts', () => {
        const dir = tmpDir();
        write(dir, { ...KANT, ...ETHICS });
        expect(commands.check(dir)).toEqual({
            ok: true,
            out: ['ok: 2 module(s) checked, declaring 1 inference(s) and 2 axiom(s); 0 imported module(s)'],
            err: [],
        });
    });

    it('check --verbose lists every declaration', () => {
        const dir = tmpDir();
        write(dir, KANT);
        expect(commands.check(dir, { verbose: true }).out.slice(0, 2)).toEqual([
            'axiom     Kant.Ethics.good_will : good will',
            'axiom     Kant.Ethics.duty : \\X. good X => ought (act_from X)',
        ]);
    });

    it('check reports every failing module, each once', () => {
        const dir = tmpDir();
        write(dir, {
            'A/Bad.ul': 'x = nope;',
            'B/Bad.ul': 'y : p;\nz = y y;',
            'C/Uses.ul': 'import A.Bad;',
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
        expect(commands.restsOn('Ethics.Main.acting', dir).out).toEqual([
            'Kant.Ethics.duty : \\X. good X => ought (act_from X)',
            'Kant.Ethics.good_will : good will',
        ]);
    });

    it('reports a missing declaration', () => {
        const d2 = tmpDir();
        write(d2, KANT);
        expect(commands.show('Kant.Ethics.nope', d2)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/no declaration 'nope'/)],
        });
    });
});

describe('packages installed by DPM: "deps/*" roots', () => {
    const KANT = 'bafybeieedcwo4wyypn4l2r73564segyb3evs7zeeomxbci7pedrif3c2pu';
    const ALICE = 'bafybeielfcvgt3wpq3y5aeg3j7n4iz4lpylgqex436uhz6xkepefn524di';
    const config = JSON.stringify({ roots: ['.', 'deps/*'], check: ['Mine'] });

    it('looks modules up in every folder of deps/, each laid out like a project', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': config,
            [`deps/${KANT}/Kant/Ethics.ul`]: 'duty : \\X. good X => ought X;\n',
            [`deps/${ALICE}/Alice/Beliefs.ul`]: 'import Kant.Ethics;\nhonest : good honesty;\n',
            'Mine/Main.ul': 'import Alice.Beliefs;\nimport Kant.Ethics;\nmine = duty honest;\n',
        });
        expect(commands.check(dir)).toMatchObject({ ok: true });
        // In name order: KANT's CID sorts before ALICE's.
        expect(targetOf(dir).roots).toEqual([dir, path.join(dir, 'deps', KANT), path.join(dir, 'deps', ALICE)]);
    });

    it('names a module by the most specific root it is under', () => {
        const dir = tmpDir();
        write(dir, { [`deps/${KANT}/Kant/Ethics.ul`]: 'duty : \\X. good X => ought X;\n' });
        const ws = new Workspace([dir, path.join(dir, 'deps', KANT)]);
        expect(ws.moduleNameOf(path.join(dir, 'deps', KANT, 'Kant/Ethics.ul'))).toBe('Kant.Ethics');
    });

    it('reports two packages providing the same module', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': config,
            [`deps/${KANT}/Kant/Ethics.ul`]: 'duty : \\X. good X => ought X;\n',
            [`deps/${ALICE}/Kant/Ethics.ul`]: 'duty : \\X. good X => permitted X;\n',
            'Mine/Main.ul': 'import Kant.Ethics;\n',
        });
        expect(commands.check(dir)).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/Module Kant\.Ethics is ambiguous/)],
        });
    });
});
