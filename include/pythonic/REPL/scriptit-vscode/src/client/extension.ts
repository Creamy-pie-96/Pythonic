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

// ── ScriptIt Token Colors ────────────────────────────────
// These get injected into editor.tokenColorCustomizations
// so they work with ANY theme the user has selected.
const SCRIPTIT_TOKEN_RULES = [
    // var keyword → white bold
    { scope: 'storage.type.var.scriptit', settings: { foreground: '#FFFFFF', fontStyle: 'bold' } },
    // fn, give → yellow bold (special keywords)
    { scope: 'keyword.declaration.function.scriptit', settings: { foreground: '#e5c07b', fontStyle: 'bold' } },
    { scope: 'keyword.control.return.scriptit', settings: { foreground: '#e5c07b', fontStyle: 'bold' } },
    { scope: 'keyword.other.special.scriptit', settings: { foreground: '#e5c07b', fontStyle: 'bold' } },
    { scope: 'storage.modifier.reference.scriptit', settings: { foreground: '#e5c07b', fontStyle: 'bold' } },
    // Control keywords (if, else, for, while, break, return) → red bold
    { scope: 'keyword.control.flow.scriptit', settings: { foreground: '#e06c75', fontStyle: 'bold' } },
    { scope: 'keyword.control.loop.scriptit', settings: { foreground: '#e06c75', fontStyle: 'bold' } },
    { scope: 'keyword.control.context.scriptit', settings: { foreground: '#e06c75', fontStyle: 'bold' } },
    // Type conversions → teal
    { scope: 'support.type.conversion.scriptit', settings: { foreground: '#008080' } },
    // Variables / identifiers → white/light gray
    { scope: 'variable.other.scriptit', settings: { foreground: '#eeeeee' } },
    { scope: 'variable.other.declaration.scriptit', settings: { foreground: '#eeeeee' } },
    { scope: 'variable.other.loop.scriptit', settings: { foreground: '#eeeeee' } },
    { scope: 'variable.other.reference.scriptit', settings: { foreground: '#eeeeee' } },
    { scope: 'variable.parameter.scriptit', settings: { foreground: '#eeeeee' } },
    { scope: 'variable.parameter.reference.scriptit', settings: { foreground: '#eeeeee' } },
    // Functions & methods → cyan
    { scope: 'entity.name.function.definition.scriptit', settings: { foreground: '#00ffff' } },
    { scope: 'entity.name.function.forward.scriptit', settings: { foreground: '#00ffff' } },
    { scope: 'entity.name.function.call.scriptit', settings: { foreground: '#00ffff' } },
    { scope: 'entity.name.function.method.scriptit', settings: { foreground: '#00ffff' } },
    { scope: 'support.function.builtin.scriptit', settings: { foreground: '#00ffff' } },
    { scope: 'support.function.math.scriptit', settings: { foreground: '#00ffff' } },
    // Numbers → orange
    { scope: 'constant.numeric.float.scriptit', settings: { foreground: '#d19a66' } },
    { scope: 'constant.numeric.integer.scriptit', settings: { foreground: '#d19a66' } },
    // Booleans & None → purple
    { scope: 'constant.language.boolean.true.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'constant.language.boolean.false.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'constant.language.none.scriptit', settings: { foreground: '#c678dd' } },
    // Strings → green
    { scope: 'string.quoted.double.scriptit', settings: { foreground: '#98c379' } },
    { scope: 'string.quoted.single.scriptit', settings: { foreground: '#98c379' } },
    { scope: 'constant.character.escape.scriptit', settings: { foreground: '#d19a66' } },
    // Comments: # → gray italic, --> → green
    { scope: 'comment.line.number-sign.scriptit', settings: { foreground: '#5c6370', fontStyle: 'italic' } },
    { scope: 'comment.block.arrow.scriptit', settings: { foreground: '#98c379' } },
    { scope: 'punctuation.definition.comment.begin.scriptit', settings: { foreground: '#98c379' } },
    { scope: 'punctuation.definition.comment.end.scriptit', settings: { foreground: '#98c379' } },
    // Operators → purple
    { scope: 'keyword.operator.comparison.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.arithmetic.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.assignment.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.assignment.compound.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.increment.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.logical.scriptit', settings: { foreground: '#c678dd' } },
    { scope: 'keyword.operator.logical.symbol.scriptit', settings: { foreground: '#c678dd' } },
    // Punctuation → light gray
    { scope: 'punctuation.bracket.round.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.bracket.square.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.bracket.curly.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.separator.comma.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.separator.colon.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.separator.parameter.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.terminator.block.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.terminator.statement.scriptit', settings: { foreground: '#abb2bf' } },
    { scope: 'punctuation.definition.function.scriptit', settings: { foreground: '#abb2bf' } },
];

/**
 * Inject ScriptIt token colors into the user's settings.
 * configurationDefaults doesn't reliably work for tokenColorCustomizations,
 * so we set them programmatically. Only injects once (checks for existing rules).
 */
async function applyScriptItColors() {
    try {
        const config = vscode.workspace.getConfiguration('editor');
        const current: Record<string, unknown> = config.get('tokenColorCustomizations') || {};
        const existingRules: Array<{ scope?: string }> = (current as any).textMateRules || [];

        // Check if ScriptIt rules are already present
        const hasScriptIt = existingRules.some(
            (r) => typeof r.scope === 'string' && r.scope.endsWith('.scriptit')
        );
        if (hasScriptIt) return;

        // Merge our rules with any existing user rules
        const merged = {
            ...current,
            textMateRules: [...existingRules, ...SCRIPTIT_TOKEN_RULES]
        };

        await config.update('tokenColorCustomizations', merged, vscode.ConfigurationTarget.Global);
        console.log('ScriptIt: Token colors applied.');
    } catch (err) {
        console.error('ScriptIt: Failed to apply token colors:', err);
    }
}

export function activate(context: vscode.ExtensionContext) {
    console.log('ScriptIt extension activating...');

    // ── Apply Colors ─────────────────────────────────────
    applyScriptItColors();

    // ── Notebook Support (register FIRST — must not be blocked by LSP) ──
    context.subscriptions.push(
        vscode.workspace.registerNotebookSerializer(
            'scriptit-notebook',
            new ScriptItNotebookSerializer(),
            { transientOutputs: false }
        )
    );

    notebookController = new ScriptItNotebookController();
    context.subscriptions.push({ dispose: () => notebookController.dispose() });

    // ── Language Server (wrapped in try/catch — must not break notebook) ──
    try {
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
    } catch (err) {
        console.error('ScriptIt: Language server failed to start:', err);
        vscode.window.showWarningMessage(
            'ScriptIt language server failed to start. Syntax highlighting and notebooks still work.'
        );
    }

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
