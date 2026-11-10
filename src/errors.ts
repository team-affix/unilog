/** A position in a source file (1-based). */
export interface Loc {
    file: string;
    line: number;
    col: number;
}

/** Every error Unilog reports about user input. */
export class UnilogError extends Error {
    constructor(
        message: string,
        public readonly loc?: Loc,
    ) {
        super(loc ? `${loc.file}:${loc.line}:${loc.col}: ${message}` : message);
        this.name = 'UnilogError';
    }
}
