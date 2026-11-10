import * as path from 'path';
import { ExtensionContext, workspace } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

/** Starts the Unilog language server (bundled as out/server.js) for .ul files. */
export async function activate(context: ExtensionContext): Promise<void> {
    const module = context.asAbsolutePath(path.join('out', 'server.js'));
    const serverOptions: ServerOptions = {
        run: { module, transport: TransportKind.ipc },
        debug: { module, transport: TransportKind.ipc, options: { execArgv: ['--nolazy', '--inspect=6009'] } },
    };
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'unilog' }],
        // Re-check when .ul files or a unilog.json change on disk (e.g. after installing dependencies).
        synchronize: { fileEvents: workspace.createFileSystemWatcher('**/{*.ul,unilog.json}') },
    };
    client = new LanguageClient('unilog', 'Unilog', serverOptions, clientOptions);
    await client.start();
}

export function deactivate(): Promise<void> | undefined {
    return client?.stop();
}
