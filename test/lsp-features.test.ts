import path from 'path';
import { describe, expect, it } from 'vitest';
import { diskSources, SourceProvider } from '../src';
import {
    analyze,
    declarationMarkers,
    definition,
    findProjectDir,
    hover,
    inlayHints,
    outline,
    problems,
} from '../src/lsp/features';
import { tmpDir, write } from './helpers';

function project() {
    const dir = tmpDir();
    write(dir, {
        'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['Mine'] }),
        'deps/Kant/Ethics.ul': 'duty : \\X. good X => ought X;\nhonesty : good truthful;\ntell_truth = duty honesty;',
        'Mine/Main.ul': 'import Kant.Ethics;\nmine = duty honesty;\nstated : ought truthful = tell_truth;\n',
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
        const a = analyze(dir, overlay({ [main]: 'import Kant.Ethics;\nbad = duty duty;' }));
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
        const a = analyze(dir, overlay({ [kant]: 'duty : \\X. good X => ought X;\nhonesty = nope;' }));
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
            return hover(analyze(dir), path.join(dir, 'Mine/Main.ul'), { line, character })?.markdown;
        };

        it('on a declaration name: what it proves, and that it was inferred', () => {
            expect(at(1, 1)).toBe('```unilog-step\nMine.Main.mine : ought truthful\n```\n\ninference (claim inferred)');
            expect(at(2, 2)).toBe('```unilog-step\nMine.Main.stated : ought truthful\n```\n\ninference');
        });

        it('on a rule applied to arguments: each step of applying it, then its general statement', () => {
            expect(at(1, 8)).toBe(
                '```unilog-step\nduty : good truthful => ought truthful\nduty honesty : ought truthful\n```' +
                    '\n\n```unilog-step\nKant.Ethics.duty : \\X. good X => ought X\n```\n\naxiom',
            );
        });

        it('on an import: the module', () => {
            expect(at(0, 10)).toMatch(/^module \*\*Kant\.Ethics\*\*: 2 axiom\(s\), 1 inference\(s\)/);
        });

        describe('inside a curried proof, without needing parentheses', () => {
            // and_comm = \P. pair (snd P) (fst P);
            // 0         1         2         3
            // 012345678901234567890123456789012345
            const dir = () => {
                const d = tmpDir();
                write(d, {
                    'M/P.ul':
                        'pair : \\A B. A => B => and A B;\nfst : \\A B. and A B => A;\nsnd : \\A B. and A B => B;\nand_comm = \\P. pair (snd P) (fst P);',
                });
                return d;
            };
            const on = (character: number) => {
                const d = dir();
                return hover(analyze(d), path.join(d, 'M/P.ul'), { line: 3, character })?.markdown;
            };

            it('the rule name shows it taking each argument in turn, named as in the statement', () => {
                expect(on(16)).toBe(
                    '```unilog-step\npair : B => A => and B A\npair (snd P) : A => and B A\npair (snd P) (fst P) : and B A\n```' +
                        '\n\n```unilog-step\nM.P.pair : \\A B. A => B => and A B\n```\n\naxiom',
                );
            });

            it('an assumption where it is introduced shows what it is assumed to prove', () => {
                expect(on(12)).toBe('```unilog-step\nP : and A B\n```\n\nassumption');
                expect(on(25)).toBe('```unilog-step\nP : and A B\n```\n\nassumption');
            });

            it('parentheses belong to the step they enclose, at both ends', () => {
                expect(on(20)).toBe('```unilog-step\nsnd P : B\n```');
                expect(on(26)).toBe('```unilog-step\nsnd P : B\n```');
                expect(on(28)).toBe('```unilog-step\nfst P : A\n```');
            });

            it('a rule taking one argument shows both steps', () => {
                expect(on(21)).toBe(
                    '```unilog-step\nsnd : and A B => B\nsnd P : B\n```\n\n```unilog-step\nM.P.snd : \\A B. and A B => B\n```\n\naxiom',
                );
            });

            it('the gap after a rule name is its first application only; the last gap is the whole', () => {
                // and_comm = \P. pair (snd P) (fst P);   gap at 19 is after `pair`, gap at 27 after `(snd P)`
                expect(on(19)).toBe('```unilog-step\npair (snd P) : A => and B A\n```');
                expect(on(27)).toBe('```unilog-step\npair (snd P) (fst P) : and B A\n```');
            });

            it('reports the exact range it describes, for the editor to highlight', () => {
                const d = dir();
                const range = (character: number) =>
                    hover(analyze(d), path.join(d, 'M/P.ul'), { line: 3, character })?.range;
                const span = (from: number, to: number) => ({
                    start: { line: 3, character: from },
                    end: { line: 3, character: to },
                });
                expect(range(16)).toEqual(span(15, 19)); // the name pair
                expect(range(19)).toEqual(span(15, 27)); // pair (snd P)
                expect(range(27)).toEqual(span(15, 35)); // pair (snd P) (fst P)
                expect(range(20)).toEqual(span(20, 27)); // (snd P), parentheses included
                expect(range(12)).toEqual(span(12, 13)); // the assumption P
                expect(range(2)).toEqual(span(0, 8)); // the declaration name and_comm
            });

            it('whitespace between steps belongs to the enclosing application', () => {
                expect(on(27)).toBe('```unilog-step\npair (snd P) (fst P) : and B A\n```');
            });
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

    it('declaration markers: which lines start axioms and which start inferences, from parsing alone', () => {
        const text = 'import Kant.Ethics;\n\na : p;\nb : p => q;\nc =\n    b a;\nd : q = c;\n';
        expect(declarationMarkers(text, 'f.ul')).toEqual([
            { name: 'a', kind: 'axiom', line: 2 },
            { name: 'b', kind: 'axiom', line: 3 },
            { name: 'c', kind: 'inference', line: 4 },
            { name: 'd', kind: 'inference', line: 6 },
        ]);
        // Still there when the file has checking errors (nothing here is declared or imported) ...
        expect(declarationMarkers('t = nope nope;', 'f.ul')).toEqual([{ name: 't', kind: 'inference', line: 0 }]);
        // ... and none while it doesn't parse.
        expect(declarationMarkers('a : (p;', 'f.ul')).toEqual([]);
    });

    it('outline lists declarations with what they prove', () => {
        const dir = project();
        expect(
            outline(analyze(dir), path.join(dir, 'deps/Kant/Ethics.ul')).map((e) => [e.name, e.kind, e.detail]),
        ).toEqual([
            ['duty', 'axiom', '\\X. good X => ought X'],
            ['honesty', 'axiom', 'good truthful'],
            ['tell_truth', 'inference', 'ought truthful'],
        ]);
    });
});
