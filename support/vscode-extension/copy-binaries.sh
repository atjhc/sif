#!/bin/bash

# Script to copy Sif binaries into the VS Code extension for packaging
# Usage: ./copy-binaries.sh [debug|release]

BUILD_TYPE="${1:-release}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SIF_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Copying binaries from $BUILD_TYPE build..."

# Detect current platform
PLATFORM=$(uname -s)
ARCH=$(uname -m)

if [ "$PLATFORM" = "Darwin" ]; then
    if [ "$ARCH" = "arm64" ]; then
        PLATFORM_DIR="darwin-arm64"
    else
        PLATFORM_DIR="darwin-x64"
    fi
elif [ "$PLATFORM" = "Linux" ]; then
    PLATFORM_DIR="linux-x64"
else
    echo "Unsupported platform: $PLATFORM"
    exit 1
fi

BIN_DIR="$SCRIPT_DIR/bin/$PLATFORM_DIR"
mkdir -p "$BIN_DIR"

# Copy binaries
cp "$SIF_ROOT/build/$BUILD_TYPE/sif_lsp" "$BIN_DIR/" 2>/dev/null || echo "Warning: sif_lsp not found"
cp "$SIF_ROOT/build/$BUILD_TYPE/sif_tool" "$BIN_DIR/" 2>/dev/null || echo "Warning: sif_tool not found"

# Make them executable
chmod +x "$BIN_DIR/sif_lsp" 2>/dev/null
chmod +x "$BIN_DIR/sif_tool" 2>/dev/null

echo "Binaries copied to $BIN_DIR/"
ls -lh "$BIN_DIR/"
