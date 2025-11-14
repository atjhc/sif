# Sif Language Support for VS Code

This extension provides language support for the Sif programming language, including:

- Syntax highlighting
- Language server integration for intelligent code completion
- Document synchronization
- Error diagnostics (coming soon)

## Installation

### Step 1: Install Dependencies

```bash
cd support/vscode-extension
npm install
```

### Step 2: Compile the Extension

```bash
npm run compile
```

### Step 3: Install the Extension in VS Code

You have two options:

#### Option A: Install via Command Line (Recommended)

```bash
# From the support/vscode-extension directory
code --install-extension $(pwd)
```

Then reload VS Code.

#### Option B: Install via VS Code UI

1. Open VS Code
2. Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
3. Type "Extensions: Install from VSIX"
4. Navigate to the `support/vscode-extension` directory
5. Click "Install"

#### Option C: Development Mode (Best for Testing)

1. Open the `support/vscode-extension` folder in VS Code
2. Press `F5` to launch a new VS Code window with the extension loaded
3. Open a `.sif` file to activate the extension

### Step 4: Configure Paths (Optional)

The extension will try to automatically find the Sif executables. If it can't find them, you can set the paths manually:

1. Open VS Code Settings (`Cmd+,` or `Ctrl+,`)
2. Search for "sif"
3. Configure:
   - **Sif: Language Server Path** - Path to `sif_lsp` (for autocomplete and diagnostics)
   - **Sif: Interpreter Path** - Path to `sif_tool` (for running files)

Example paths:
```
/path/to/sif/build/debug/sif_lsp
/path/to/sif/build/debug/sif_tool
```

Or add to your `settings.json`:
```json
{
  "sif.languageServer.path": "/path/to/sif/build/debug/sif_lsp",
  "sif.interpreter.path": "/path/to/sif/build/debug/sif_tool"
}
```

## Usage

1. Create or open a `.sif` file
2. The extension will automatically activate
3. You should see "Sif Language Server started!" notification

### Running Sif Files

To run the current Sif file, use the Command Palette:

1. Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
2. Type "Run Sif File" and press Enter

The file will be executed in an integrated terminal, showing output directly in VS Code.
The file is automatically saved before running if it has unsaved changes.

**Optional: Add a Keyboard Shortcut**

To bind the run command to a keyboard shortcut:

1. Open Keyboard Shortcuts: `Cmd+K Cmd+S` (Mac) or `Ctrl+K Ctrl+S` (Windows/Linux)
2. Search for "Run Sif File"
3. Click the `+` icon and set your preferred keybinding (e.g., `Cmd+Shift+R`)

## Features

### Current Features

- **Syntax Highlighting**: Keywords, strings, numbers, comments, and operators are highlighted
- **Auto-closing Pairs**: Automatically closes brackets, parentheses, and quotes
- **Comment Toggle**: Use `Cmd+/` to toggle line comments
- **Run File**: Execute the current Sif file via Command Palette
- **Language Server**: Background parsing and analysis with autocomplete and diagnostics

### Upcoming Features

- **Semantic Highlighting**: More accurate syntax coloring based on semantic analysis
- **Auto-completion**: Intelligent suggestions for functions, variables, and keywords
- **Error Diagnostics**: Real-time error detection and reporting
- **Go to Definition**: Navigate to function and variable definitions
- **Hover Information**: See type and documentation on hover

## Troubleshooting

### Language Server Not Found

If you see an error about the language server not being found:

1. Make sure you've built the language server:
   ```bash
   cd /path/to/sif
   make debug
   ```

2. Verify the executable exists:
   ```bash
   ls -la /path/to/sif/build/debug/sif_lsp
   ```

3. Set the path manually in VS Code settings (see Step 4 above)

### Extension Not Activating

1. Check VS Code's output panel (`View > Output`)
2. Select "Sif Language Server" from the dropdown
3. Look for error messages

### Reloading the Extension

After making changes to the extension:

1. In the Extension Development Host window, press `Cmd+R` (Mac) or `Ctrl+R` (Windows/Linux)
2. Or use Command Palette: "Developer: Reload Window"

## Packaging and Distribution

To create a distributable `.vsix` package:

```bash
# 1. Build Sif binaries
cd /path/to/sif
make release

# 2. Copy binaries into extension
cd support/vscode-extension
./copy-binaries.sh release

# 3. Create package
npm run package
```

This creates `sif-language-X.X.X.vsix` with bundled binaries.

See `PACKAGING.md` and `README-PACKAGING.md` for detailed information on:
- Multi-platform builds
- Publishing to VS Code Marketplace
- How binary lookup works

## Development

### Project Structure

```
vscode-extension/
├── package.json           # Extension manifest
├── tsconfig.json          # TypeScript configuration
├── src/
│   └── extension.ts       # Extension entry point
├── syntaxes/
│   └── sif.tmLanguage.json  # Syntax highlighting rules
└── language-configuration.json  # Language-specific settings
```

### Building

```bash
npm run compile
```

### Watching for Changes

```bash
npm run watch
```

## License

Apache License 2.0
