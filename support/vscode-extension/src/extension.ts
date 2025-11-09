import * as path from 'path';
import * as vscode from 'vscode';
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
  Executable
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
  // Get the language server path from settings or use default
  const config = vscode.workspace.getConfiguration('sif');
  let serverPath = config.get<string>('languageServer.path', '');

  // If not configured, try to find sif_lsp in common locations
  if (!serverPath) {
    // Try to find relative to the workspace
    const workspaceFolders = vscode.workspace.workspaceFolders;
    if (workspaceFolders && workspaceFolders.length > 0) {
      const workspaceRoot = workspaceFolders[0].uri.fsPath;
      const possiblePaths = [
        path.join(workspaceRoot, 'build', 'debug', 'sif_lsp'),
        path.join(workspaceRoot, 'build', 'release', 'sif_lsp'),
        path.join(workspaceRoot, '..', 'sif', 'build', 'debug', 'sif_lsp'),
        path.join(workspaceRoot, '..', 'sif', 'build', 'release', 'sif_lsp'),
      ];

      for (const testPath of possiblePaths) {
        try {
          if (require('fs').existsSync(testPath)) {
            serverPath = testPath;
            break;
          }
        } catch (e) {
          // Continue trying
        }
      }
    }
  }

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
