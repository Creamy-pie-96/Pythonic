#!/usr/bin/env python3
"""
═══════════════════════════════════════════════════════════════
  ScriptIt — Complete Installer
═══════════════════════════════════════════════════════════════

  This script installs EVERYTHING from zero:
    1. Checks & installs system dependencies (cmake, g++, node, npm)
    2. Builds ScriptIt via CMake and installs system-wide
    3. Installs npm dependencies for the VS Code extension
    4. Compiles the TypeScript extension code
    5. Packages the extension into a .vsix file
    6. Installs the .vsix into VS Code

  Usage:
    python3 install.py              # Full install (needs sudo for system-wide)
    python3 install.py --ext-only   # Only build & install the VS Code extension
    python3 install.py --build-only # Only build ScriptIt binary
    python3 install.py --check      # Just check what's installed / missing

═══════════════════════════════════════════════════════════════
"""

import os
import sys
import shutil
import subprocess
import argparse
import platform
from pathlib import Path

# ── Paths ─────────────────────────────────────────────────

SCRIPT_DIR = Path(__file__).resolve().parent
# The REPL directory containing ScriptIt source
REPL_DIR = SCRIPT_DIR.parent if SCRIPT_DIR.name == "scriptit-vscode" else SCRIPT_DIR
# If install.py is inside scriptit-vscode/, REPL is parent
# If install.py is in REPL/ directly, use SCRIPT_DIR
# Detect properly:
if (SCRIPT_DIR / "package.json").exists():
    # We're inside scriptit-vscode/
    REPL_DIR = SCRIPT_DIR.parent
    EXT_DIR = SCRIPT_DIR
elif (SCRIPT_DIR / "scriptit-vscode" / "package.json").exists():
    REPL_DIR = SCRIPT_DIR
    EXT_DIR = SCRIPT_DIR / "scriptit-vscode"
else:
    print("ERROR: Cannot find project structure. Run this from REPL/ or scriptit-vscode/")
    sys.exit(1)

CMAKE_DIR = REPL_DIR  # CMakeLists.txt lives here
BUILD_DIR = REPL_DIR / "build_scriptit"
NOTEBOOK_DIR = REPL_DIR / "notebook"

# ── Colors ────────────────────────────────────────────────

class C:
    BOLD  = "\033[1m"
    GREEN = "\033[92m"
    BLUE  = "\033[94m"
    YELLOW= "\033[93m"
    RED   = "\033[91m"
    CYAN  = "\033[96m"
    RESET = "\033[0m"

def info(msg):  print(f"{C.BLUE}[INFO]{C.RESET} {msg}")
def ok(msg):    print(f"{C.GREEN}[  OK]{C.RESET} {msg}")
def warn(msg):  print(f"{C.YELLOW}[WARN]{C.RESET} {msg}")
def err(msg):   print(f"{C.RED}[ ERR]{C.RESET} {msg}")
def step(msg):  print(f"\n{C.BOLD}{C.CYAN}{'═'*60}\n  {msg}\n{'═'*60}{C.RESET}")

def run(cmd, cwd=None, check=True, capture=False):
    """Run a shell command, print it, and return result."""
    if isinstance(cmd, str):
        display = cmd
    else:
        display = " ".join(str(c) for c in cmd)
    info(f"$ {display}")
    try:
        result = subprocess.run(
            cmd, shell=isinstance(cmd, str), cwd=cwd,
            check=check, capture_output=capture, text=True
        )
        return result
    except subprocess.CalledProcessError as e:
        if capture:
            err(f"Command failed: {e.stderr or e.stdout or str(e)}")
        else:
            err(f"Command failed with exit code {e.returncode}")
        if check:
            raise
        return e

def which(name):
    """Check if a command exists."""
    return shutil.which(name)

def get_version(cmd):
    """Get version string from a command."""
    try:
        r = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=5)
        return r.stdout.strip() or r.stderr.strip()
    except:
        return None

# ══════════════════════════════════════════════════════════
# Step 1: Check & Install System Dependencies
# ══════════════════════════════════════════════════════════

def check_dependencies():
    step("Checking System Dependencies")

    deps = {}

    # g++
    if which("g++"):
        ver = get_version("g++ --version | head -1")
        ok(f"g++ found: {ver}")
        deps["g++"] = True
    else:
        warn("g++ NOT found")
        deps["g++"] = False

    # cmake
    if which("cmake"):
        ver = get_version("cmake --version | head -1")
        ok(f"cmake found: {ver}")
        deps["cmake"] = True
    else:
        warn("cmake NOT found")
        deps["cmake"] = False

    # node
    if which("node"):
        ver = get_version("node --version")
        ok(f"node found: {ver}")
        # Check if version >= 20
        major = int(ver.lstrip("v").split(".")[0]) if ver else 0
        if major < 20:
            warn(f"Node.js {ver} is too old — need >= 20 for extension packaging")
            deps["node"] = "old"
        else:
            deps["node"] = True
    else:
        warn("node NOT found")
        deps["node"] = False

    # npm
    if which("npm"):
        ver = get_version("npm --version")
        ok(f"npm found: {ver}")
        deps["npm"] = True
    else:
        warn("npm NOT found")
        deps["npm"] = False

    # VS Code
    vscode_cmd = None
    for cmd in ["code", "code-insiders", "codium"]:
        if which(cmd):
            vscode_cmd = cmd
            break
    if vscode_cmd:
        ok(f"VS Code found: {vscode_cmd}")
        deps["vscode"] = vscode_cmd
    else:
        warn("VS Code CLI ('code') NOT found in PATH")
        deps["vscode"] = False

    # python3 (we're running it, so it exists)
    ok(f"python3: {sys.version.split()[0]}")
    deps["python3"] = True

    return deps

def install_missing_deps(deps):
    step("Installing Missing Dependencies")

    system = platform.system()
    missing = [k for k, v in deps.items() if v is False or v == "old"]

    if not missing:
        ok("All dependencies are already installed!")
        return True

    info(f"Need to install/upgrade: {', '.join(missing)}")

    if system == "Linux":
        distro = ""
        try:
            with open("/etc/os-release") as f:
                for line in f:
                    if line.startswith("ID="):
                        distro = line.strip().split("=")[1].strip('"')
                        break
        except:
            distro = "unknown"

        if distro in ("ubuntu", "debian", "pop", "mint", "elementary"):
            return install_deps_apt(deps, missing)
        elif distro in ("fedora", "rhel", "centos", "rocky", "alma"):
            return install_deps_dnf(deps, missing)
        elif distro in ("arch", "manjaro", "endeavouros"):
            return install_deps_pacman(deps, missing)
        else:
            warn(f"Unknown distro '{distro}' — trying apt")
            return install_deps_apt(deps, missing)

    elif system == "Darwin":
        return install_deps_brew(deps, missing)

    else:
        err(f"Unsupported OS: {system}. Please install manually: {', '.join(missing)}")
        return False

def install_deps_apt(deps, missing):
    pkgs = []
    if "g++" in missing: pkgs.append("g++")
    if "cmake" in missing: pkgs.append("cmake")

    if pkgs:
        run(f"sudo apt update && sudo apt install -y {' '.join(pkgs)}")

    # Node.js — need version 20+
    if "node" in missing or deps.get("node") == "old":
        info("Installing Node.js 22 from NodeSource...")
        run("curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -")
        run("sudo apt install -y nodejs")

    if "npm" in missing and "node" not in missing:
        run("sudo apt install -y npm")

    return True

def install_deps_dnf(deps, missing):
    pkgs = []
    if "g++" in missing: pkgs.append("gcc-c++")
    if "cmake" in missing: pkgs.append("cmake")
    if pkgs:
        run(f"sudo dnf install -y {' '.join(pkgs)}")

    if "node" in missing or deps.get("node") == "old":
        run("curl -fsSL https://rpm.nodesource.com/setup_22.x | sudo bash -")
        run("sudo dnf install -y nodejs")

    return True

def install_deps_pacman(deps, missing):
    pkgs = []
    if "g++" in missing: pkgs.append("gcc")
    if "cmake" in missing: pkgs.append("cmake")
    if "node" in missing or deps.get("node") == "old":
        pkgs.append("nodejs")
        pkgs.append("npm")
    if pkgs:
        run(f"sudo pacman -S --noconfirm {' '.join(pkgs)}")
    return True

def install_deps_brew(deps, missing):
    pkgs = []
    if "g++" in missing: pkgs.append("gcc")
    if "cmake" in missing: pkgs.append("cmake")
    if "node" in missing or deps.get("node") == "old":
        pkgs.append("node")
    if pkgs:
        run(f"brew install {' '.join(pkgs)}")
    return True

# ══════════════════════════════════════════════════════════
# Step 2: Build ScriptIt via CMake
# ══════════════════════════════════════════════════════════

def build_scriptit():
    step("Building ScriptIt via CMake")

    cmake_file = CMAKE_DIR / "CMakeLists.txt"
    if not cmake_file.exists():
        err(f"CMakeLists.txt not found at {cmake_file}")
        return False

    BUILD_DIR.mkdir(exist_ok=True)

    # Configure
    run(f"cmake .. -DCMAKE_BUILD_TYPE=Release", cwd=BUILD_DIR)

    # Build
    run(f"cmake --build . --config Release -j{os.cpu_count() or 4}", cwd=BUILD_DIR)

    # Check the binary was created
    binary = BUILD_DIR / "scriptit"
    if not binary.exists():
        # Try Windows
        binary = BUILD_DIR / "Release" / "scriptit.exe"
    if not binary.exists():
        binary = BUILD_DIR / "scriptit.exe"

    if binary.exists():
        ok(f"ScriptIt binary built: {binary}")
    else:
        warn("Binary not found at expected location — check build output")

    return True

def install_scriptit():
    step("Installing ScriptIt System-Wide")
    info("(Requires sudo)")

    build_abs = str(BUILD_DIR.resolve())
    binary = BUILD_DIR / "scriptit"

    # FUSE/encrypted filesystems (gocryptfs, ecryptfs) may block root access.
    # Detect this and copy via temp location if needed.
    import tempfile

    def _needs_temp_copy():
        """Check if sudo can't read our build dir (common with FUSE mounts)."""
        try:
            r = subprocess.run(
                ["sudo", "test", "-r", str(binary)],
                capture_output=True, timeout=5
            )
            return r.returncode != 0
        except:
            return True

    use_temp = _needs_temp_copy() if binary.exists() else False

    try:
        if use_temp:
            info("Detected FUSE/encrypted mount — copying via temp directory")
            with tempfile.TemporaryDirectory() as tmpdir:
                # Copy the entire build dir as current user so sudo can access it
                tmp_build = Path(tmpdir) / "build_scriptit"
                shutil.copytree(build_abs, str(tmp_build))
                # Also need to copy the binary into the temp build dir
                # because cmake_install.cmake references the original binary path
                tmp_binary = tmp_build / "scriptit"
                if not tmp_binary.exists() and binary.exists():
                    shutil.copy2(str(binary), str(tmp_binary))
                # Patch the cmake_install.cmake to point to the temp binary
                cmake_install = tmp_build / "cmake_install.cmake"
                if cmake_install.exists():
                    content = cmake_install.read_text()
                    content = content.replace(build_abs, str(tmp_build))
                    # Also replace the REPL source paths for notebook files etc.
                    repl_abs = str(REPL_DIR.resolve())
                    tmp_repl = Path(tmpdir) / "repl_src"
                    if repl_abs in content:
                        # Copy files that cmake_install references
                        notebook_src = REPL_DIR / "notebook"
                        if notebook_src.exists():
                            shutil.copytree(str(notebook_src), str(tmp_repl / "notebook"), dirs_exist_ok=True)
                        notebook_sh = REPL_DIR / "notebook.sh"
                        if notebook_sh.exists():
                            shutil.copy2(str(notebook_sh), str(tmp_repl / "notebook.sh"))
                        content = content.replace(repl_abs, str(tmp_repl))
                    cmake_install.write_text(content)
                run(f"sudo cmake --install {tmp_build}")
                ok("ScriptIt installed system-wide!")
        else:
            run(f"sudo cmake --install {build_abs}", cwd=None)
            ok("ScriptIt installed system-wide!")

        # Verify
        if which("scriptit"):
            ver = get_version("scriptit --version 2>&1 || echo 'installed'")
            ok(f"scriptit in PATH: {ver}")
        else:
            warn("'scriptit' not in PATH — you may need to restart your shell or add /usr/local/bin to PATH")

        return True
    except:
        warn("System-wide install failed — trying to copy binary manually")
        if binary.exists():
            binary_abs = str(binary.resolve())
            try:
                if use_temp:
                    with tempfile.TemporaryDirectory() as tmpdir:
                        tmp_bin = Path(tmpdir) / "scriptit"
                        shutil.copy2(binary_abs, str(tmp_bin))
                        run(f"sudo cp '{tmp_bin}' /usr/local/bin/scriptit")
                        run("sudo chmod +x /usr/local/bin/scriptit")
                else:
                    run(f"sudo cp '{binary_abs}' /usr/local/bin/scriptit")
                    run("sudo chmod +x /usr/local/bin/scriptit")
                ok("Manually copied to /usr/local/bin/scriptit")
                return True
            except:
                err("Could not install binary. You can manually copy it from:")
                err(f"  {binary_abs}")
                return False
        return False

# ══════════════════════════════════════════════════════════
# Step 3: Build VS Code Extension
# ══════════════════════════════════════════════════════════

def build_extension():
    step("Building VS Code Extension")

    pkg_json = EXT_DIR / "package.json"
    if not pkg_json.exists():
        err(f"package.json not found at {pkg_json}")
        return False

    # npm install
    info("Installing npm dependencies...")
    run("npm install", cwd=EXT_DIR)
    ok("npm dependencies installed")

    # TypeScript compile
    info("Compiling TypeScript...")
    run("npx tsc -b", cwd=EXT_DIR)
    ok("TypeScript compiled successfully")

    # Check output exists
    out_dir = EXT_DIR / "out"
    if out_dir.exists():
        js_files = list(out_dir.rglob("*.js"))
        ok(f"Compiled {len(js_files)} JavaScript files to out/")
    else:
        err("out/ directory not created — compilation may have failed")
        return False

    return True

def package_extension():
    step("Packaging Extension (.vsix)")

    # Install vsce if not present
    info("Packaging with @vscode/vsce...")
    run("npx @vscode/vsce package --allow-missing-repository --skip-license", cwd=EXT_DIR)

    # Find the .vsix file
    vsix_files = list(EXT_DIR.glob("*.vsix"))
    if vsix_files:
        vsix = vsix_files[-1]  # latest
        ok(f"Extension packaged: {vsix}")
        return vsix
    else:
        err("No .vsix file created!")
        return None

def install_extension(vsix_path):
    step("Installing Extension into VS Code")

    # Find VS Code command
    vscode_cmd = None
    for cmd in ["code", "code-insiders", "codium"]:
        if which(cmd):
            vscode_cmd = cmd
            break

    if not vscode_cmd:
        warn("VS Code CLI not found in PATH.")
        warn(f"You can manually install the extension:")
        warn(f"  code --install-extension {vsix_path}")
        return False

    try:
        run(f"{vscode_cmd} --install-extension {vsix_path} --force")
        ok(f"Extension installed via '{vscode_cmd}'!")
        ok("Restart VS Code to activate the extension.")
        return True
    except:
        warn(f"Auto-install failed. Manually install with:")
        warn(f"  {vscode_cmd} --install-extension {vsix_path}")
        return False

# ══════════════════════════════════════════════════════════
# Main
# ══════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description="ScriptIt Complete Installer",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument("--ext-only", action="store_true",
                        help="Only build & install the VS Code extension")
    parser.add_argument("--build-only", action="store_true",
                        help="Only build the ScriptIt binary")
    parser.add_argument("--check", action="store_true",
                        help="Just check dependencies, don't install anything")
    parser.add_argument("--no-install", action="store_true",
                        help="Build everything but don't install system-wide or into VS Code")
    args = parser.parse_args()

    print(f"""
{C.BOLD}{C.CYAN}
  ╔═══════════════════════════════════════════════════╗
  ║          ScriptIt — Complete Installer            ║
  ║                                                   ║
  ║  Language + VS Code Extension + Notebook          ║
  ╚═══════════════════════════════════════════════════╝
{C.RESET}
  REPL dir:      {REPL_DIR}
  Extension dir: {EXT_DIR}
  Build dir:     {BUILD_DIR}
""")

    # ── Check Dependencies ──
    deps = check_dependencies()

    if args.check:
        print(f"\n{C.BOLD}Summary:{C.RESET}")
        all_ok = all(v is True or (isinstance(v, str) and v not in ("old", "False"))
                     for v in deps.values())
        if all_ok:
            ok("All dependencies are satisfied!")
        else:
            missing = [k for k, v in deps.items() if v is False or v == "old"]
            warn(f"Missing/outdated: {', '.join(missing)}")
        return

    # ── Install Missing Deps ──
    missing = [k for k, v in deps.items()
               if v is False or v == "old"]
    # Don't require vscode for build-only
    if args.build_only and "vscode" in missing:
        missing.remove("vscode")

    if missing:
        install_missing_deps(deps)
        # Re-check
        deps = check_dependencies()

    # ── Build ScriptIt ──
    if not args.ext_only:
        try:
            build_scriptit()
            if not args.no_install:
                install_scriptit()
        except Exception as e:
            err(f"ScriptIt build failed: {e}")
            if not args.build_only:
                warn("Continuing with extension build...")

    if args.build_only:
        print(f"\n{C.GREEN}{C.BOLD}ScriptIt build complete!{C.RESET}")
        return

    # ── Build Extension ──
    try:
        build_extension()
    except Exception as e:
        err(f"Extension build failed: {e}")
        sys.exit(1)

    # ── Package Extension ──
    vsix = None
    try:
        vsix = package_extension()
    except Exception as e:
        err(f"Extension packaging failed: {e}")
        warn("The extension is still built in out/ — you can use F5 in VS Code to test it.")

    # ── Install Extension ──
    if vsix and not args.no_install:
        install_extension(vsix)

    # ── Done ──
    print(f"""
{C.BOLD}{C.GREEN}
  ╔═══════════════════════════════════════════════════╗
  ║              Installation Complete! 🎉            ║
  ╚═══════════════════════════════════════════════════╝
{C.RESET}
  What was installed:
    ✓ ScriptIt binary → /usr/local/bin/scriptit
    ✓ VS Code extension → scriptit-lang
  
  Quick start:
    • Create a file with .sit extension and open in VS Code
    • Open any .nsit file for notebook experience
    • Press Ctrl+Shift+R to run a ScriptIt file
    • Type 'fn' + Tab for function snippet

  Files:
    • Extension: {EXT_DIR}
    • VSIX:      {vsix or 'not packaged'}
    • Binary:    {BUILD_DIR / 'scriptit'}
""")

if __name__ == "__main__":
    main()
