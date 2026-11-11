import fs from 'fs';
import path from 'path';
import { describe, expect, it } from 'vitest';
import { commands } from '../src';
import { tmpDir, write } from './helpers';

/** A project whose own code is Mine/, importing Base/ and, through it, Logic/. */
function project(mine: string) {
    const dir = tmpDir();
    write(dir, {
        'unilog.json': JSON.stringify({ roots: ['.'], check: ['Mine'] }),
        'Logic/Core.ul': 'explode : false => C.\nid = \\P -> P.',
        'Base/Facts.ul': `
            import Logic.Core.
            a : likes jake cake.
            b : likes X cake => likes X sugar.
            c = b a.
            everything : wrong X.
        `,
        'Mine/Main.ul': mine,
    });
    return dir;
}

describe('find', () => {
    it('finds the declarations proving a proposition, in the project and its imports', () => {
        const dir = project('import Base.Facts.\nmine = b a.');
        expect(commands.find('likes jake sugar', dir)).toEqual({
            ok: true,
            out: ['theorem Base.Facts.c : likes jake sugar', 'theorem Mine.Main.mine : likes jake sugar'],
            err: [],
        });
    });

    it('detects a contradiction: unilog find false', () => {
        const clean = project('import Base.Facts.\nmine = b a.');
        expect(commands.find('false', clean)).toEqual({ ok: false, out: [], err: ['No declaration proves false'] });

        const contradictory = project(`
            import Base.Facts.
            import Logic.Core.
            not_cake : likes jake cake => false.
            oops = not_cake a.
        `);
        expect(commands.find('false', contradictory)).toMatchObject({
            ok: true,
            out: ['theorem Mine.Main.oops : false'],
        });
    });

    it('counts a more general proof: something proving every X also proves the query', () => {
        const dir = project('import Base.Facts.');
        expect(commands.find('wrong bob_party', dir).out).toEqual(['axiom   Base.Facts.everything : wrong X']);
    });

    it('requires a proof of the query in full generality by default', () => {
        const dir = project('import Base.Facts.');
        // b proves likes X cake => likes X sugar for every X; c only for jake
        expect(commands.find('likes X cake => likes X sugar', dir).out).toEqual([
            'axiom   Base.Facts.b : likes X cake => likes X sugar',
        ]);
        expect(commands.find('likes X sugar', dir)).toMatchObject({ ok: false });
    });

    it('--unify matches anything of the query shape', () => {
        const dir = project('import Base.Facts.');
        expect(commands.find('likes X sugar', dir, { unify: true }).out).toEqual([
            'theorem Base.Facts.c : likes jake sugar',
        ]);
    });

    it("--json lists each match with its module and whether it is the project's own code", () => {
        const dir = project('import Base.Facts.\nmine = b a.');
        const r = commands.find('likes jake sugar', dir, { json: true });
        expect(r.ok).toBe(true);
        expect(JSON.parse(r.out[0] as string)).toEqual({
            query: 'likes jake sugar',
            found: [
                {
                    name: 'Base.Facts.c',
                    kind: 'theorem',
                    module: 'Base.Facts',
                    own: false,
                    proposition: 'likes jake sugar',
                },
                {
                    name: 'Mine.Main.mine',
                    kind: 'theorem',
                    module: 'Mine.Main',
                    own: true,
                    proposition: 'likes jake sugar',
                },
            ],
        });
        expect(commands.find('nothing_here', dir, { json: true })).toMatchObject({ ok: false });
    });

    it('rejects a malformed query', () => {
        expect(commands.find('likes (jake', project(''))).toMatchObject({
            ok: false,
            err: [expect.stringMatching(/<query>:1:/)],
        });
    });
});

describe('theorems', () => {
    it('lists the theorems declared in a module, without proofs', () => {
        const dir = project('import Base.Facts.\nmine = b a.\nalso : likes jake cake = a.\nmore : q.');
        expect(commands.theorems('Mine.Main', dir).out).toEqual([
            'Mine.Main.mine : likes jake sugar',
            'Mine.Main.also : likes jake cake',
            '2 theorem(s) in Mine.Main',
        ]);
    });

    it('--transitive includes every imported module, each once', () => {
        const dir = project('import Base.Facts.\nimport Logic.Core.\nmine = b a.');
        expect(commands.theorems('Mine.Main', dir, { transitive: true }).out).toEqual([
            'Mine.Main.mine : likes jake sugar',
            'Base.Facts.c : likes jake sugar',
            'Logic.Core.id : A => A',
            '3 theorem(s) in Mine.Main and the 2 module(s) it imports',
        ]);
        expect(commands.theorems('Mine.Main', dir, { transitive: true, count: true }).out).toEqual(['3']);
        expect(JSON.parse(commands.theorems('Base.Facts', dir, { json: true }).out[0] as string)).toEqual({
            module: 'Base.Facts',
            transitive: false,
            count: 1,
            theorems: [{ name: 'Base.Facts.c', module: 'Base.Facts', proposition: 'likes jake sugar' }],
        });
    });

    it('reports check errors', () => {
        expect(commands.theorems('Mine.Main', project('x = nope.'))).toMatchObject({ ok: false });
    });
});

describe('init options', () => {
    it('writes the given roots and check paths', () => {
        const dir = tmpDir();
        expect(commands.init(dir, { roots: ['.', 'deps'], check: ['Bob'] }).out).toEqual([
            `Created ${path.join(dir, 'unilog.json')}.`,
        ]);
        expect(JSON.parse(fs.readFileSync(path.join(dir, 'unilog.json'), 'utf8'))).toEqual({
            roots: ['.', 'deps'],
            check: ['Bob'],
        });
    });
});

describe('errors in imported modules show how they were reached', () => {
    it('names the failing module and the chain of imports leading to it', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['Bob'] }),
            'Bob/Rebuttal.ul': 'import Alice.Beliefs.',
            'deps/Alice/Beliefs.ul': 'import Logic.Prelude.',
        });
        const r = commands.check(dir);
        expect(r.err).toHaveLength(1);
        expect(r.err[0]).toMatch(/Alice\/Beliefs\.ul:1:1: Module Logic\.Prelude not found/);
        expect(r.err[0]).toMatch(/\(in Alice\.Beliefs, imported by Bob\.Rebuttal\)$/);
    });

    it('reports a module once however many modules import it', () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['.'], check: ['Mine'] }),
            'Mine/A.ul': 'import Shared.Broken.',
            'Mine/B.ul': 'import Shared.Broken.',
            'Shared/Broken.ul': 'x = nope.',
        });
        const r = commands.check(dir);
        expect(r.err).toHaveLength(1);
        expect(r.err[0]).toMatch(/'nope' is not declared[\s\S]*\(in Shared\.Broken, imported by Mine\.A\)/);
    });

    it('adds no context to errors in the checked modules themselves', () => {
        const dir = tmpDir();
        write(dir, { 'unilog.json': JSON.stringify({ roots: ['.'], check: ['Mine'] }), 'Mine/A.ul': 'x = nope.' });
        expect(commands.check(dir).err[0]).not.toMatch(/\(in /);
    });
});
