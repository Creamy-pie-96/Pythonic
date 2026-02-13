#!/bin/bash
# Build and install ScriptIt system-wide
# Usage: ./install.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHONIC_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
SRC="$SCRIPT_DIR/ScriptIt.cpp"
STUBS="$PYTHONIC_ROOT/src/pythonicDispatchStubs.cpp"
INCLUDE="$PYTHONIC_ROOT/include"
OUTPUT="/usr/local/bin/scriptit"

echo "🔨 Building ScriptIt v2..."
echo "   Source: $SRC"
echo "   Include: $INCLUDE"

g++ -std=c++20 -O2 -I"$INCLUDE" -o /tmp/scriptit_build "$SRC" "$STUBS"

echo "📦 Installing to $OUTPUT..."
sudo cp /tmp/scriptit_build "$OUTPUT"
sudo chmod +x "$OUTPUT"
rm /tmp/scriptit_build

echo ""
echo "✅ ScriptIt installed successfully!"
echo ""
echo "Usage:"
echo "  scriptit                  # Interactive REPL"
echo "  scriptit myprogram.sit    # Run a .sit script"
echo "  scriptit --test           # Run built-in self-test"
