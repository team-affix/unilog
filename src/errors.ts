/** A position in a source file (1-based). */
export interface Loc {
    file: string;
    line: number;
    col: number;
}

/** Every error Unilog reports about user input. */
export class UnilogError extends Error {
    constructor(
        /** The error without its location. */
        public readonly reason: string,
        public readonly loc?: Loc,
        /** How the failing module was reached, e.g. "in Alice.Beliefs, imported by Bob.Rebuttal". */
        public readonly context?: string,
    ) {
        super((loc ? `${loc.file}:${loc.line}:${loc.col}: ` : '') + reason + (context ? `\n  (${context})` : ''));
        this.name = 'UnilogError';
    }
}
