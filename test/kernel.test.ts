import fs from 'fs';
import path from 'path';
import { describe, expect, it } from 'vitest';
import { checkProof, checkProposition, KernelError, KProof, KTerm } from '../src/kernel';
import { findSources, kernelProposition, Module, Workspace } from '../src';
import { checkSource } from './helpers';

const SRC = `
    a : likes jake cake;
    pie : likes jake pie;
    b : \\X. likes X cake => likes X sugar;
    c = b a;
    id : \\A. A => A = \\P. P;
    all_mortal : \\X. mortal X;
    socrates : mortal socrates;
    pair : \\A B. A => B => both A B;
    premise : (\\M. mortal M) => both (mortal socrates) (mortal plato) = \\F. pair F F;
`;

const mod = checkSource(SRC);
const env = (m: Module) => (key: string) => {
    const d = m.decls.get(key.slice(key.lastIndexOf('.') + 1));
    return d && kernelProposition(d);
};
const cert = (name: string) => mod.decls.get(name)?.certificate as KProof;
const claim = (name: string) => kernelProposition(mod.decls.get(name) as never);
const verify = (p: KProof, c: KTerm) => () => checkProof(p, c, env(mod));

const atom = (name: string): KTerm => ({ k: 'atom', name });
const app = (fn: KTerm, arg: KTerm): KTerm => ({ k: 'app', fn, arg });

describe('the proof kernel', () => {
    it('accepts the certificates the checker writes', () => {
        for (const name of ['c', 'id', 'premise']) expect(verify(cert(name), claim(name))).not.toThrow();
    });

    it('accepts every certificate of the examples, re-checked here independently', () => {
        const root = path.join(__dirname, '..', 'examples');
        const ws = new Workspace([root]);
        const modules = new Map<string, Module>();
        for (const file of findSources(root)) {
            const name = path.relative(root, file).replace(/\.ul$/, '').split(path.sep).join('.');
            modules.set(name, ws.load(name));
        }
        const all = (key: string) => {
            const at = key.lastIndexOf('.');
            const d = modules.get(key.slice(0, at))?.decls.get(key.slice(at + 1));
            return d && kernelProposition(d);
        };
        let checked = 0;
        for (const m of modules.values())
            for (const d of m.decls.values())
                if (d.certificate) {
                    expect(() => checkProof(d.certificate as KProof, kernelProposition(d), all)).not.toThrow();
                    checked++;
                }
        expect(checked).toBeGreaterThan(50);
    });

    it('writes out what the checker filled in: b instantiated at jake, applied to a', () => {
        expect(cert('c')).toEqual({
            k: 'app',
            fn: { k: 'inst', of: { k: 'decl', name: 'Test.b' }, term: atom('jake') },
            arg: { k: 'decl', name: 'Test.a' },
        });
    });

    describe('rejects tampered certificates', () => {
        it('a wrong instantiation (b at pie, applied to a proof about cake)', () => {
            const bad: KProof = {
                k: 'app',
                fn: { k: 'inst', of: { k: 'decl', name: 'Test.b' }, term: atom('pie') },
                arg: { k: 'decl', name: 'Test.a' },
            };
            expect(verify(bad, claim('c'))).toThrow(/does not prove the function's premise/);
        });

        it('a different argument', () => {
            const bad: KProof = {
                k: 'app',
                fn: { k: 'inst', of: { k: 'decl', name: 'Test.b' }, term: atom('jake') },
                arg: { k: 'decl', name: 'Test.pie' },
            };
            expect(verify(bad, claim('c'))).toThrow(KernelError);
        });

        it('a certificate for a different claim', () => {
            expect(verify(cert('c'), app(app(atom('likes'), atom('jake')), atom('pie')))).toThrow(
                /proves something other than the claim/,
            );
        });

        it('an unknown declaration', () => {
            expect(verify({ k: 'decl', name: 'Test.nothing' }, claim('c'))).toThrow(/unknown declaration/);
        });

        it('a universal claimed from a single example', () => {
            // "for every X, mortal X", from a proof about socrates only.
            const bad: KProof = { k: 'gen', ty: 't', body: { k: 'decl', name: 'Test.socrates' } };
            expect(verify(bad, claim('all_mortal'))).toThrow(/proves something other than the claim/);
        });

        it('a fixed symbol used outside its "for every"', () => {
            const bad: KProof = { k: 'lam', prop: { k: 'gvar', i: 0 }, body: { k: 'hyp', i: 0 } };
            expect(verify(bad, claim('id'))).toThrow(/not in scope/);
        });

        it('an assumption that is not in scope', () => {
            expect(verify({ k: 'hyp', i: 0 }, claim('c'))).toThrow(/not in scope/);
        });

        it('applying something that is not an implication', () => {
            const bad: KProof = { k: 'app', fn: { k: 'decl', name: 'Test.a' }, arg: { k: 'decl', name: 'Test.a' } };
            expect(verify(bad, claim('c'))).toThrow(/not an implication/);
        });

        it('instantiating with a gap where a proposition is expected', () => {
            const gap: KTerm = { k: 'lam', ty: 't', body: atom('p') };
            const bad: KProof = {
                k: 'app',
                fn: { k: 'inst', of: { k: 'decl', name: 'Test.b' }, term: gap },
                arg: { k: 'decl', name: 'Test.a' },
            };
            expect(verify(bad, claim('c'))).toThrow(/wrong type/);
        });

        it('passing a single example to a universal premise', () => {
            const bad: KProof = {
                k: 'app',
                fn: { k: 'decl', name: 'Test.premise' },
                arg: { k: 'decl', name: 'Test.socrates' },
            };
            const both = app(
                app(atom('both'), app(atom('mortal'), atom('socrates'))),
                app(atom('mortal'), atom('plato')),
            );
            expect(verify(bad, both)).toThrow(/does not prove the function's premise/);
        });
    });

    it("rejects a proposition applying a variable to itself (Curry's paradox)", () => {
        const curry: KTerm = {
            k: 'all',
            ty: 't',
            body: { k: 'app', fn: { k: 'bvar', i: 0 }, arg: { k: 'bvar', i: 0 } },
        };
        expect(() => checkProposition(curry)).toThrow(KernelError);
    });

    it('shares no code with the checker: it imports nothing', () => {
        const source = fs.readFileSync(path.join(__dirname, '..', 'src', 'kernel.ts'), 'utf8');
        expect(source).not.toMatch(/^import /m);
    });
});
