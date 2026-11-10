import path from 'path';
import { PassThrough } from 'stream';
import { pathToFileURL } from 'url';
import { afterEach, describe, expect, it } from 'vitest';
import {
    createMessageConnection,
    MessageConnection,
    StreamMessageReader,
    StreamMessageWriter,
} from 'vscode-jsonrpc/node';
import { createConnection } from 'vscode-languageserver/node';
import { listen } from '../src/lsp/server';
import { tmpDir, write } from './helpers';

interface Diagnostics {
    uri: string;
    diagnostics: { message: string; range: { start: { line: number; character: number } } }[];
}

const open: MessageConnection[] = [];
afterEach(() => {
    for (const c of open.splice(0)) c.dispose();
});

/** Starts the server in-process and connects a protocol client to it, as an editor would. */
async function start(rootDir: string) {
    const toServer = new PassThrough();
    const toClient = new PassThrough();
    listen(createConnection(toServer, toClient));
    const client = createMessageConnection(new StreamMessageReader(toClient), new StreamMessageWriter(toServer));
    open.push(client);

    const diagnostics: Diagnostics[] = [];
    const waiters: (() => void)[] = [];
    client.onNotification('textDocument/publishDiagnostics', (d: Diagnostics) => {
        diagnostics.push(d);
        waiters.splice(0).forEach((w) => w());
    });
    client.onRequest('workspace/inlayHint/refresh', () => null);
    client.listen();

    const init = await client.sendRequest('initialize', {
        processId: null,
        rootUri: pathToFileURL(rootDir).toString(),
        workspaceFolders: [{ uri: pathToFileURL(rootDir).toString(), name: 'test' }],
        capabilities: {},
    });
    await client.sendNotification('initialized', {});

    /** Waits for the next diagnostics notification for `file`. */
    const nextDiagnostics = async (file: string) => {
        const uri = pathToFileURL(file).toString();
        for (;;) {
            const found = [...diagnostics].reverse().find((d) => d.uri === uri);
            if (found) {
                diagnostics.splice(0);
                return found;
            }
            await new Promise<void>((r) => waiters.push(r));
        }
    };
    return { client, init, nextDiagnostics };
}

describe('language server (over the protocol)', () => {
    it('advertises its features', async () => {
        const { init } = await start(tmpDir());
        expect(init).toMatchObject({
            capabilities: {
                hoverProvider: true,
                definitionProvider: true,
                documentSymbolProvider: true,
                inlayHintProvider: true,
            },
        });
    });

    it('checks an open document as you type: errors, then fixed', async () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['.'], check: ['Mine'] }),
            'Mine/Main.ul': 'a : likes jake cake.\nb : likes X cake => likes X sugar.\n',
        });
        const file = path.join(dir, 'Mine/Main.ul');
        const uri = pathToFileURL(file).toString();
        const { client, nextDiagnostics } = await start(dir);

        // Opened with unsaved text containing a mistake (on disk the file has no theorem at all).
        await client.sendNotification('textDocument/didOpen', {
            textDocument: {
                uri,
                languageId: 'unilog',
                version: 1,
                text: 'a : likes jake pie.\nb : likes X cake => likes X sugar.\nc = b a.\n',
            },
        });
        const bad = await nextDiagnostics(file);
        expect(bad.diagnostics).toHaveLength(1);
        expect(bad.diagnostics[0]?.message).toMatch(/Cannot apply b to a[\s\S]*cake does not match pie/);
        expect(bad.diagnostics[0]?.range.start).toEqual({ line: 2, character: 4 });

        // The user fixes it.
        await client.sendNotification('textDocument/didChange', {
            textDocument: { uri, version: 2 },
            contentChanges: [{ text: 'a : likes jake cake.\nb : likes X cake => likes X sugar.\nc = b a.\n' }],
        });
        expect((await nextDiagnostics(file)).diagnostics).toEqual([]);

        // Hover, inlay hints and outline all reflect the unsaved text.
        const h = (await client.sendRequest('textDocument/hover', {
            textDocument: { uri },
            position: { line: 2, character: 4 },
        })) as {
            contents: { value: string };
        };
        expect(h.contents.value).toContain('b : likes jake cake => likes jake sugar');

        const hints = (await client.sendRequest('textDocument/inlayHint', {
            textDocument: { uri },
            range: { start: { line: 0, character: 0 }, end: { line: 3, character: 0 } },
        })) as { label: string; position: unknown; textEdits: { newText: string }[] }[];
        expect(hints).toEqual([
            expect.objectContaining({
                label: ': likes jake sugar',
                position: { line: 2, character: 1 },
                textEdits: [expect.objectContaining({ newText: ' : likes jake sugar' })],
            }),
        ]);

        const symbols = (await client.sendRequest('textDocument/documentSymbol', { textDocument: { uri } })) as {
            name: string;
        }[];
        expect(symbols.map((s) => s.name)).toEqual(['a', 'b', 'c']);
    });

    it('goes to the definition in another file', async () => {
        const dir = tmpDir();
        write(dir, {
            'unilog.json': JSON.stringify({ roots: ['.', 'deps'], check: ['Mine'] }),
            'deps/Kant/Ethics.ul': 'duty : good X => ought X.\n',
            'Mine/Main.ul': 'import Kant.Ethics.\nmine = p => duty p.\n',
        });
        const uri = pathToFileURL(path.join(dir, 'Mine/Main.ul')).toString();
        const { client } = await start(dir);
        await client.sendNotification('textDocument/didOpen', {
            textDocument: { uri, languageId: 'unilog', version: 1, text: 'import Kant.Ethics.\nmine = p => duty p.\n' },
        });
        const def = await client.sendRequest('textDocument/definition', {
            textDocument: { uri },
            position: { line: 1, character: 13 },
        });
        expect(def).toEqual({
            uri: pathToFileURL(path.join(dir, 'deps/Kant/Ethics.ul')).toString(),
            range: { start: { line: 0, character: 0 }, end: { line: 0, character: 4 } },
        });
    });
});
