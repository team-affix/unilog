/**
 * The package version. Read from package.json at runtime in the npm package; inlined by esbuild into the
 * single-file release bundles.
 */
// eslint-disable-next-line @typescript-eslint/no-require-imports
export const VERSION: string = (require('../package.json') as { version: string }).version;
