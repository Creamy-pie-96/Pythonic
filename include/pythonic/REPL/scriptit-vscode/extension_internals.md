# ScriptIt VS Code Extension — Internals Guide

This document explains the extension's architecture and teaches you how to add
new grammar rules, highlighting, diagnostics, completions, and hover info.

---

## Architecture Overview

```
scriptit-vscode/
├── package.json                    ← Extension manifest (THE master config)
├── language-configuration.json     ← Bracket/comment/indent rules
├── syntaxes/
│   └── scriptit.tmLanguage.json    ← TextMate grammar (syntax highlighting)
├── snippets/
│   └── scriptit.json               ← Code snippets
├── themes/
│   └── scriptit-icon-theme.json    ← File icon definitions
├── images/
│   ├── icon.svg                    ← Extension marketplace icon
│   ├── sit-icon.svg                ← .sit file icon
│   └── nsit-icon.svg               ← .nsit file icon
└── src/
    ├── client/
    │   ├── extension.ts            ← Entry point (activates everything)
    │   ├── notebookSerializer.ts   ← Reads/writes .nsit JSON format
    │   └── notebookController.ts   ← Kernel management & cell execution
    └── server/
        ├── server.ts               ← Language Server (LSP) — main logic
        ├── diagnostics.ts          ← Subprocess error validation
        ├── completions.ts          ← All auto-completion items
        └── hover.ts                ← Hover tooltips & signature info
```

### How VS Code loads the extension:

1. User opens a `.sit` file → VS Code checks `package.json` → sees `"activationEvents": ["onLanguage:scriptit"]`
2. VS Code runs `out/client/extension.js` (the compiled `extension.ts`)
3. `extension.ts` starts the **Language Server** (`out/server/server.js`) via IPC
4. The server handles: diagnostics, completions, hover, signature help
5. **TextMate grammar** handles syntax highlighting (independent of the server)
6. **Notebook serializer** handles `.nsit` file open/save
7. **Notebook controller** spawns the kernel for cell execution

---

## How Syntax Highlighting Works

### The TextMate Grammar System

VS Code uses **TextMate grammars** — a system of regex patterns that assign **scopes**
to text. Your theme then maps scopes to colors.

The grammar lives in `syntaxes/scriptit.tmLanguage.json`.

### Key Concepts:

```json
{
  "scopeName": "source.scriptit",     // Root scope for all ScriptIt files
  "patterns": [                        // Top-level patterns, tried in ORDER
    { "include": "#comments" },        // First: comments (override everything)
    { "include": "#strings" },         // Second: strings
    { "include": "#function-definition" },
    ...
    { "include": "#identifiers" }      // LAST: catch-all for remaining words
  ],
  "repository": {                      // Named pattern groups
    "comments": { "patterns": [...] },
    "strings": { "patterns": [...] },
    ...
  }
}
```

**ORDER MATTERS!** Patterns are tried top to bottom. The first match wins.
That's why `builtin-functions` comes before `function-call` — we want `print(`
to match as a builtin, not a generic function call.

### Pattern Types:

#### 1. Simple Match (one line)
```json
{
  "name": "keyword.control.flow.scriptit",    // Scope to assign
  "match": "\\b(if|elif|else|for|while)\\b"   // Regex pattern
}
```

#### 2. Match with Captures (parts get different scopes)
```json
{
  "match": "\\b(fn)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
  "captures": {
    "1": { "name": "keyword.declaration.function.scriptit" },  // fn
    "2": { "name": "entity.name.function.definition.scriptit" } // function name
  }
}
```

#### 3. Begin/End (multi-character or multi-line regions)
```json
{
  "begin": "\"",
  "end": "\"",
  "name": "string.quoted.double.scriptit",
  "patterns": [
    { "name": "constant.character.escape.scriptit", "match": "\\\\." }
  ]
}
```

#### 4. Begin/End with Sub-patterns (e.g., function parameters)
```json
{
  "begin": "\\b(fn)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\(",
  "beginCaptures": {
    "1": { "name": "keyword.declaration.function.scriptit" },
    "2": { "name": "entity.name.function.definition.scriptit" }
  },
  "end": "\\)\\s*(:|\\.)",
  "patterns": [
    {
      "name": "variable.parameter.scriptit",
      "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"
    }
  ]
}
```
This captures everything between `fn name(` and `):` — each identifier inside
becomes `variable.parameter`.

### Scope Naming Convention:

VS Code themes map scopes to colors. Use standard names:

| Scope prefix | What it colors | Example |
|-------------|---------------|---------|
| `keyword.*` | Language keywords | `if`, `fn`, `var` |
| `entity.name.function.*` | Function/method names | `add`, `myFunc` |
| `variable.parameter.*` | Function parameters | `x`, `y` in `fn f(x, y)` |
| `variable.other.*` | Variables | `x` in `var x = 10` |
| `support.function.*` | Built-in functions | `print`, `sin` |
| `constant.numeric.*` | Numbers | `42`, `3.14` |
| `constant.language.*` | Language constants | `True`, `False`, `None` |
| `string.quoted.*` | String literals | `"hello"` |
| `comment.*` | Comments | `# note` |
| `storage.modifier.*` | Modifiers | `@` in `@param` |
| `punctuation.*` | Punctuation | `.`, `;`, `,` |

### How Variable Highlighting Works:

For `var x, b = ...`:
```json
"var-declaration": {
  "patterns": [{
    "begin": "\\b(var)\\s+",          // Match "var " — keyword scope
    "beginCaptures": {
      "1": { "name": "keyword.declaration.variable.scriptit" }
    },
    "end": "(?==)|$",                  // Stop at = or end of line
    "patterns": [{
      "name": "variable.other.declaration.scriptit",
      "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\b"   // Each name inside
    }]
  }]
}
```
Result: `var` → blue keyword, `x` → light blue variable, `b` → light blue variable.

For function calls like `myFunc(x)`:
```json
"function-call": {
  "patterns": [{
    "match": "\\b([a-zA-Z_][a-zA-Z0-9_]*)\\s*(?=\\()",
    "captures": {
      "1": { "name": "entity.name.function.call.scriptit" }
    }
  }]
}
```
Result: `myFunc` → yellow (function call color).

---

## How to Add New Grammar Rules

### Example: Adding a `class` keyword

1. **Add the pattern** in `scriptit.tmLanguage.json`:

```json
// In the "repository" section, add:
"class-definition": {
  "patterns": [{
    "match": "\\b(class)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*(:)",
    "captures": {
      "1": { "name": "keyword.declaration.class.scriptit" },
      "2": { "name": "entity.name.type.class.scriptit" },
      "3": { "name": "punctuation.definition.class.scriptit" }
    }
  }]
}
```

2. **Include it in the top-level patterns** (before `identifiers`!):
```json
"patterns": [
    ...
    { "include": "#class-definition" },  // ADD HERE
    ...
    { "include": "#identifiers" }        // Keep this LAST
]
```

3. **Test**: Restart the extension host (F5) and open a `.sit` file.

### Example: Adding a `lambda` expression

```json
"lambda-expression": {
  "patterns": [{
    "match": "\\b(lambda)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*:",
    "captures": {
      "1": { "name": "keyword.other.lambda.scriptit" },
      "2": { "name": "variable.parameter.lambda.scriptit" }
    }
  }]
}
```

### Testing Grammar Changes:

- Use **Developer: Inspect Editor Tokens and Scopes** (Ctrl+Shift+P → type it)
- Click on any token to see its assigned scope
- If the scope is wrong, your pattern order or regex needs adjusting

---

## How to Add Completions

### Adding a New Builtin Function

Edit `src/server/completions.ts`:

```typescript
// In the SCRIPTIT_BUILTINS array, add:
{ label: 'myNewFunc', kind: CompletionItemKind.Function,
  detail: 'Description of what it does', data: 'fn_myNewFunc' },
```

### Adding a New Keyword

```typescript
// In the SCRIPTIT_KEYWORDS array, add:
{ label: 'myKeyword', kind: CompletionItemKind.Keyword,
  detail: 'What this keyword does', data: 'kw_myKeyword' },
```

### Adding Method Completions (after dot)

Edit `src/server/server.ts`, in `getMethodCompletions()`:

```typescript
// Add to the methods array:
{ label: 'myMethod', detail: 'Description', kind: CompletionItemKind.Method },
```

After editing, recompile: `npx tsc -b` (or use the watch task: `npx tsc -b -w`)

---

## How to Add Hover Documentation

Edit `src/server/hover.ts`:

```typescript
// Add a new entry to HOVER_INFO:
myNewFunc: {
    signature: 'myNewFunc(x, y)',
    detail: 'Computes something amazing with x and y.',
    params: ['x — first parameter', 'y — second parameter']
},
```

This automatically provides:
- **Hover tooltip**: Shows signature + description when hovering
- **Signature help**: Shows parameter info when typing `myNewFunc(`

---

## How to Add Diagnostics (Error Checks)

### Adding a Static Check (no subprocess)

Edit `src/server/server.ts`, in `validateTextDocument()`:

```typescript
// After the existing static checks, add:
// Check for deprecated function
if (/\boldFunc\s*\(/.test(trimmed)) {
    diagnostics.push({
        severity: DiagnosticSeverity.Warning,
        range: {
            start: { line: i, character: codePart.indexOf('oldFunc') },
            end: { line: i, character: codePart.indexOf('oldFunc') + 7 }
        },
        message: `'oldFunc' is deprecated. Use 'newFunc' instead.`,
        source: 'scriptit'
    });
}
```

### Adding Subprocess Error Patterns

Edit `src/server/diagnostics.ts`, in `parseErrors()`:

```typescript
// Add a new regex pattern:
// Pattern: MyCustomError at position N: message
match = trimmed.match(/MyCustomError\s+at\s+position\s+(\d+)\s*:\s*(.+)/i);
if (match) {
    const lineNum = Math.max(0, parseInt(match[1]) - 1);
    diagnostics.push(this.createDiagnostic(lineNum, match[2], sourceLines, DiagnosticSeverity.Error));
    continue;
}
```

---

## How to Add Snippets

Edit `snippets/scriptit.json`:

```json
"My Snippet": {
    "prefix": "mysnip",
    "body": [
        "fn ${1:name}(${2:params}):",
        "    ${3:# body}",
        ";",
        ""
    ],
    "description": "Description of what this snippet creates"
}
```

Snippet syntax:
- `${1:placeholder}` — Tab stop 1 with default text
- `$0` — Final cursor position
- `${1|choice1,choice2|}` — Drop-down choice

---

## Notebook Architecture

### NotebookSerializer (`notebookSerializer.ts`)

Converts between `.nsit` JSON format and VS Code's internal `NotebookData`:

```
.nsit file (JSON) ←→ NotebookSerializer ←→ VS Code NotebookData
```

- `deserializeNotebook()`: JSON → NotebookData (on file open)
- `serializeNotebook()`: NotebookData → JSON (on file save)

### NotebookController (`notebookController.ts`)

Manages the ScriptIt kernel subprocess:

```
Cell execution request
    → ensureKernel() — spawns `scriptit --kernel` if not running
    → sendToKernel({ action: "execute", code: "..." })
    → waitForResponse() — reads JSON from kernel stdout
    → Maps response to cell outputs
```

The kernel protocol is JSON over stdin/stdout:
```
Request:  {"action": "execute", "cell_id": "...", "code": "..."}
Response: {"cell_id": "...", "status": "ok", "stdout": "...", "stderr": "..."}
```

---

## Language Server Protocol (LSP)

The server (`server.ts`) speaks LSP over IPC:

```
VS Code (client)  ←── IPC ──→  Language Server (server.ts)
```

### Request flow:

1. User types → `textDocument/didChange` → server receives new text
2. Server runs `validateTextDocument()` → sends `textDocument/publishDiagnostics`
3. User types `pri` → `textDocument/completion` → server returns completion items
4. User hovers over `print` → `textDocument/hover` → server returns hover info
5. User types `print(` → `textDocument/signatureHelp` → server returns signature

### Adding a new LSP feature:

1. Register the capability in `connection.onInitialize()`:
```typescript
capabilities: {
    ...
    documentFormattingProvider: true,  // NEW
}
```

2. Add the handler:
```typescript
connection.onDocumentFormatting((params) => {
    // Return TextEdit[] with formatting changes
    return [];
});
```

---

## Build & Test Workflow

```bash
# One-time setup
npm install

# Compile (one-shot)
npx tsc -b

# Compile (watch mode — recompiles on save)
npx tsc -b -w

# Package into .vsix
npx @vscode/vsce package --allow-missing-repository

# Install into VS Code
code --install-extension scriptit-lang-0.1.0.vsix

# Debug: Press F5 in VS Code with the extension folder open
# This launches an Extension Development Host window
```

### Testing Changes:

1. **Grammar changes**: Just reload window (Ctrl+Shift+P → "Reload Window")
2. **Server changes**: Recompile (`npx tsc -b`) → Reload window
3. **Client changes**: Recompile → Restart Extension Development Host (F5)
4. **Snippet changes**: Reload window

### Inspecting Tokens:

Use `Ctrl+Shift+P` → "Developer: Inspect Editor Tokens and Scopes"
Click on any character to see:
- Token type
- Assigned scope (e.g., `keyword.control.flow.scriptit`)
- Theme color applied

---

## File Map (quick reference)

| I want to... | Edit this file |
|-------------|---------------|
| Add syntax highlighting for a new keyword | `syntaxes/scriptit.tmLanguage.json` |
| Add a new auto-completion item | `src/server/completions.ts` |
| Add hover documentation | `src/server/hover.ts` |
| Add a new error check (static) | `src/server/server.ts` → `validateTextDocument()` |
| Add a new error pattern (subprocess) | `src/server/diagnostics.ts` → `parseErrors()` |
| Add a code snippet | `snippets/scriptit.json` |
| Change bracket/comment/indent behavior | `language-configuration.json` |
| Add a new command | `src/client/extension.ts` + `package.json` commands |
| Change file associations | `package.json` → languages |
| Add a new file icon | `images/` + `themes/scriptit-icon-theme.json` |
| Modify notebook serialization | `src/client/notebookSerializer.ts` |
| Modify kernel execution | `src/client/notebookController.ts` |
