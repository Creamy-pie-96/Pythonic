#!/usr/bin/env python3
"""
ScriptIt Color Customizer — Backend Server
Serves the color customizer HTML and handles save requests.

Two save modes (auto-detected):
  DEV MODE  — extension.ts found nearby → write rules there + rebuild
  USER MODE — system install            → write to VS Code settings.json
"""

import os
import sys
import re
import json
import subprocess
import platform
import shutil
from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import urlparse
import webbrowser
import threading

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# ── Find extension.ts (only works when running from the dev tree) ──
def _find_extension_ts():
    """Walk upward from SCRIPT_DIR looking for src/client/extension.ts."""
    d = os.path.dirname(SCRIPT_DIR)          # try parent first (scriptit-vscode/)
    for _ in range(4):                         # at most 4 levels up
        candidate = os.path.join(d, "src", "client", "extension.ts")
        if os.path.exists(candidate):
            return d, candidate
        d = os.path.dirname(d)
    return None, None

EXT_DIR, EXTENSION_TS = _find_extension_ts()
INSTALL_PY = os.path.join(EXT_DIR, "install.py") if EXT_DIR else None
DEV_MODE   = EXTENSION_TS is not None

# ── VS Code user settings.json (cross-platform) ──
def _find_vscode_settings():
    home = os.path.expanduser("~")
    system = platform.system()
    if system == "Linux":
        return os.path.join(home, ".config", "Code", "User", "settings.json")
    elif system == "Darwin":
        return os.path.join(home, "Library", "Application Support", "Code", "User", "settings.json")
    elif system == "Windows":
        appdata = os.environ.get("APPDATA", os.path.join(home, "AppData", "Roaming"))
        return os.path.join(appdata, "Code", "User", "settings.json")
    return None

VSCODE_SETTINGS = _find_vscode_settings()

PORT = 7890


class CustomizerHandler(SimpleHTTPRequestHandler):
    """Serves customize.html and handles /api/save, /api/save-install, /api/status."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=SCRIPT_DIR, **kwargs)

    def log_message(self, format, *args):
        pass  # suppress noisy logs

    # ── GET ──────────────────────────────────────────────────────────
    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path in ("/", "/index.html", "/customize.html"):
            html = os.path.join(SCRIPT_DIR, "customize.html")
            if os.path.exists(html):
                with open(html, "r") as f:
                    content = f.read().encode("utf-8")
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(content)))
                self.end_headers()
                self.wfile.write(content)
            else:
                self.send_error(404, "customize.html not found")
        elif parsed.path == "/api/status":
            self._send_json({
                "dev_mode": DEV_MODE,
                "ext_dir": EXT_DIR or "",
                "settings_path": VSCODE_SETTINGS or "",
            })
        else:
            super().do_GET()

    # ── OPTIONS (CORS) ──────────────────────────────────────────────
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    # ── POST ─────────────────────────────────────────────────────────
    def do_POST(self):
        path = urlparse(self.path).path
        if path == "/api/save":
            self._handle_save(install=False)
        elif path == "/api/save-install":
            self._handle_save(install=True)
        else:
            self.send_error(404, "Not found")

    # ── Save logic ──────────────────────────────────────────────────
    def _handle_save(self, install=False):
        try:
            length = int(self.headers.get("Content-Length", 0))
            body   = self.rfile.read(length).decode("utf-8")
            data   = json.loads(body)
            rules  = data.get("rules", [])

            if not rules:
                self._send_json({"status": "error", "message": "No rules provided"})
                return

            if DEV_MODE:
                result = self._save_to_extension_ts(rules, install)
            else:
                result = self._save_to_vscode_settings(rules)

            self._send_json(result)

        except Exception as e:
            self._send_json({"status": "error", "message": str(e)})

    # ── DEV MODE: patch extension.ts ─────────────────────────────────
    def _save_to_extension_ts(self, rules, install):
        with open(EXTENSION_TS, "r") as f:
            content = f.read()

        new_block = self._build_ts_rules_block(rules)
        pattern   = r"const SCRIPTIT_TOKEN_RULES = \[.*?\];"
        match     = re.search(pattern, content, re.DOTALL)
        if not match:
            return {"status": "error",
                    "message": "Could not find SCRIPTIT_TOKEN_RULES in extension.ts"}

        new_content = content[:match.start()] + new_block + content[match.end():]
        with open(EXTENSION_TS, "w") as f:
            f.write(new_content)

        msg = "Colors saved to extension.ts!"

        if install and INSTALL_PY and os.path.exists(INSTALL_PY):
            msg += " Rebuilding extension..."
            try:
                r = subprocess.run(
                    [sys.executable, INSTALL_PY, "--ext-only"],
                    capture_output=True, text=True, timeout=120,
                    cwd=EXT_DIR
                )
                if r.returncode == 0:
                    msg = ("Colors saved, extension rebuilt & installed! "
                           "Restart VS Code to see changes.")
                else:
                    msg += f"\n{r.stderr or r.stdout}"
            except subprocess.TimeoutExpired:
                msg += " Build timed out."
            except Exception as e:
                msg += f" Build error: {e}"

        return {"status": "ok", "message": msg}

    # ── USER MODE: patch VS Code settings.json ───────────────────────
    def _save_to_vscode_settings(self, rules):
        if not VSCODE_SETTINGS:
            return {"status": "error",
                    "message": "Could not locate VS Code settings.json on this OS."}

        # Build textMateRules list
        tm_rules = []
        for r in rules:
            entry = {"scope": r["scope"],
                     "settings": {"foreground": r["foreground"]}}
            if r.get("fontStyle"):
                entry["settings"]["fontStyle"] = r["fontStyle"]
            tm_rules.append(entry)

        # Read existing settings (or start fresh)
        settings = {}
        if os.path.exists(VSCODE_SETTINGS):
            with open(VSCODE_SETTINGS, "r") as f:
                try:
                    settings = json.load(f)
                except json.JSONDecodeError:
                    settings = {}

        token_colors = settings.get("editor.tokenColorCustomizations", {})
        if not isinstance(token_colors, dict):
            token_colors = {}

        # Keep any non-ScriptIt rules the user already has
        existing = token_colors.get("textMateRules", [])
        kept = [r for r in existing
                if not (isinstance(r.get("scope"), str)
                        and r["scope"].endswith(".scriptit"))]

        token_colors["textMateRules"] = kept + tm_rules
        settings["editor.tokenColorCustomizations"] = token_colors

        os.makedirs(os.path.dirname(VSCODE_SETTINGS), exist_ok=True)
        with open(VSCODE_SETTINGS, "w") as f:
            json.dump(settings, f, indent=4)

        return {"status": "ok",
                "message": (f"Colors saved to VS Code settings!\n"
                            f"{VSCODE_SETTINGS}\n"
                            f"Reload VS Code (Ctrl+Shift+P → Reload Window) to see changes.")}

    # ── Helpers ──────────────────────────────────────────────────────
    def _build_ts_rules_block(self, rules):
        lines = ["const SCRIPTIT_TOKEN_RULES = ["]
        for r in rules:
            parts = [f"foreground: '{r.get('foreground', '#ffffff')}'"]
            if r.get("fontStyle"):
                parts.append(f"fontStyle: '{r['fontStyle']}'")
            lines.append(f"    {{ scope: '{r['scope']}', settings: {{ {', '.join(parts)} }} }},")
        lines.append("];")
        return "\n".join(lines)

    def _send_json(self, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def main():
    port = PORT
    if "--port" in sys.argv:
        idx = sys.argv.index("--port")
        if idx + 1 < len(sys.argv):
            port = int(sys.argv[idx + 1])

    if DEV_MODE:
        mode_str   = "DEV MODE  → saves to extension.ts"
        save_target = EXTENSION_TS
    else:
        mode_str   = "USER MODE → saves to VS Code settings.json"
        save_target = VSCODE_SETTINGS or "(not found)"

    server = HTTPServer(("127.0.0.1", port), CustomizerHandler)
    print(f"""
\033[1m\033[96m╔══════════════════════════════════════════════════╗
║  ScriptIt Color Customizer                        ║
╚══════════════════════════════════════════════════╝\033[0m

  \033[96mURL:\033[0m      http://localhost:{port}
  \033[96mMode:\033[0m     {mode_str}
  \033[96mSaves to:\033[0m {save_target}
  \033[96mPress\033[0m     Ctrl+C to stop
""")

    def open_browser():
        import time; time.sleep(1)
        webbrowser.open(f"http://localhost:{port}")

    threading.Thread(target=open_browser, daemon=True).start()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Server] Shutting down...")
        server.shutdown()


if __name__ == "__main__":
    main()
