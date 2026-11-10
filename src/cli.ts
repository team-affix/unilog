#!/usr/bin/env node
import { Command } from 'commander';
import { axioms, check, Output, show } from './commands';

const emit = (o: Output) => {
    o.out.forEach((l) => console.log(l));
    o.err.forEach((l) => console.error(l));
    if (!o.ok) process.exitCode = 1;
};

const program = new Command().name('unilog').description('Check Unilog proofs, and list the beliefs a claim rests on');

program
    .command('check')
    .description('Check a DPM project (its <name>/ files, against deps/), or every .ul file in a directory')
    .argument('[dir]', 'directory', '.')
    .option('-v, --verbose', 'print every declaration and what it proves')
    .action((dir: string, opts: { verbose?: boolean }) => emit(check(dir, opts.verbose)));

program
    .command('axioms')
    .description('List every axiom a declaration rests on, through all imported modules')
    .argument('<name>', 'qualified declaration name, e.g. Ethics.Duty.keep_promises')
    .argument('[dir]', 'directory', '.')
    .action((name: string, dir: string) => emit(axioms(name, dir)));

program
    .command('show')
    .description('Show what a declaration proves')
    .argument('<name>', 'qualified declaration name')
    .argument('[dir]', 'directory', '.')
    .action((name: string, dir: string) => emit(show(name, dir)));

program.parseAsync().catch((e: unknown) => {
    console.error(e);
    process.exitCode = 1;
});
