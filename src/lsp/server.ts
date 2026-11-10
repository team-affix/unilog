#!/usr/bin/env node
import path from 'path';
import { fileURLToPath, pathToFileURL } from 'url';
import {
    Connection,
    createConnection,
    Diagnostic,
    DiagnosticSeverity,
    InlayHint,
    InlayHintKind,
    MarkupKind,
    ProposedFeatures,
    SymbolKind,
    TextDocuments,
    TextDocumentSyncKind,
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';
import { VERSION } from '../version';
import { diskSources, EXTENSION, SourceProvider } from '../workspace';
import {
    analyze,
    Analysis,
    declarationMarkers,
    definition,
    findProjectDir,
    hover,
    inlayHints,
    outline,
    problems,
} from './features';

const toPath = (uri: string) => path.resolve(fileURLToPath(uri));
const toUri = (file: string) => pathToFileURL(file).toString();

/**
 * Runs the Unilog language server on a connection. Every open document is checked as part of its project
 * (the nearest unilog.json above it), using the editor's unsaved text for every open file.
 */
export function listen(connection: Connection): void {
    const documents = new TextDocuments(TextDocument);
    let workspaceRoots: string[] = [];
    let canRefreshHints = false;

    /** Latest analysis per project directory, and the files it last reported problems for. */
    const analyses = new Map<string, Analysis>();
    const reported = new Map<string, Set<string>>();
    const pending = new Map<string, NodeJS.Timeout>();

    /** Reads open documents from the editor, everything else from disk. */
    const sources: SourceProvider = {
        exists: (file) => documents.get(toUri(file)) !== undefined || diskSources.exists(file),
        read: (file) => documents.get(toUri(file))?.getText() ?? diskSources.read(file),
    };

    const projectOf = (file: string) => {
        const root = workspaceRoots.find((r) => !path.relative(r, file).startsWith('..'));
        return findProjectDir(file, root);
    };

    function check(projectDir: string): Analysis {
        const open = documents
            .all()
            .map((d) => toPath(d.uri))
            .filter((f) => f.endsWith(EXTENSION) && projectOf(f) === projectDir);
        const a = analyze(projectDir, sources, open);
        analyses.set(projectDir, a);

        const byFile = new Map<string, Diagnostic[]>();
        for (const p of problems(a)) {
            const list = byFile.get(p.file) ?? [];
            list.push({ range: p.range, message: p.message, severity: DiagnosticSeverity.Error, source: 'unilog' });
            byFile.set(p.file, list);
        }
        // Clear files that had problems last time but don't now.
        for (const file of reported.get(projectDir) ?? []) if (!byFile.has(file)) byFile.set(file, []);
        for (const [file, diagnostics] of byFile) void connection.sendDiagnostics({ uri: toUri(file), diagnostics });
        reported.set(projectDir, new Set([...byFile].filter(([, d]) => d.length > 0).map(([f]) => f)));
        if (canRefreshHints) void connection.languages.inlayHint.refresh();
        return a;
    }

    /** Checks soon, so a burst of keystrokes causes one check. */
    function schedule(projectDir: string): void {
        clearTimeout(pending.get(projectDir));
        pending.set(
            projectDir,
            setTimeout(() => {
                pending.delete(projectDir);
                check(projectDir);
            }, 150),
        );
    }

    /** The current analysis for a document's project, checking now if there is none yet or one is pending. */
    function analysisFor(uri: string): Analysis {
        const dir = projectOf(toPath(uri));
        const timer = pending.get(dir);
        if (timer) {
            clearTimeout(timer);
            pending.delete(dir);
            return check(dir);
        }
        return analyses.get(dir) ?? check(dir);
    }

    connection.onInitialize((params) => {
        workspaceRoots = (params.workspaceFolders ?? []).map((f) => toPath(f.uri));
        if (workspaceRoots.length === 0 && params.rootUri) workspaceRoots = [toPath(params.rootUri)];
        canRefreshHints = !!params.capabilities.workspace?.inlayHint?.refreshSupport;
        return {
            capabilities: {
                textDocumentSync: TextDocumentSyncKind.Incremental,
                hoverProvider: true,
                definitionProvider: true,
                documentSymbolProvider: true,
                inlayHintProvider: true,
            },
            serverInfo: { name: 'unilog', version: VERSION },
        };
    });

    documents.onDidOpen((e) => schedule(projectOf(toPath(e.document.uri))));
    documents.onDidChangeContent((e) => schedule(projectOf(toPath(e.document.uri))));
    documents.onDidClose(() => {
        for (const dir of analyses.keys()) schedule(dir);
    });
    // unilog.json or files on disk changed: re-check everything.
    connection.onDidChangeWatchedFiles(() => {
        for (const dir of analyses.keys()) schedule(dir);
    });

    connection.onHover(({ textDocument, position }) => {
        const info = hover(analysisFor(textDocument.uri), toPath(textDocument.uri), position);
        // The range tells the editor what the hover describes, so it highlights exactly that.
        return info ? { contents: { kind: MarkupKind.Markdown, value: info.markdown }, range: info.range } : null;
    });

    connection.onDefinition(({ textDocument, position }) => {
        const target = definition(analysisFor(textDocument.uri), toPath(textDocument.uri), position);
        return target ? { uri: toUri(target.file), range: target.range } : null;
    });

    connection.onDocumentSymbol(({ textDocument }) =>
        outline(analysisFor(textDocument.uri), toPath(textDocument.uri)).map((e) => ({
            name: e.name,
            detail: e.detail,
            kind: e.kind === 'axiom' ? SymbolKind.Constant : SymbolKind.Function,
            range: e.range,
            selectionRange: e.selectionRange,
        })),
    );

    connection.languages.inlayHint.on(({ textDocument }): InlayHint[] =>
        inlayHints(analysisFor(textDocument.uri), toPath(textDocument.uri)).map((h) => ({
            position: h.position,
            label: h.label,
            kind: InlayHintKind.Type,
            paddingLeft: true,
            tooltip: 'Inferred claim. Double-click to write it into the file.',
            textEdits: [{ range: { start: h.position, end: h.position }, newText: h.insert }],
        })),
    );

    // Unilog-specific request, used by the VS Code extension for gutter markers: which lines start axioms
    // and which start theorems. Parse-only, so markers stay while the file has checking errors.
    connection.onRequest('unilog/declarationMarkers', ({ uri }: { uri: string }) => {
        const doc = documents.get(uri);
        const text = doc ? doc.getText() : sources.read(toPath(uri));
        return declarationMarkers(text, toPath(uri));
    });

    documents.listen(connection);
    connection.listen();
}

if (require.main === module) listen(createConnection(ProposedFeatures.all));
