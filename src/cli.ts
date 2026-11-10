#!/usr/bin/env node
import { Command, InvalidArgumentError } from 'commander';
import { axioms, check, Output, show, summary } from './commands';

const emit = (o: Output) => {
    o.out.forEach((l) => console.log(l));
    o.err.forEach((l) => console.error(l));
    if (!o.ok) process.exitCode = 1;
};

const program = new Command().name('unilog').description('Check Unilog proofs, and list the beliefs a claim rests on');

program
    .command('check')
    .description('Check a project (configured by unilog.json), or every .ul file in a directory without one')
    .argument('[dir]', 'directory', '.')
    .option('-v, --verbose', 'print every declaration and what it proves')
    .option(
        '--max-axioms <n>',
        'fail if the checked code declares more than n axioms (0: it may only use imported ones)',
        (v: string) => {
            const n = Number(v);
            if (!Number.isInteger(n) || n < 0) throw new InvalidArgumentError('must be a whole number, 0 or more');
            return n;
        },
    )
    .action((dir: string, opts: { verbose?: boolean; maxAxioms?: number }) => emit(check(dir, opts)));

program
    .command('axioms')
    .description('List every axiom a declaration rests on, through all imported modules')
    .argument('<name>', 'qualified declaration name, e.g. Ethics.Duty.keep_promises')
    .argument('[dir]', 'directory', '.')
    .option('--json', "output JSON, including each axiom's module")
    .action((name: string, dir: string, opts: { json?: boolean }) => emit(axioms(name, dir, opts.json)));

program
    .command('show')
    .description('Show what a declaration proves')
    .argument('<name>', 'qualified declaration name')
    .argument('[dir]', 'directory', '.')
    .action((name: string, dir: string) => emit(show(name, dir)));

program
    .command('summary')
    .description('Axiom and theorem counts for every module, checked and imported')
    .argument('[dir]', 'directory', '.')
    .option('--json', 'output the full declaration graph as JSON (uses, and the axioms each rests on)')
    .action((dir: string, opts: { json?: boolean }) => emit(summary(dir, opts.json)));

program.parseAsync().catch((e: unknown) => {
    console.error(e);
    process.exitCode = 1;
});
