#!/usr/bin/env node
import { Command } from 'commander';
import { VERSION } from './version';
import { axioms, check, find, init, Output, proof, restsOn, show, summary, inferences } from './commands';

const emit = (o: Output) => {
    o.out.forEach((l) => console.log(l));
    o.err.forEach((l) => console.error(l));
    if (!o.ok) process.exitCode = 1;
};

const program = new Command()
    .name('unilog')
    .version(VERSION)
    .description('Check Unilog proofs, and list the beliefs a claim rests on');

program
    .command('init')
    .description('Create a unilog.json')
    .argument('[dir]', 'directory', '.')
    .option('--roots <paths...>', 'search roots (default: .)')
    .option('--check <paths...>', 'folders or .ul files to check')
    .action((dir: string, opts: { roots?: string[]; check?: string[] }) => emit(init(dir, opts)));

program
    .command('check')
    .description('Check a project (configured by unilog.json), or every .ul file in a directory without one')
    .argument('[dir]', 'directory', '.')
    .option('-v, --verbose', 'print every declaration and what it proves')
    .action((dir: string, opts: { verbose?: boolean }) => emit(check(dir, opts)));

program
    .command('axioms')
    .description('Count the axioms declared in a module (only parses; does not check)')
    .argument('<module>', 'module name, e.g. Ethics.Main')
    .argument('[dir]', 'directory', '.')
    .option('-t, --transitive', 'include every module it imports, transitively')
    .option('-c, --count', 'print only the number')
    .option('--json', 'output JSON')
    .action((module: string, dir: string, opts: { transitive?: boolean; count?: boolean; json?: boolean }) =>
        emit(axioms(module, dir, opts)),
    );

program
    .command('rests-on')
    .description('List every axiom a declaration rests on, following its proof through all imported modules')
    .argument('<name>', 'qualified declaration name, e.g. Ethics.Main.keep_promises')
    .argument('[dir]', 'directory', '.')
    .option('--json', "output JSON, including each axiom's module")
    .action((name: string, dir: string, opts: { json?: boolean }) => emit(restsOn(name, dir, opts.json)));

program
    .command('inferences')
    .description('List the inferences declared in a module, with what each proves')
    .argument('<module>', 'module name, e.g. Ethics.Main')
    .argument('[dir]', 'directory', '.')
    .option('-t, --transitive', 'include every module it imports, transitively')
    .option('-c, --count', 'print only the number')
    .option('--json', 'output JSON')
    .action((module: string, dir: string, opts: { transitive?: boolean; count?: boolean; json?: boolean }) =>
        emit(inferences(module, dir, opts)),
    );

program
    .command('find')
    .description(
        'List the declarations that prove a proposition, e.g. `unilog find false`. Exits non-zero if there are none.',
    )
    .argument('<proposition>', 'the proposition, quoted if it has spaces or parentheses: "wrong X"')
    .argument('[dir]', 'directory', '.')
    .option(
        '-u, --unify',
        "match any declaration that unifies with the query (both sides' variables may bind): shape matching, not proof",
    )
    .option('-e, --exact', 'match only declarations of exactly the same generality (variable names may differ)')
    .option('--json', 'output JSON')
    .action((query: string, dir: string, opts: { unify?: boolean; exact?: boolean; json?: boolean }) =>
        emit(find(query, dir, opts)),
    );

program
    .command('proof')
    .description("Show a declaration's whole proof as a tree, down to axioms and assumptions")
    .argument('<name>', 'qualified declaration name, e.g. Ethics.Main.keep_promises')
    .argument('[dir]', 'directory', '.')
    .option('-s, --shallow', "don't expand the inferences it uses into their own proofs")
    .option('-f, --flat', 'list the declarations it relies on, each once, in declaration order')
    .option('-a, --axioms-first', 'with --flat: list axioms before inferences')
    .option('--json', 'output the tree (or with --flat, the list) as JSON')
    .action(
        (
            name: string,
            dir: string,
            opts: { shallow?: boolean; flat?: boolean; axiomsFirst?: boolean; json?: boolean },
        ) => emit(proof(name, dir, opts)),
    );

program
    .command('show')
    .description('Show what a declaration proves')
    .argument('<name>', 'qualified declaration name')
    .argument('[dir]', 'directory', '.')
    .action((name: string, dir: string) => emit(show(name, dir)));

program
    .command('summary')
    .description('Axiom and inference counts for every module, checked and imported')
    .argument('[dir]', 'directory', '.')
    .option('--json', 'output the full declaration graph as JSON (uses, and the axioms each rests on)')
    .action((dir: string, opts: { json?: boolean }) => emit(summary(dir, opts.json)));

program.parseAsync().catch((e: unknown) => {
    console.error(e);
    process.exitCode = 1;
});
