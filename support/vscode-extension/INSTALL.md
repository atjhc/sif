# Quick Installation Guide

## Prerequisites

- Node.js and npm installed
- VS Code installed
- Sif language server built at `<sif-repo>/build/debug/sif_lsp` or `<sif-repo>/build/release/sif_lsp`

## Method 1: Development Mode (Easiest for Testing)

1. **Open the extension folder in VS Code:**
   ```bash
   code <sif-repo>/support/vscode-extension
   ```
   (Replace `<sif-repo>` with the path to your Sif repository)

2. **Press F5**
   - This will compile the extension and launch a new VS Code window with it loaded
   - The new window will have "[Extension Development Host]" in the title

3. **Open or create a `.sif` file in the Extension Development Host window:**
   - You can use the included `test.sif` file
   - You should see "Sif Language Server started!" notification

4. **To reload after making changes:**
   - In the Extension Development Host window, press `Cmd+R` (Mac) or `Ctrl+R` (Windows/Linux)

## Method 2: Package and Install

1. **Install vsce (VS Code Extension Manager):**
   ```bash
   npm install -g @vscode/vsce
   ```

2. **Package the extension:**
   ```bash
   cd <sif-repo>/support/vscode-extension
   vsce package
   ```
   This creates a `.vsix` file.

3. **Install the extension:**
   ```bash
   code --install-extension sif-language-0.1.0.vsix
   ```

4. **Reload VS Code:**
   - Press `Cmd+Shift+P` (Mac) or `Ctrl+Shift+P` (Windows/Linux)
   - Type "Developer: Reload Window"
   - Press Enter

## Testing the Extension

1. **Create a test file:**
   ```bash
   cat > ~/test.sif << 'EOF'
   -- Test Sif file
   function greet {name}
       print "Hello, {name}!"
   end function

   set message to "Testing"
   greet message
   EOF
   ```

2. **Open it in VS Code:**
   ```bash
   code ~/test.sif
   ```

3. **What to expect:**
   - Syntax highlighting for keywords (if, function, end, etc.)
   - Comments should be grayed out
   - Strings should be colored differently
   - You should see a notification: "Sif Language Server started!"

## Verifying the Language Server

### Check the Output Panel

1. In VS Code, open the Output panel: `View > Output` (or `Cmd+Shift+U`)
2. From the dropdown on the right, select "Sif Language Server"
3. You should see messages about the language server starting

### Check for Errors

If you see errors:

1. **"Language server not found"**
   - Make sure the language server is built:
     ```bash
     ls -la <sif-repo>/build/debug/sif_lsp
     ```
   - If it doesn't exist, build it:
     ```bash
     cd <sif-repo>
     make debug
     ```

2. **"Cannot find module 'vscode-languageclient'"**
   - Install dependencies:
     ```bash
     cd <sif-repo>/support/vscode-extension
     npm install
     npm run compile
     ```

## Configuration

You can configure the language server path in VS Code settings:

1. Open Settings: `Cmd+,` (Mac) or `Ctrl+,` (Windows/Linux)
2. Search for "sif"
3. Set "Sif: Language Server Path" to your language server path

Or edit `settings.json` directly:
```json
{
  "sif.languageServer.path": "<sif-repo>/build/debug/sif_lsp"
}
```
(Replace `<sif-repo>` with the actual path to your Sif repository)

## Uninstalling

```bash
code --uninstall-extension sif.sif-language
```

Or through VS Code UI:
1. Open Extensions view (`Cmd+Shift+X`)
2. Find "Sif Language Support"
3. Click the gear icon
4. Select "Uninstall"
