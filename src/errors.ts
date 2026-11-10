/** A position in a source file (1-based). */
export interface Loc {
    file: string;
    line: number;
    col: number;
    /** Where the located text ends (exclusive), when known. */
    end?: { line: number; col: number };
}

/** A location spanning from the start of `a` to the end of `b`. */
export function span(a: Loc, b: Loc): Loc {
    return { file: a.file, line: a.line, col: a.col, end: b.end ?? { line: b.line, col: b.col } };
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
