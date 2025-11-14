# Packaging the Sif VS Code Extension

This document explains how to create a distributable `.vsix` package of the Sif extension.

## Overview

The extension can be packaged in two ways:

1. **With Bundled Binaries** (Recommended for distribution) - Includes `sif_lsp` and `sif_tool` executables
2. **Without Binaries** (For development) - Users configure paths manually

## Prerequisites

```bash
# Install packaging tool
npm install

# Build Sif binaries (from repository root)
cd /path/to/sif
make release
```

## Packaging with Bundled Binaries

### Step 1: Copy Binaries

From the `vscode-extension` directory:

```bash
# Copy release binaries for your platform
./copy-binaries.sh release

# Or copy debug binaries
./copy-binaries.sh debug
```

This copies `sif_lsp` and `sif_tool` to `bin/<platform>/` where `<platform>` is:
- `darwin-arm64` - macOS Apple Silicon
- `darwin-x64` - macOS Intel
- `linux-x64` - Linux x86_64
- `win32-x64` - Windows x64

### Step 2: Build Extension

```bash
npm run compile
```

### Step 3: Package

```bash
npm run package
```

This creates `sif-language-X.X.X.vsix` in the current directory.

## Multi-Platform Builds

To create a universal extension that works on all platforms, you need to build binaries on each platform and copy them all into the extension:

```bash
# On macOS ARM
./copy-binaries.sh release

# Copy to darwin-arm64/
# Then transfer darwin-arm64/ to extension repository

# On macOS Intel
./copy-binaries.sh release

# Copy to darwin-x64/
# Repeat for Linux and Windows...
```

Then package with all platforms' binaries present:

```bash
npm run package
```

## Platform-Specific Packages (Alternative)

Instead of one universal package, you can create platform-specific packages:

```bash
# Package only for current platform
vsce package --target darwin-arm64  # or darwin-x64, linux-x64, win32-x64
```

This creates smaller packages but requires separate distributions for each platform.

## Installation

### Local Installation

```bash
code --install-extension sif-language-X.X.X.vsix
```

### Publishing to Marketplace

```bash
# First time: create publisher account at https://marketplace.visualstudio.com/
# Then login
vsce login <publisher-name>

# Publish
npm run publish
```

## Binary Lookup Order

The extension searches for binaries in this order:

1. User-configured path in settings (`sif.languageServer.path`, `sif.interpreter.path`)
2. Bundled binary in `bin/<platform>/`
3. Auto-detected in workspace (`build/debug/`, `build/release/`)

This allows:
- Packaged extensions to work out-of-the-box (bundled binaries)
- Developers to use local builds (auto-detection)
- Users to override with custom builds (configured path)

## Directory Structure

```
vscode-extension/
├── bin/
│   ├── darwin-arm64/
│   │   ├── sif_lsp
│   │   └── sif_tool
│   ├── darwin-x64/
│   ├── linux-x64/
│   └── win32-x64/
├── out/                    # Compiled TypeScript
├── src/                    # TypeScript source
├── copy-binaries.sh        # Helper script
└── package.json
```

## Troubleshooting

### "Binary not found" after packaging

1. Verify binaries are in `bin/<platform>/`
2. Check they're executable: `chmod +x bin/*/sif_*`
3. Confirm `.vscodeignore` doesn't exclude `bin/`

### Package too large

Use platform-specific packaging or compress binaries with UPX:

```bash
upx --best bin/*/sif_*
```

### Cross-platform builds

Use GitHub Actions or Docker to build for multiple platforms. See `.github/workflows/` for examples.
