// ═══════════════════════════════════════════════════════════
// ScriptIt VS Code Extension — Client (entry point)
// ═══════════════════════════════════════════════════════════

import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';
import { ScriptItNotebookSerializer } from './notebookSerializer';
import { ScriptItNotebookController } from './notebookController';

let client: LanguageClient;
let notebookController: ScriptItNotebookController;

export function activate(context: vscode.ExtensionContext) {
    console.log('ScriptIt extension activating...');

    // ── Language Server ──────────────────────────────────
    const serverModule = context.asAbsolutePath(
        path.join('out', 'server', 'server.js')
    );

    const debugOptions = { execArgv: ['--nolazy', '--inspect=6009'] };

    const serverOptions: ServerOptions = {
        run: { module: serverModule, transport: TransportKind.ipc },
        debug: {
            module: serverModule,
            transport: TransportKind.ipc,
            options: debugOptions
        }
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [
            { scheme: 'file', language: 'scriptit' },
            { scheme: 'untitled', language: 'scriptit' }
        ],
        synchronize: {
            configurationSection: 'scriptit',
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.{si,sit}')
        }
    };

    client = new LanguageClient(
        'scriptitLanguageServer',
        'ScriptIt Language Server',
        serverOptions,
        clientOptions
    );

    client.start();

    // ── Notebook Support ─────────────────────────────────
    context.subscriptions.push(
        vscode.workspace.registerNotebookSerializer(
            'scriptit-notebook',
            new ScriptItNotebookSerializer(),
            { transientOutputs: false }
        )
    );

    notebookController = new ScriptItNotebookController();
    context.subscriptions.push({ dispose: () => notebookController.dispose() });

    // ── Run File Command ─────────────────────────────────
    const runFileCmd = vscode.commands.registerCommand('scriptit.runFile', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('No active ScriptIt file to run.');
            return;
        }

        const filePath = editor.document.fileName;
        const config = vscode.workspace.getConfiguration('scriptit');
        const scriptitPath = config.get<string>('scriptitPath', 'scriptit');

        // Save before running
        editor.document.save().then(() => {
            const terminal = vscode.window.createTerminal('ScriptIt');
            terminal.show();
            terminal.sendText(`${scriptitPath} --script < "${filePath}"`);
        });
    });

    // ── Run Selection Command ────────────────────────────
    const runSelectionCmd = vscode.commands.registerCommand('scriptit.runSelection', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) return;

        const selection = editor.document.getText(editor.selection);
        if (!selection) {
            vscode.window.showWarningMessage('No text selected.');
            return;
        }

        const config = vscode.workspace.getConfiguration('scriptit');
        const scriptitPath = config.get<string>('scriptitPath', 'scriptit');

        const terminal = vscode.window.createTerminal('ScriptIt');
        terminal.show();
        terminal.sendText(`echo '${selection.replace(/'/g, "'\\''")}' | ${scriptitPath} --script`);
    });

    // ── Open Notebook Command ────────────────────────────
    const openNotebookCmd = vscode.commands.registerCommand('scriptit.openNotebook', () => {
        const config = vscode.workspace.getConfiguration('scriptit');
        const scriptitPath = config.get<string>('scriptitPath', 'scriptit');

        // Try to find the notebook server script
        const terminal = vscode.window.createTerminal('ScriptIt Notebook');
        terminal.show();
        terminal.sendText('scriptit-notebook || echo "Notebook server not found. Install ScriptIt system-wide first."');
    });

    context.subscriptions.push(runFileCmd, runSelectionCmd, openNotebookCmd);

    console.log('ScriptIt extension activated.');
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
