// Bundles the extension client and the Unilog language server (from ../../src) into out/, so the
// extension needs nothing else installed.
import { build } from 'esbuild';

const common = { bundle: true, platform: 'node', format: 'cjs', target: 'node18', sourcemap: true, logLevel: 'info' };

await build({ ...common, entryPoints: ['src/extension.ts'], outfile: 'out/extension.js', external: ['vscode'] });
await build({ ...common, entryPoints: ['../../src/lsp/server.ts'], outfile: 'out/server.js' });
