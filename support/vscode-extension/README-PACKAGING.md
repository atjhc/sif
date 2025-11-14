# VS Code Extension Packaging Summary

## How Language Servers Are Typically Packaged

VS Code extensions with language servers use one of these approaches:

### 1. **Bundled Binaries** (Recommended - What we implemented)
- **Examples**: rust-analyzer, clangd, pylance
- **How**: Include pre-compiled executables in the extension package
- **Pros**: Works out-of-the-box, no user setup needed
- **Cons**: Large package size, need multi-platform builds

### 2. **Download on Demand**
- **Examples**: rust-analyzer (alternative mode), Go extension
- **How**: Download language server on first activation
- **Pros**: Smaller extension, auto-updates, platform-specific downloads
- **Cons**: Requires internet, more complex code

### 3. **User-Provided Path** (Our fallback)
- **Examples**: Development/testing scenarios
- **How**: User configures path in settings
- **Pros**: Simple, flexible for developers
- **Cons**: Not user-friendly, requires documentation

## Our Implementation

The Sif extension now uses a **hybrid approach**:

1. **First**: Check user-configured path in settings
2. **Second**: Look for bundled binary in `bin/<platform>/`
3. **Third**: Auto-detect in workspace build directories (for development)

This provides the best of all approaches:
- Packaged extensions work immediately (bundled binaries)
- Developers can use local builds (auto-detection)
- Power users can override (configured path)

## Quick Start

### For End Users (Installing from .vsix)

```bash
# Install the packaged extension
code --install-extension sif-language-0.1.0.vsix
```

The extension automatically uses bundled binaries. No configuration needed!

### For Developers (Local Development)

```bash
cd support/vscode-extension
npm install
npm run compile

# Extension will auto-detect binaries in ../../build/release/
# Just press F5 to launch Extension Development Host
```

### For Package Maintainers (Creating .vsix)

```bash
# Build Sif binaries
cd /path/to/sif
make release

# Copy binaries into extension
cd support/vscode-extension
./copy-binaries.sh release

# Create package
npm run package
```

This creates `sif-language-X.X.X.vsix` with bundled binaries for your platform.

## File Structure

```
vscode-extension/
├── bin/                          # Bundled binaries (not in git)
│   └── darwin-arm64/             # Platform-specific
│       ├── sif_lsp              # Language server
│       └── sif_tool             # Interpreter
├── out/                          # Compiled TypeScript
├── src/
│   └── extension.ts             # Binary lookup logic
├── copy-binaries.sh             # Helper to copy binaries
├── PACKAGING.md                 # Detailed packaging guide
└── package.json                 # Extension manifest
```

## Commands

```bash
# Development
npm run compile       # Build TypeScript
npm run watch         # Watch mode

# Packaging
./copy-binaries.sh    # Copy binaries
npm run package       # Create .vsix
npm run publish       # Publish to marketplace (requires setup)
```

See `PACKAGING.md` for detailed instructions on multi-platform builds and publishing.
