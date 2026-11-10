import path from 'path';
import { describe, expect, it } from 'vitest';
import { diskSources, SourceProvider } from '../src';
import { analyze, definition, findProjectDir, hover, inlayHints, outline, problems } from '../src/lsp/features';
import { tmpDir, write } from './helpers';

function project() {
    const dir = tmpDir();
    write(dir, {
        'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['Mine'] }),
        'deps/Kant/Ethics.ul': 'duty : good X => ought X.\nhonesty : good truthful.\ntell_truth = duty honesty.',
        'Mine/Main.ul': 'import Kant.Ethics.\nmine = duty honesty.\nstated : ought truthful = tell_truth.\n',
    });
    return dir;
}

/** Editor text for some files, the disk for the rest. */
const overlay = (texts: Record<string, string>): SourceProvider => ({
    exists: (f) => f in texts || diskSources.exists(f),
    read: (f) => texts[f] ?? diskSources.read(f),
});

describe('language server features', () => {
    it('finds the project of a file by its nearest unilog.json', () => {
        const dir = project();
        expect(findProjectDir(path.join(dir, 'Mine/Main.ul'))).toBe(dir);
        expect(findProjectDir(path.join(dir, 'deps/Kant/Ethics.ul'))).toBe(dir);
    });

    it('reports no problems for a project that checks', () => {
        expect(problems(analyze(project()))).toEqual([]);
    });

    it('checks unsaved editor text, and locates problems with ranges', () => {
        const dir = project();
        const main = path.join(dir, 'Mine/Main.ul');
        const a = analyze(dir, overlay({ [main]: 'import Kant.Ethics.\nbad = duty duty.' }));
        expect(problems(a)).toEqual([
            {
                file: main,
                range: { start: { line: 1, character: 6 }, end: { line: 1, character: 15 } },
                message: expect.stringMatching(/^Cannot apply duty to duty/),
            },
        ]);
    });

    it('reports a problem in an imported module at its own location, saying how it was reached', () => {
        const dir = project();
        const kant = path.join(dir, 'deps/Kant/Ethics.ul');
        const a = analyze(dir, overlay({ [kant]: 'duty : good X => ought X.\nhonesty = nope.' }));
        expect(problems(a)).toEqual([
            {
                file: kant,
                range: { start: { line: 1, character: 10 }, end: { line: 1, character: 14 } },
                message: expect.stringMatching(
                    /'nope' is not declared[\s\S]*\(in Kant\.Ethics, imported by Mine\.Main\)/,
                ),
            },
        ]);
    });

    it('puts configuration problems on unilog.json', () => {
        const dir = tmpDir();
        write(dir, { 'unilog.json': '{ "roots": ["."], "check": [] }' });
        expect(problems(analyze(dir))).toMatchObject([
            { file: path.join(dir, 'unilog.json'), message: expect.stringMatching(/nothing to check/) },
        ]);
    });

    describe('hover', () => {
        const at = (line: number, character: number) => {
            const dir = project();
            return hover(analyze(dir), path.join(dir, 'Mine/Main.ul'), { line, character });
        };

        it('on a declaration name: what it proves, and that it was inferred', () => {
            expect(at(1, 1)).toBe('```unilog\ntheorem Mine.Main.mine : ought truthful\n```\n\n(inferred)');
            expect(at(2, 2)).toBe('```unilog\ntheorem Mine.Main.stated : ought truthful\n```');
        });

        it('on a rule used in a proof: what it proves there, then its general statement', () => {
            expect(at(1, 8)).toBe(
                '```unilog\nduty : good truthful => ought truthful\n```\n\n```unilog\naxiom Kant.Ethics.duty : good X => ought X\n```',
            );
        });

        it('on an import: the module', () => {
            expect(at(0, 10)).toMatch(/^module \*\*Kant\.Ethics\*\*: 2 axiom\(s\), 1 theorem\(s\)/);
        });

        it('on an assumption and on a whole sub-proof', () => {
            const dir = tmpDir();
            write(dir, {
                'M/A.ul': 'b : likes X cake => likes X sugar.\nb2 : q => likes jake cake.\nlift = p => b (b2 p).',
            });
            const a = analyze(dir);
            const file = path.join(dir, 'M/A.ul');
            // lift = p => b (b2 p).   p is at column 18; the '(' at 14 is inside only the application b (b2 p)
            expect(hover(a, file, { line: 2, character: 18 })).toBe('```unilog\np : q\n```\n\nassumption');
            expect(hover(a, file, { line: 2, character: 14 })).toBe('```unilog\nb (b2 p) : likes jake sugar\n```');
        });

        it('on nothing in particular: nothing', () => {
            expect(at(1, 6)).toBeUndefined();
        });
    });

    it('inlay hints show inferred claims, ready to insert after the name', () => {
        const dir = project();
        expect(inlayHints(analyze(dir), path.join(dir, 'Mine/Main.ul'))).toEqual([
            { position: { line: 1, character: 4 }, label: ': ought truthful', insert: ' : ought truthful' },
        ]);
    });

    it('go to definition: from a use to its declaration, and from an import to its module', () => {
        const dir = project();
        const a = analyze(dir);
        const main = path.join(dir, 'Mine/Main.ul');
        const kant = path.join(dir, 'deps/Kant/Ethics.ul');
        expect(definition(a, main, { line: 2, character: 28 })).toEqual({
            file: kant,
            range: { start: { line: 2, character: 0 }, end: { line: 2, character: 10 } },
        });
        expect(definition(a, main, { line: 0, character: 8 })).toEqual({
            file: kant,
            range: { start: { line: 0, character: 0 }, end: { line: 0, character: 0 } },
        });
    });

    it('outline lists declarations with what they prove', () => {
        const dir = project();
        expect(
            outline(analyze(dir), path.join(dir, 'deps/Kant/Ethics.ul')).map((e) => [e.name, e.kind, e.detail]),
        ).toEqual([
            ['duty', 'axiom', 'good X => ought X'],
            ['honesty', 'axiom', 'good truthful'],
            ['tell_truth', 'theorem', 'ought truthful'],
        ]);
    });
});
