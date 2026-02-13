# ScriptIt Notebook — Internal Architecture

> Complete technical documentation for rebuilding the ScriptIt Notebook system from scratch.
> This covers every layer: the kernel, the server, the GUI, and the protocol that ties them together.

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Architecture Diagram](#architecture-diagram)
3. [The Kernel](#the-kernel)
   - [Startup & Ready Signal](#startup--ready-signal)
   - [The Kernel Protocol (JSON over stdio)](#the-kernel-protocol)
   - [Actions Reference](#actions-reference)
   - [Kernel Internals (C++)](#kernel-internals-c)
   - [State Persistence Between Cells](#state-persistence-between-cells)
   - [Stdout Capture](#stdout-capture)
   - [Error Handling](#error-handling)
4. [The Python Server](#the-python-server)
   - [Server Architecture](#server-architecture)
   - [API Endpoints Reference](#api-endpoints-reference)
   - [Kernel Lifecycle Management](#kernel-lifecycle-management)
   - [Request/Response Flow](#requestresponse-flow)
   - [File Serving](#file-serving)
   - [Directory Browsing](#directory-browsing)
5. [The File Format (.nsit)](#the-file-format-nsit)
   - [Schema](#schema)
   - [Cell Structure](#cell-structure)
   - [Example Notebook File](#example-notebook-file)
6. [The GUI (index.html)](#the-gui)
   - [State Management](#state-management)
   - [Cell Types & Rendering](#cell-types--rendering)
   - [Toolbar](#toolbar)
   - [Keyboard Shortcuts](#keyboard-shortcuts)
   - [Markdown Renderer](#markdown-renderer)
   - [Save/Load System](#saveload-system)
   - [UI Components](#ui-components)
7. [End-to-End Walkthrough](#end-to-end-walkthrough)
8. [Failure Modes & Recovery](#failure-modes--recovery)
9. [Design Decisions & Rationale](#design-decisions--rationale)

---

## System Overview

The ScriptIt Notebook is a browser-based interactive coding environment — similar in
concept to Jupyter Notebook — purpose-built for the ScriptIt language. It lets you write
code in cells, execute them against a persistent kernel, interleave markdown documentation,
and save/load your work as `.nsit` files.

The system is composed of **three independent processes** that communicate through well-defined interfaces:

| Component           | Technology                       | Role                                             |
| ------------------- | -------------------------------- | ------------------------------------------------ |
| **ScriptIt Kernel** | C++ binary (`scriptit --kernel`) | Executes ScriptIt code, maintains variable state |
| **Notebook Server** | Python (`notebook_server.py`)    | HTTP server, kernel manager, file I/O            |
| **Notebook GUI**    | HTML/CSS/JS (`index.html`)       | Browser-based cell editor, renderer, UX          |

No component knows the internals of the others. The kernel doesn't know about HTTP.
The GUI doesn't know about subprocesses. The server is the bridge.

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        USER'S BROWSER                               │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                     index.html (GUI)                          │  │
│  │                                                               │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐ │  │
│  │  │ Code     │ │ Markdown │ │ Code     │ │ Toolbar          │ │  │
│  │  │ Cell 1   │ │ Cell 2   │ │ Cell 3   │ │ [Run][Save][...] │ │  │
│  │  │          │ │          │ │          │ │ Status: ● idle    │ │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘ │  │
│  └───────────────────────────┬───────────────────────────────────┘  │
│                              │                                      │
│                         HTTP (fetch)                                │
│                              │                                      │
└──────────────────────────────┼──────────────────────────────────────┘
                               │
                    ┌──────────▼──────────┐
                    │  notebook_server.py  │
                    │  (Python HTTP)       │
                    │                      │
                    │  • Serves index.html │
                    │  • REST API          │
                    │  • Kernel manager    │
                    │  • File I/O (.nsit)  │
                    │  • Dir browser       │
                    │                      │
                    │  Port 8888 (default) │
                    └──────────┬───────────┘
                               │
                      stdin/stdout (JSON)
                               │
                    ┌──────────▼───────────┐
                    │  scriptit --kernel    │
                    │  (C++ subprocess)     │
                    │                       │
                    │  • Parses ScriptIt    │
                    │  • Persistent scope   │
                    │  • Captures stdout    │
                    │  • Returns results    │
                    └───────────────────────┘
```

### Communication Summary

```
Browser (index.html)  ←── HTTP/REST ──→  Python Server (notebook_server.py)
                                                │
                                         stdin/stdout (JSON)
                                                │
                                         ScriptIt Kernel (scriptit --kernel)
```

Every user action (run cell, save, restart) becomes an HTTP request to the server.
If code execution is involved, the server forwards it as a JSON message to the kernel's stdin,
reads the JSON response from the kernel's stdout, and relays it back as an HTTP response.

---

## The Kernel

The kernel is the execution engine. It is the `scriptit` binary run with the `--kernel` flag,
which puts it into a special mode: instead of running a file or opening a REPL, it sits in a
loop reading JSON commands from stdin and writing JSON responses to stdout.

### Startup & Ready Signal

When the kernel process starts, it performs its initialization (setting up the global scope,
built-in functions, etc.) and then emits a single JSON line to stdout:

```json
{ "status": "kernel_ready", "version": "2.0" }
```

**This is critical.** The server must wait for this line before sending any commands.
It serves as:

- Confirmation the kernel launched successfully
- A version handshake (the server can check compatibility)
- A synchronization point (the server knows stdin/stdout are connected)

If the server doesn't receive this within a timeout (e.g. 5 seconds), it should assume
the kernel failed to start.

### The Kernel Protocol

All communication is **newline-delimited JSON** over stdin/stdout:

- **Server → Kernel**: Write one JSON object per line to the kernel's stdin, terminated by `\n`
- **Kernel → Server**: Write one JSON object per line to stdout, terminated by `\n`

**Important constraints:**

- One request, one response. The protocol is synchronous from the server's perspective.
- The kernel must not write anything to stdout except protocol JSON. Debug output goes to stderr.
- Each JSON message is a single line (no pretty-printing in the wire format).

```
Server writes to stdin:  {"action":"execute","cell_id":"c1","code":"print(42)."}\n
Kernel writes to stdout: {"cell_id":"c1","status":"ok","stdout":"42\n","stderr":"","result":"","execution_count":1}\n
```

### Actions Reference

#### 1. `execute` — Run Code

**Request:**

```json
{
  "action": "execute",
  "cell_id": "c1abc12345",
  "code": "var x = 10.\nprint(x)."
}
```

**Successful Response:**

```json
{
  "cell_id": "c1abc12345",
  "status": "ok",
  "stdout": "10\n",
  "stderr": "",
  "result": "",
  "execution_count": 1
}
```

**Error Response:**

```json
{
  "cell_id": "c1abc12345",
  "status": "error",
  "stdout": "",
  "stderr": "NameError: variable 'y' is not defined",
  "result": "",
  "execution_count": 2
}
```

Fields explained:
| Field | Description |
|-------|-------------|
| `cell_id` | Echoed back from the request, so the GUI knows which cell this belongs to |
| `status` | `"ok"` or `"error"` |
| `stdout` | Everything the code printed to standard output |
| `stderr` | Error messages (parse errors, runtime errors, etc.) |
| `result` | The value from a top-level `give` statement (like a return value for the cell) |
| `execution_count` | Monotonically increasing counter across all executions in this kernel session |

#### 2. `reset` — Clear State

Wipes all variables from the global scope but keeps the kernel process alive.
Equivalent to restarting without the subprocess overhead.

**Request:**

```json
{
  "action": "reset"
}
```

**Response:**

```json
{
  "status": "reset_ok"
}
```

After a reset:

- All user-defined variables are gone
- The execution counter resets
- Built-in functions remain available
- The kernel is ready for new `execute` commands immediately

#### 3. `shutdown` — Kill the Kernel

Tells the kernel to exit cleanly.

**Request:**

```json
{
  "action": "shutdown"
}
```

**Response:**

```json
{
  "status": "shutdown_ok"
}
```

After writing this response, the kernel process exits with code 0.
The server should then wait for the process to actually terminate.

#### 4. `complete` — Tab Completion

Requests autocompletion suggestions for a partial input.

**Request:**

```json
{
  "action": "complete",
  "code": "pri"
}
```

**Response:**

```json
{
  "status": "ok",
  "completions": ["print", "pprint"]
}
```

The completions are based on:

- Built-in function names
- User-defined variables/functions in the current scope
- Keywords

---

### Kernel Internals (C++)

The kernel mode is implemented primarily in `json_and_kernel.hpp`. Here's how it works internally:

#### The `runKernel()` Function

This is the entry point when `scriptit --kernel` is invoked. Pseudocode:

```cpp
void runKernel() {
    Scope globalScope;          // persistent state — lives for the whole session
    int executionCount = 0;

    // Signal readiness
    std::cout << R"({"status":"kernel_ready","version":"2.0"})" << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        json request = json::parse(line);
        std::string action = request["action"];

        if (action == "execute") {
            executionCount++;
            std::string cellId = request["cell_id"];
            std::string code   = request["code"];

            // Capture stdout
            std::ostringstream capturedOut;
            std::streambuf* oldBuf = std::cout.rdbuf(capturedOut.rdbuf());

            std::string stderr_str = "";
            std::string result_str = "";
            std::string status = "ok";

            try {
                auto ast = parse(code);
                Value val = execute(ast, globalScope);  // globalScope is PERSISTENT
            } catch (ReturnException& e) {
                result_str = e.value.toString();        // top-level 'give'
            } catch (std::exception& e) {
                stderr_str = e.what();
                status = "error";
            }

            // Restore stdout
            std::cout.rdbuf(oldBuf);

            json response = {
                {"cell_id", cellId},
                {"status", status},
                {"stdout", capturedOut.str()},
                {"stderr", stderr_str},
                {"result", result_str},
                {"execution_count", executionCount}
            };
            std::cout << response.dump() << std::endl;

        } else if (action == "reset") {
            globalScope = Scope();      // fresh scope
            executionCount = 0;
            std::cout << R"({"status":"reset_ok"})" << std::endl;

        } else if (action == "shutdown") {
            std::cout << R"({"status":"shutdown_ok"})" << std::endl;
            break;                      // exit the loop → process ends

        } else if (action == "complete") {
            // ... gather completions from scope + builtins ...
            std::cout << json({{"status","ok"},{"completions", matches}}).dump() << std::endl;
        }
    }
}
```

### State Persistence Between Cells

The key design decision: **`globalScope` is a single object that persists across all `execute` calls.**

This is what makes the notebook feel interactive — you define a variable in Cell 1,
and it's available in Cell 5:

```
Cell 1:  var name = "Alice".        →  globalScope now has: {name: "Alice"}
Cell 2:  print("Hello, " + name).  →  stdout: "Hello, Alice\n"
Cell 3:  var age = 30.              →  globalScope now has: {name: "Alice", age: 30}
```

A `reset` action replaces `globalScope` with a fresh `Scope()`, clearing everything.
A `restart` (handled by the server) kills the entire process and starts a new one —
same effect but heavier.

### Stdout Capture

The kernel needs to separate "what the code printed" from "the protocol response."
It does this with a classic C++ stream redirect:

```
Before execution:
  std::cout → terminal (but actually → kernel's stdout pipe → server)

During execution:
  std::cout → ostringstream (captured)
  Any print() calls write to the ostringstream

After execution:
  std::cout → restored to original
  capturedOut.str() contains everything that was printed
  The JSON response (written to real stdout) includes capturedOut as the "stdout" field
```

This means `print()` inside user code does NOT interfere with the JSON protocol.
The protocol messages and the captured output never mix.

### Error Handling

Errors during execution are caught by a try/catch:

```
try {
    parse + execute the code
} catch (ReturnException) {
    // A top-level `give` statement — this is the cell's "return value"
    // Not an error — captured as `result`
} catch (std::exception) {
    // Parse error, runtime error, type error, etc.
    // Captured as `stderr`, status set to "error"
}
```

The kernel **never crashes** on bad user code. Every error is caught, packaged into the
JSON response, and sent back. The kernel remains alive and ready for the next command.

If the kernel crashes for a different reason (segfault, out of memory, etc.), the server
detects the dead process and auto-restarts it (see [Failure Modes](#failure-modes--recovery)).

---

## The Python Server

### Server Architecture

`notebook_server.py` is a single-file Python server built on the standard library's
`http.server` module. No Flask, no Django, no dependencies beyond Python 3.

```python
# Simplified structure of notebook_server.py

import http.server
import subprocess
import json

class NotebookHandler(http.server.BaseHTTPRequestHandler):
    kernel_process = None      # the subprocess
    notebook_state = {...}     # in-memory notebook

    def do_GET(self):
        if self.path == '/':
            # Serve index.html
        elif self.path == '/api/notebook':
            # Return notebook state as JSON
        elif self.path == '/api/kernel/status':
            # Check if kernel is alive
        elif self.path.startswith('/api/browse'):
            # Directory listing
        else:
            # Serve static files

    def do_POST(self):
        if self.path == '/api/execute':
            # Forward code to kernel, return result
        elif self.path == '/api/kernel/restart':
            # Kill kernel, start new one
        elif self.path == '/api/kernel/reset':
            # Send reset command to kernel
        elif self.path == '/api/notebook/save':
            # Write notebook to .nsit file
        elif self.path == '/api/notebook/load':
            # Read .nsit file into memory
        # ... etc

def main():
    start_kernel()
    server = http.server.HTTPServer(('', 8888), NotebookHandler)
    server.serve_forever()
```

### API Endpoints Reference

All endpoints return JSON. POST endpoints accept JSON bodies.

---

#### `GET /` — Serve the GUI

Returns `index.html`. This is the notebook interface.

---

#### `GET /api/notebook` — Get Notebook State

Returns the full in-memory notebook object.

**Response:**

```json
{
  "metadata": {"title": "Untitled Notebook", "created": "...", "modified": "..."},
  "cells": [...]
}
```

---

#### `POST /api/notebook/new` — Create New Notebook

Replaces the in-memory notebook with a fresh one (one empty code cell).

**Request body:** (none required)

**Response:**

```json
{
  "status": "ok",
  "notebook": { ... }
}
```

---

#### `POST /api/notebook/save` — Save Notebook

**Request body:**

```json
{
  "path": "/home/user/my_notebook.nsit",
  "notebook": { ... }
}
```

Writes the notebook JSON to the specified path on the server's filesystem.

**Response:**

```json
{
  "status": "ok",
  "path": "/home/user/my_notebook.nsit"
}
```

---

#### `POST /api/notebook/load` — Load Notebook

**Request body:**

```json
{
  "path": "/home/user/my_notebook.nsit"
}
```

Reads the file, parses the JSON, replaces the in-memory notebook.

**Response:**

```json
{
  "status": "ok",
  "notebook": { ... }
}
```

---

#### `POST /api/execute` — Execute Code

This is the core endpoint. It forwards code to the kernel and returns the result.

**Request body:**

```json
{
  "cell_id": "c1abc12345",
  "code": "var x = 10.\nprint(x)."
}
```

**What the server does internally:**

1. Write `{"action":"execute","cell_id":"c1abc12345","code":"var x = 10.\nprint(x)."}\n` to kernel stdin
2. Read one line from kernel stdout
3. Parse the JSON response
4. Return it to the browser

**Response (proxied from kernel):**

```json
{
  "cell_id": "c1abc12345",
  "status": "ok",
  "stdout": "10\n",
  "stderr": "",
  "result": "",
  "execution_count": 1
}
```

---

#### `GET /api/kernel/status` — Kernel Status

**Response:**

```json
{
  "alive": true,
  "pid": 12345
}
```

Checks if the kernel subprocess is still running via `process.poll()`.

---

#### `POST /api/kernel/restart` — Restart Kernel

1. Sends `{"action":"shutdown"}` to the kernel
2. Waits for process to exit (with timeout)
3. If it doesn't exit, kills it (`SIGKILL`)
4. Starts a new kernel subprocess
5. Waits for `{"status":"kernel_ready"}` from the new kernel

**Response:**

```json
{
  "status": "ok"
}
```

---

#### `POST /api/kernel/reset` — Reset Kernel State

Sends `{"action":"reset"}` to the kernel. Clears all variables without restarting the process.

**Response:**

```json
{
  "status": "ok"
}
```

---

#### `POST /api/cell/add` — Add a Cell

**Request body:**

```json
{
  "after_id": "c1abc12345",
  "type": "code"
}
```

Inserts a new cell after the cell with `after_id`. If `after_id` is null, appends at the end.

**Response:**

```json
{
  "status": "ok",
  "cell": { "id": "c3xyz99999", "type": "code", "source": "", ... }
}
```

---

#### `POST /api/cell/delete` — Delete a Cell

**Request body:**

```json
{
  "cell_id": "c1abc12345"
}
```

---

#### `POST /api/cell/move` — Move a Cell

**Request body:**

```json
{
  "cell_id": "c1abc12345",
  "direction": "up"
}
```

`direction` is `"up"` or `"down"`. Swaps the cell with its neighbor.

---

#### `POST /api/cell/type` — Toggle Cell Type

**Request body:**

```json
{
  "cell_id": "c1abc12345",
  "type": "markdown"
}
```

Changes the cell between `"code"` and `"markdown"`.

---

#### `GET /api/browse?path=/some/directory` — Browse Directories

Returns a listing of files and directories at the given path. Used by the Save-As
and Load modals to let the user navigate the server's filesystem.

**Response:**

```json
{
  "path": "/home/user",
  "entries": [
    { "name": "Documents", "type": "directory" },
    { "name": "my_notebook.nsit", "type": "file" },
    { "name": "data.csv", "type": "file" }
  ]
}
```

---

### Kernel Lifecycle Management

The server is the sole manager of the kernel process. Here's the full lifecycle:

```
Server startup
     │
     ▼
  start_kernel()
     │
     ├─── subprocess.Popen(["./scriptit", "--kernel"], stdin=PIPE, stdout=PIPE, stderr=PIPE)
     │
     ├─── Read first line from stdout
     │
     ├─── Verify: {"status":"kernel_ready","version":"2.0"}
     │
     ▼
  Kernel is IDLE ◄────────────────────────────────┐
     │                                             │
     │  (execute request comes in)                 │
     ▼                                             │
  Kernel is BUSY                                   │
     │                                             │
     ├── Write JSON to kernel stdin                │
     ├── Read JSON from kernel stdout              │
     ├── Return result to browser                  │
     │                                             │
     └─────────────────────────────────────────────┘

     │  (restart requested)
     ▼
  shutdown_kernel()
     │
     ├── Send {"action":"shutdown"}
     ├── Wait for process exit (timeout: 5s)
     ├── SIGKILL if it didn't exit
     ▼
  start_kernel()  ──→  back to IDLE
```

**Auto-restart on crash:**

```python
def execute_code(cell_id, code):
    if kernel_process.poll() is not None:
        # Kernel is dead! Restart it.
        start_kernel()

    # Now send the execute command
    write_to_kernel({"action": "execute", "cell_id": cell_id, "code": code})
    response = read_from_kernel()
    return response
```

### Request/Response Flow

Here's a complete trace of what happens when the user clicks "Run" on a cell:

```
 1. User clicks ▶ Run on Cell 3
 2. GUI reads textarea content: "var x = 10.\nprint(x)."
 3. GUI sets kernel status indicator to BUSY (yellow)
 4. GUI sends:
       POST /api/execute
       {"cell_id": "c3xyz99999", "code": "var x = 10.\nprint(x)."}

 5. Server receives HTTP request
 6. Server writes to kernel stdin:
       {"action":"execute","cell_id":"c3xyz99999","code":"var x = 10.\nprint(x)."}\n
 7. Server blocks, reading kernel stdout...

 8. Kernel parses the JSON
 9. Kernel redirects stdout to capture buffer
10. Kernel parses and executes "var x = 10.\nprint(x)."
       → x is stored in globalScope
       → print(x) writes "10\n" to the capture buffer
11. Kernel restores stdout
12. Kernel writes to stdout:
       {"cell_id":"c3xyz99999","status":"ok","stdout":"10\n","stderr":"","result":"","execution_count":3}\n

13. Server reads the response line
14. Server sends HTTP response to browser

15. GUI receives response
16. GUI updates Cell 3:
       → Shows "10" in the output area
       → Shows "[3]" as the execution count
       → Shows execution time (e.g., "0.002s")
17. GUI sets kernel status to IDLE (green)
```

### File Serving

The server doubles as a static file server for the GUI:

- `GET /` → serves `notebook/index.html`
- `GET /style.css` → serves `notebook/style.css` (if exists)
- `GET /favicon.ico` → serves favicon (if exists)

The server determines `Content-Type` from file extensions:

- `.html` → `text/html`
- `.css` → `text/css`
- `.js` → `application/javascript`
- `.json` → `application/json`
- `.png` → `image/png`

### Directory Browsing

The `/api/browse` endpoint enables the Save/Load modals to navigate the server's
filesystem without the user needing to type full paths:

```python
def handle_browse(path):
    entries = []
    for item in os.listdir(path):
        full = os.path.join(path, item)
        entries.append({
            "name": item,
            "type": "directory" if os.path.isdir(full) else "file"
        })
    # Sort: directories first, then files, alphabetically
    entries.sort(key=lambda e: (e["type"] != "directory", e["name"].lower()))
    return {"path": path, "entries": entries}
```

---

## The File Format (.nsit)

Notebooks are persisted as JSON files with the `.nsit` extension
(**N**otebook **S**cript**It**).

### Schema

```
.nsit file
├── metadata
│   ├── title       (string)    — Display title of the notebook
│   ├── created     (ISO 8601)  — Creation timestamp
│   └── modified    (ISO 8601)  — Last modification timestamp
│
└── cells           (array)
    └── cell
        ├── id              (string)       — Unique cell identifier
        ├── type            (string)       — "code" or "markdown"
        ├── source          (string)       — The cell's content (code or markdown text)
        ├── outputs         (array)        — Execution outputs (code cells only)
        │   └── output
        │       ├── type    (string)       — "stdout", "stderr", or "result"
        │       └── text    (string)       — The output text
        ├── execution_count (int | null)   — Execution number, null for unrun/markdown cells
        └── metadata        (object)       — Reserved for future use
```

### Cell Structure

**Code cell (executed):**

```json
{
  "id": "c1abc12345",
  "type": "code",
  "source": "var x = 10.\nprint(x).",
  "outputs": [{ "type": "stdout", "text": "10\n" }],
  "execution_count": 1,
  "metadata": {}
}
```

**Code cell (with error):**

```json
{
  "id": "c2def67890",
  "type": "code",
  "source": "print(y).",
  "outputs": [
    { "type": "stderr", "text": "NameError: variable 'y' is not defined" }
  ],
  "execution_count": 2,
  "metadata": {}
}
```

**Code cell (with result from `give`):**

```json
{
  "id": "c3ghi13579",
  "type": "code",
  "source": "give 42.",
  "outputs": [{ "type": "result", "text": "42" }],
  "execution_count": 3,
  "metadata": {}
}
```

**Markdown cell:**

```json
{
  "id": "c4jkl24680",
  "type": "markdown",
  "source": "# Data Analysis\nThis notebook explores the dataset.",
  "outputs": [],
  "execution_count": null,
  "metadata": {}
}
```

### Example Notebook File

A complete `.nsit` file:

```json
{
  "metadata": {
    "title": "Getting Started with ScriptIt",
    "created": "2025-06-15T10:30:00.000Z",
    "modified": "2025-06-15T11:45:00.000Z"
  },
  "cells": [
    {
      "id": "c1a0b1c2d3",
      "type": "markdown",
      "source": "# Getting Started\nLet's learn ScriptIt basics.",
      "outputs": [],
      "execution_count": null,
      "metadata": {}
    },
    {
      "id": "c2e4f5a6b7",
      "type": "code",
      "source": "var greeting = \"Hello, World!\".\nprint(greeting).",
      "outputs": [{ "type": "stdout", "text": "Hello, World!\n" }],
      "execution_count": 1,
      "metadata": {}
    },
    {
      "id": "c3c8d9e0f1",
      "type": "code",
      "source": "var nums = [1, 2, 3, 4, 5].\nfor n in nums {\n    print(n * n).\n}.",
      "outputs": [{ "type": "stdout", "text": "1\n4\n9\n16\n25\n" }],
      "execution_count": 2,
      "metadata": {}
    },
    {
      "id": "c4a2b3c4d5",
      "type": "markdown",
      "source": "## Observations\nSquares grow quickly!",
      "outputs": [],
      "execution_count": null,
      "metadata": {}
    }
  ]
}
```

### Cell ID Generation

Cell IDs are generated in the GUI using a simple pattern:

```javascript
function generateId() {
  return (
    "c" + Date.now().toString(36) + Math.random().toString(36).substr(2, 5)
  );
}
// Example: "c1m5x7a2k9b3q"
```

The prefix `c` makes them recognizable. The combination of timestamp + random
ensures uniqueness even when adding cells rapidly.

---

## The GUI

The entire notebook interface is a single `index.html` file — HTML, CSS, and JavaScript
all in one. No build step, no bundler, no framework. It runs directly in the browser.

### State Management

All state lives in plain JavaScript variables at the module level:

```javascript
let notebook = {
  // The full notebook object (mirrors .nsit format)
  metadata: { title: "Untitled Notebook", created: null, modified: null },
  cells: [],
};

let activeCellId = null; // ID of the currently focused/selected cell
let editingMarkdownId = null; // ID of the markdown cell currently in edit mode
let savedFilePath = null; // Server-side file path (null if never saved)
let isRunning = false; // True while "Run All" is executing
let kernelStatus = "idle"; // 'idle' | 'busy' | 'dead'
let executionQueue = []; // Queue for Run All (list of cell IDs to execute)
```

There is no state management library. Functions directly mutate these variables and
then call `renderNotebook()` to update the DOM.

### Cell Types & Rendering

#### Code Cells

A code cell renders as:

```
┌────────────────────────────────────────────────────┐
│ [3] │ ▶ │  var x = [1, 2, 3].                     │  ← textarea (editable)
│     │   │  for item in x {                         │
│     │   │      print(item).                        │
│     │   │  }.                                      │
├─────┴───┴──────────────────────────────────────────┤
│ 1                                                  │  ← output area
│ 2                                                  │
│ 3                                                  │
│                                      0.003s        │  ← execution time
└────────────────────────────────────────────────────┘
```

Components:

- **Execution count badge** `[3]` — shows the execution order number
- **Run button** `▶` — in the cell gutter; shows a spinning animation while executing
- **Textarea** — the code editor (monospace font, auto-expanding height)
- **Output area** — shows stdout (white), stderr (red), result (blue)
- **Execution time** — displayed after each run

#### Markdown Cells

**View mode** (default):

```
┌────────────────────────────────────────────────────┐
│     │ ✏️ │  Data Analysis                           │  ← rendered HTML
│     │    │  This notebook explores the dataset.     │
│     │    │  • Point one                             │
│     │    │  • Point two                             │
└────────────────────────────────────────────────────┘
```

**Edit mode** (double-click or click ✏️):

```
┌────────────────────────────────────────────────────┐
│     │ ✓  │  # Data Analysis                        │  ← textarea (editable)
│     │    │  This notebook explores the dataset.     │
│     │    │  - Point one                             │
│     │    │  - Point two                             │
└────────────────────────────────────────────────────┘
```

Double-click the rendered markdown → enter edit mode (shows textarea with raw markdown).
Press Escape or click the checkmark → exit edit mode (renders markdown to HTML).

#### Cell Action Buttons (on hover)

When hovering over a cell, action buttons appear:

```
  [Clear Output]  [M↓ / { }]  [▲ Move Up]  [▼ Move Down]  [✕ Delete]
```

- **Clear Output** — removes the output area (code cells only)
- **M↓** / **{ }** — toggle cell type: code→markdown (M↓) or markdown→code ({ })
- **▲ / ▼** — reorder cells
- **✕** — delete the cell (with confirmation if it has content)

#### Add Cell Dividers

Between every pair of cells (and after the last cell), a thin divider appears on hover:

```
  ── + Code ── + Markdown ──
```

Clicking `+ Code` inserts a new code cell at that position.
Clicking `+ Markdown` inserts a new markdown cell.

### Toolbar

The toolbar sits at the top of the page:

```
┌──────────────────────────────────────────────────────────────────┐
│  📒 [My Notebook Title    ]   ● idle                             │
│                                                                  │
│  [▶▶ Run All] [↻ Restart] [⟲ Reset] [🗑 Clear] [💾 Save] [📂 Open] [📄 New]  │
└──────────────────────────────────────────────────────────────────┘
```

| Button      | Action                                                            |
| ----------- | ----------------------------------------------------------------- |
| **Run All** | Executes all code cells top-to-bottom sequentially                |
| **Restart** | Kills and restarts the kernel (POST `/api/kernel/restart`)        |
| **Reset**   | Clears kernel state without restarting (POST `/api/kernel/reset`) |
| **Clear**   | Clears all cell outputs in the GUI (does not touch kernel state)  |
| **Save**    | Saves notebook (to existing path, or opens Save-As)               |
| **Open**    | Opens the Load modal                                              |
| **New**     | Creates a fresh notebook (prompts to save if unsaved changes)     |

**Kernel status indicator:**

- 🟢 Green dot + "idle" — kernel is ready
- 🟡 Yellow dot + "busy" — code is executing
- 🔴 Red dot + "dead" — kernel process died (click Restart to fix)

**Notebook title:**
The title is an editable text field. Changes update `notebook.metadata.title` directly.

### Keyboard Shortcuts

The GUI has two modes: **Edit mode** (cursor is inside a cell's textarea) and
**Command mode** (no textarea focused).

#### Edit Mode Shortcuts

| Shortcut           | Action                                                                         |
| ------------------ | ------------------------------------------------------------------------------ |
| `Shift+Enter`      | Run cell, move focus to the next cell (create one if at end)                   |
| `Ctrl+Enter`       | Run cell, keep focus on current cell                                           |
| `Ctrl+S`           | Save notebook                                                                  |
| `Ctrl+Shift+Enter` | Run All cells                                                                  |
| `Escape`           | Exit edit mode (enter command mode). For markdown cells, also finishes editing |
| `Tab`              | Insert 4 spaces at cursor position (does not move to next element)             |
| `Shift+Tab`        | Remove up to 4 leading spaces from current line (dedent)                       |

#### Command Mode Shortcuts

| Shortcut         | Action                                           |
| ---------------- | ------------------------------------------------ |
| `b`              | Add new code cell below the active cell          |
| `a`              | Add new code cell above the active cell          |
| `dd`             | Delete the active cell (press `d` twice quickly) |
| `m`              | Convert active cell to markdown type             |
| `y`              | Convert active cell to code type                 |
| `Enter`          | Enter edit mode on the active cell               |
| `↑` (Arrow Up)   | Move focus to the cell above                     |
| `↓` (Arrow Down) | Move focus to the cell below                     |

The `dd` shortcut uses a timer: pressing `d` once starts a 500ms window; pressing `d` again
within that window triggers deletion. If the window expires, it resets.

### Markdown Renderer

The GUI includes a simple built-in markdown renderer. It does **not** use a library —
it's a custom function that converts markdown text to HTML using regex replacements.

**Supported syntax:**

| Markdown               | Rendered                                |
| ---------------------- | --------------------------------------- |
| `# Heading 1`          | `<h1>Heading 1</h1>`                    |
| `## Heading 2`         | `<h2>Heading 2</h2>`                    |
| `### Heading 3`        | `<h3>Heading 3</h3>`                    |
| `**bold**`             | `<strong>bold</strong>`                 |
| `*italic*`             | `<em>italic</em>`                       |
| `***bold italic***`    | `<strong><em>bold italic</em></strong>` |
| `` `inline code` ``    | `<code>inline code</code>`              |
| `> blockquote`         | `<blockquote>blockquote</blockquote>`   |
| `- list item`          | `<ul><li>list item</li></ul>`           |
| `---`                  | `<hr>`                                  |
| `[text](url)`          | `<a href="url">text</a>`                |
| ` ``` code block ``` ` | `<pre><code>code block</code></pre>`    |

**Rendering approach (simplified):**

````javascript
function renderMarkdown(src) {
  let html = src;

  // Code blocks first (to protect their content from other replacements)
  html = html.replace(/```([\s\S]*?)```/g, "<pre><code>$1</code></pre>");

  // Inline code (before bold/italic to protect backtick content)
  html = html.replace(/`([^`]+)`/g, "<code>$1</code>");

  // Headings
  html = html.replace(/^### (.+)$/gm, "<h3>$1</h3>");
  html = html.replace(/^## (.+)$/gm, "<h2>$1</h2>");
  html = html.replace(/^# (.+)$/gm, "<h1>$1</h1>");

  // Bold italic, then bold, then italic
  html = html.replace(/\*\*\*(.+?)\*\*\*/g, "<strong><em>$1</em></strong>");
  html = html.replace(/\*\*(.+?)\*\*/g, "<strong>$1</strong>");
  html = html.replace(/\*(.+?)\*/g, "<em>$1</em>");

  // Blockquotes
  html = html.replace(/^> (.+)$/gm, "<blockquote>$1</blockquote>");

  // List items
  html = html.replace(/^- (.+)$/gm, "<li>$1</li>");
  html = html.replace(/(<li>.*<\/li>)/gs, "<ul>$1</ul>");

  // Horizontal rule
  html = html.replace(/^---$/gm, "<hr>");

  // Links
  html = html.replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>');

  // Line breaks
  html = html.replace(/\n/g, "<br>");

  return html;
}
````

### Save/Load System

#### Save Flow

```
User clicks Save
       │
       ├── savedFilePath exists?
       │       │
       │    YES ──→ POST /api/notebook/save with path + notebook
       │              → Toast: "Saved ✓"
       │
       │    NO ──→ Open Save-As Modal
       │              │
       │              ├── Browse server directories (/api/browse)
       │              ├── User navigates to desired folder
       │              ├── User enters filename
       │              ├── Click "Save"
       │              │     → POST /api/notebook/save
       │              │     → Update savedFilePath
       │              │     → Toast: "Saved ✓"
       │              │
       │              └── OR click "Download"
       │                    → Create Blob from JSON
       │                    → Trigger browser download
       │                    → File saved to user's Downloads
       │
       └── Ctrl+S triggers this same flow
```

#### Load Flow

```
User clicks Open
       │
       └── Open Load Modal
              │
              ├── Browse server directories (/api/browse)
              │     → User navigates and clicks a .nsit file
              │     → POST /api/notebook/load with path
              │     → Notebook replaced in memory
              │     → All cells re-rendered
              │
              └── OR click "Upload"
                    → <input type="file"> opens
                    → User selects .nsit file from their computer
                    → FileReader reads the file
                    → JSON.parse the content
                    → Replace notebook in memory
                    → All cells re-rendered
```

#### Save-As / Load Modal UI

Both modals share a similar layout:

```
┌─────────────────────────────────────────────────┐
│  Save Notebook As                          [✕]  │
│─────────────────────────────────────────────────│
│                                                 │
│  Path: /home/user/notebooks/                    │
│  ┌─────────────────────────────────────────┐    │
│  │ 📁 ..                                   │    │
│  │ 📁 projects                             │    │
│  │ 📁 tutorials                            │    │
│  │ 📄 analysis.nsit                        │    │
│  │ 📄 demo.nsit                            │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  Filename: [my_notebook.nsit          ]         │
│                                                 │
│  [Save to Server]    [Download as File]         │
│                                                 │
└─────────────────────────────────────────────────┘
```

- Clicking a directory navigates into it (calls `/api/browse?path=...`)
- Clicking `..` goes up one level
- Clicking a `.nsit` file selects it (for Load) or fills in the filename (for Save)
- The filename input lets the user type a custom name
- `.nsit` extension is auto-appended if missing

### UI Components

#### Toast Notifications

Temporary messages that appear at the top of the screen and fade after ~3 seconds:

```javascript
function showToast(message, type = "info") {
  // type: 'success' (green), 'error' (red), 'info' (blue)
  const toast = document.createElement("div");
  toast.className = `toast toast-${type}`;
  toast.textContent = message;
  document.body.appendChild(toast);
  setTimeout(() => toast.remove(), 3000);
}
```

Used for: "Saved ✓", "Kernel restarted", "Error: kernel is dead", etc.

#### Auto-expanding Textareas

Code cell textareas grow vertically to fit their content:

```javascript
function autoResize(textarea) {
  textarea.style.height = "auto";
  textarea.style.height = textarea.scrollHeight + "px";
}
```

Called on `input` events and after cell content loads.

#### Run All Execution

"Run All" executes code cells sequentially, top to bottom:

```javascript
async function runAll() {
  isRunning = true;
  const codeCells = notebook.cells.filter((c) => c.type === "code");
  for (const cell of codeCells) {
    await executeCell(cell.id);
    if (kernelStatus === "dead") break; // stop if kernel died
  }
  isRunning = false;
}
```

Each cell waits for the previous one to finish. The GUI updates after each cell,
so the user sees results appearing one by one.

---

## End-to-End Walkthrough

Here's what happens from server startup to running your first cell:

### 1. Starting the Server

```bash
$ python3 notebook/notebook_server.py
```

The server:

1. Starts on port 8888
2. Locates the `scriptit` binary
3. Spawns `./scriptit --kernel` as a subprocess
4. Reads `{"status":"kernel_ready","version":"2.0"}` from the kernel
5. Prints: `Notebook server running at http://localhost:8888`
6. Begins listening for HTTP requests

### 2. Opening the Notebook

User opens `http://localhost:8888` in a browser.

1. Browser sends `GET /`
2. Server responds with `index.html`
3. Browser renders the page
4. JavaScript initializes: creates an empty notebook with one code cell
5. GUI calls `GET /api/kernel/status` to verify the kernel is alive
6. Status indicator turns green: "idle"

### 3. Writing and Running Code

User types in Cell 1: `var x = 42.\nprint("The answer is " + str(x)).`

User presses Shift+Enter:

1. GUI calls `executeCell("c1...")`:
   - Sets status to "busy"
   - Shows spinner on run button
   - Sends POST `/api/execute` with cell_id and code

2. Server receives request:
   - Writes to kernel stdin: `{"action":"execute","cell_id":"c1...","code":"var x = 42.\nprint(\"The answer is \" + str(x))."}\n`
   - Blocks reading kernel stdout

3. Kernel executes the code:
   - Redirects stdout to capture buffer
   - Parses and runs the code
   - `x = 42` stored in globalScope
   - `print(...)` writes `"The answer is 42\n"` to capture buffer
   - Restores stdout
   - Writes response: `{"cell_id":"c1...","status":"ok","stdout":"The answer is 42\n",...}\n`

4. Server relays response to browser

5. GUI updates:
   - Cell output area shows: `The answer is 42`
   - Execution count shows: `[1]`
   - Execution time shows: `0.004s`
   - Status returns to "idle" (green)
   - Focus moves to Cell 2 (created automatically)

### 4. Cell State Persistence

User types in Cell 2: `print(x + 8).`

Executes → output: `50`

This works because `x` was defined in Cell 1 and persists in `globalScope`.

### 5. Saving

User presses Ctrl+S → Save-As modal opens (first save).
User browses to `/home/user/notebooks/`, types `tutorial.nsit`, clicks Save.

Server writes the `.nsit` JSON file to disk.

---

## Failure Modes & Recovery

### Kernel Crashes During Execution

```
Scenario: Code triggers a segfault in the kernel
  1. Server is waiting for stdout from kernel
  2. Kernel process exits unexpectedly
  3. Server's read() returns EOF or raises an exception
  4. Server detects: kernel_process.poll() is not None
  5. Server returns error response: {"status":"error","stderr":"Kernel crashed"}
  6. Server auto-restarts the kernel
  7. GUI shows error in cell output + toast notification
  8. Kernel status briefly shows red, then green after restart

  NOTE: All variable state is lost. User must re-run cells.
```

### Kernel Hangs (Infinite Loop)

```
Scenario: User code has an infinite loop
  1. Server is waiting for kernel stdout (blocks indefinitely)
  2. User clicks "Restart" in the toolbar
  3. Server kills the kernel process (SIGKILL)
  4. Starts a new kernel
  5. GUI shows: "Kernel restarted" toast

  Future improvement: execution timeout.
```

### Server Dies

```
Scenario: Server process crashes or is killed
  1. Browser's fetch() calls fail (network error)
  2. GUI shows: "Connection lost" error
  3. Kernel process also dies (it's a child of the server)
  4. User must restart the server manually
  5. If notebook was saved, user reloads the .nsit file
```

### Browser Refresh

```
Scenario: User refreshes the page
  1. All in-browser state is lost
  2. GUI reinitializes with an empty notebook
  3. Kernel is still running (managed by server)
  4. User can load their saved .nsit file to continue
  5. Variables in kernel are still intact from before the refresh

  NOTE: Unsaved cell changes are lost!
```

---

## Design Decisions & Rationale

### Why JSON over stdio (not WebSocket, not TCP)?

- **Simplicity**: No networking code in the kernel. It just reads stdin and writes stdout.
- **Universality**: Every language can read stdin/write stdout. No library dependencies.
- **Debugging**: You can test the kernel manually by piping JSON into it:
  ```bash
  echo '{"action":"execute","cell_id":"test","code":"print(1+2)."}' | ./scriptit --kernel
  ```
- **Process isolation**: The kernel is a clean subprocess. Crash? Just restart it.

### Why a Python server (not C++ or Node)?

- **Rapid development**: Python's `http.server` is batteries-included.
- **Subprocess management**: Python's `subprocess` module is battle-tested.
- **No compilation**: Change the server, restart, done. No build step.
- **The heavy lifting is in C++**: The kernel does the actual computation. The server
  is just plumbing — HTTP ↔ stdio translation.

### Why a single index.html (no framework)?

- **Zero dependencies**: No npm, no webpack, no React. Just open the file.
- **Deployment simplicity**: Copy one file. Serve it. Done.
- **Full control**: Every pixel is intentional. No framework fighting.
- **Educational**: Anyone can read the source and understand the full system.

### Why .nsit and not .ipynb?

- **Clean break**: ScriptIt is not Python. Using `.ipynb` would be confusing.
- **Simpler format**: `.nsit` is a strict subset of what `.ipynb` offers. No kernel specs,
  no MIME bundles, no widget state. Just cells and metadata.
- **The name**: **N**otebook **S**cript**It** → `.nsit`

### Why a persistent globalScope (not fresh scope per cell)?

- **Interactive feel**: Notebooks are exploratory. You build up state incrementally.
- **Matches user expectations**: Jupyter works this way. It's what people expect.
- **Reset is available**: If you want a clean slate, use Reset.

---

## Quick Reference Card

```
┌─────────────────────── KERNEL PROTOCOL ───────────────────────┐
│                                                                │
│  Start:    ./scriptit --kernel                                 │
│  Ready:    {"status":"kernel_ready","version":"2.0"}           │
│                                                                │
│  Execute:  {"action":"execute","cell_id":"..","code":".."}     │
│  Reset:    {"action":"reset"}                                  │
│  Shutdown: {"action":"shutdown"}                               │
│  Complete: {"action":"complete","code":".."}                   │
│                                                                │
├─────────────────────── SERVER API ────────────────────────────│
│                                                                │
│  GET  /                    Serve GUI                           │
│  GET  /api/notebook        Get notebook state                  │
│  POST /api/notebook/new    New notebook                        │
│  POST /api/notebook/save   Save notebook                       │
│  POST /api/notebook/load   Load notebook                       │
│  POST /api/execute         Run code in cell                    │
│  GET  /api/kernel/status   Kernel alive?                       │
│  POST /api/kernel/restart  Kill + restart kernel               │
│  POST /api/kernel/reset    Clear kernel state                  │
│  POST /api/cell/add        Add cell                            │
│  POST /api/cell/delete     Delete cell                         │
│  POST /api/cell/move       Move cell                           │
│  POST /api/cell/type       Toggle cell type                    │
│  GET  /api/browse          Directory listing                   │
│                                                                │
├─────────────────────── KEYBOARD ──────────────────────────────│
│                                                                │
│  Shift+Enter       Run + next cell                             │
│  Ctrl+Enter        Run + stay                                  │
│  Ctrl+S            Save                                        │
│  Ctrl+Shift+Enter  Run All                                     │
│  Escape            Command mode / close modal                  │
│  Tab / Shift+Tab   Indent / dedent                             │
│  b / a             New cell below / above (command mode)       │
│  dd                Delete cell (command mode)                  │
│  m / y             To markdown / to code (command mode)        │
│  ↑ / ↓             Navigate cells (command mode)               │
│  Enter             Edit mode (command mode)                    │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

---

_This document describes the ScriptIt Notebook system as of version 2.0._
_With this document and the architecture it describes, you could rebuild the entire system from scratch._
