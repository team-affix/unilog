import * as path from 'path';
import { ExtensionContext, Range, TextEditor, TextEditorDecorationType, window, workspace } from 'vscode';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

/** One declaration, as reported by the server's `unilog/declarationMarkers` request. */
interface DeclarationMarker {
    name: string;
    kind: 'axiom' | 'theorem';
    line: number;
}

/** Starts the Unilog language server (bundled as out/server.js) for .ul files, and draws gutter markers. */
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

    // Gutter markers: a hollow diamond beside each axiom (assumed), a solid circle beside each theorem (derived).
    const markers: Record<DeclarationMarker['kind'], TextEditorDecorationType> = {
        axiom: window.createTextEditorDecorationType({
            gutterIconPath: context.asAbsolutePath(path.join('media', 'axiom.svg')),
            gutterIconSize: '75%',
        }),
        theorem: window.createTextEditorDecorationType({
            gutterIconPath: context.asAbsolutePath(path.join('media', 'theorem.svg')),
            gutterIconSize: '75%',
        }),
    };
    context.subscriptions.push(markers.axiom, markers.theorem);

    const refresh = async (editor: TextEditor) => {
        if (editor.document.languageId !== 'unilog' || !client) return;
        const found = await client.sendRequest<DeclarationMarker[]>('unilog/declarationMarkers', {
            uri: editor.document.uri.toString(),
        });
        for (const kind of ['axiom', 'theorem'] as const)
            editor.setDecorations(
                markers[kind],
                found.filter((m) => m.kind === kind).map((m) => new Range(m.line, 0, m.line, 0)),
            );
    };
    const refreshVisible = () => window.visibleTextEditors.forEach((e) => void refresh(e));

    // After edits, update once typing pauses.
    let pending: NodeJS.Timeout | undefined;
    context.subscriptions.push(
        window.onDidChangeVisibleTextEditors(refreshVisible),
        workspace.onDidChangeTextDocument((e) => {
            if (e.document.languageId !== 'unilog') return;
            clearTimeout(pending);
            pending = setTimeout(() => {
                window.visibleTextEditors.filter((ed) => ed.document === e.document).forEach((ed) => void refresh(ed));
            }, 200);
        }),
    );
    refreshVisible();
}

export function deactivate(): Promise<void> | undefined {
    return client?.stop();
}
