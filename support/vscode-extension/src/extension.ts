import * as path from 'path';
import * as vscode from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  Executable
} from 'vscode-languageclient/node';

let client: LanguageClient;

function findExecutable(context: vscode.ExtensionContext, configKey: string, executableName: string): string | null {
  const config = vscode.workspace.getConfiguration('sif');
  let execPath = config.get<string>(configKey, '');

  // If configured, use that
  if (execPath) {
    return execPath;
  }

  // Try to find bundled executable first
  const platform = process.platform;
  const arch = process.arch;

  // Map Node.js platform/arch to VS Code naming
  let platformDir = '';
  if (platform === 'darwin' && arch === 'arm64') {
    platformDir = 'darwin-arm64';
  } else if (platform === 'darwin' && arch === 'x64') {
    platformDir = 'darwin-x64';
  } else if (platform === 'linux' && arch === 'x64') {
    platformDir = 'linux-x64';
  } else if (platform === 'win32' && arch === 'x64') {
    platformDir = 'win32-x64';
  }

  if (platformDir) {
    const bundledPath = path.join(context.extensionPath, 'bin', platformDir, executableName);
    if (require('fs').existsSync(bundledPath)) {
      return bundledPath;
    }
  }

  // Try to find in common locations relative to workspace (for development)
  const workspaceFolders = vscode.workspace.workspaceFolders;
  if (workspaceFolders && workspaceFolders.length > 0) {
    const workspaceRoot = workspaceFolders[0].uri.fsPath;
    const possiblePaths = [
      path.join(workspaceRoot, 'build', 'debug', executableName),
      path.join(workspaceRoot, 'build', 'release', executableName),
      path.join(workspaceRoot, '..', 'sif', 'build', 'debug', executableName),
      path.join(workspaceRoot, '..', 'sif', 'build', 'release', executableName),
    ];

    for (const testPath of possiblePaths) {
      try {
        if (require('fs').existsSync(testPath)) {
          return testPath;
        }
      } catch (e) {
        // Continue trying
      }
    }
  }

  return null;
}

export function activate(context: vscode.ExtensionContext) {
  // Register the run command
  const runCommand = vscode.commands.registerCommand('sif.runFile', () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
      vscode.window.showErrorMessage('No active Sif file to run');
      return;
    }

    if (editor.document.languageId !== 'sif') {
      vscode.window.showErrorMessage('Current file is not a Sif file');
      return;
    }

    const filePath = editor.document.uri.fsPath;

    // Save the file if it has unsaved changes
    if (editor.document.isDirty) {
      editor.document.save();
    }

    // Find the sif_tool interpreter
    const interpreterPath = findExecutable(context, 'interpreter.path', 'sif_tool');
    if (!interpreterPath) {
      vscode.window.showErrorMessage(
        'Sif interpreter not found. Please set "sif.interpreter.path" in settings.'
      );
      return;
    }

    // Create a terminal and run the file
    const terminal = vscode.window.createTerminal({
      name: 'Sif',
      cwd: path.dirname(filePath)
    });
    terminal.show();
    terminal.sendText(`"${interpreterPath}" "${filePath}"`);
  });

  context.subscriptions.push(runCommand);

  // Get the language server path
  const serverPath = findExecutable(context, 'languageServer.path', 'sif_lsp');
  if (!serverPath) {
    vscode.window.showErrorMessage(
      'Sif language server not found. Please set "sif.languageServer.path" in settings.'
    );
    return;
  }

  console.log(`Using Sif language server at: ${serverPath}`);

  const serverExecutable: Executable = {
    command: serverPath,
    args: []
  };

  const serverOptions: ServerOptions = serverExecutable;

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: 'file', language: 'sif' }],
    synchronize: {
      fileEvents: vscode.workspace.createFileSystemWatcher('**/*.sif')
    }
  };

  client = new LanguageClient(
    'sifLanguageServer',
    'Sif Language Server',
    serverOptions,
    clientOptions
  );

  client.start();

  vscode.window.showInformationMessage('Sif Language Server started!');
}

export function deactivate(): Thenable<void> | undefined {
  if (!client) {
    return undefined;
  }
  return client.stop();
}
