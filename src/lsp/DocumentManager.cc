//
//  Copyright (c) 2025 James Callender
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "sif/lsp/DocumentManager.h"
#include "sif/runtime/modules/Core.h"
#include "sif/runtime/modules/System.h"

#include <sif/Utilities.h>

SIF_NAMESPACE_BEGIN
namespace lsp {

// Convert file:// URI to filesystem path
static std::string UriToPath(const std::string &uri) {
    if (uri.substr(0, 7) != "file://") {
        return uri; // Not a URI, return as-is
    }

    std::string path = uri.substr(7); // Remove "file://"

    // On Windows, handle "file:///C:/..." → "C:/"
    // On Unix, "file:///home/..." → "/home/..."
    if (!path.empty() && path[0] == '/') {
#ifdef _WIN32
        // Windows: remove leading slash if we have drive letter
        if (path.length() > 2 && path[2] == ':') {
            path = path.substr(1);
        }
#endif
    }

    return path;
}

// Convert filesystem path to file:// URI
static std::string PathToUri(const std::string &path) {
    std::filesystem::path p(path);
    std::string absolute = std::filesystem::absolute(p).string();

    // Normalize slashes to forward slashes
    for (char &c : absolute) {
        if (c == '\\') {
            c = '/';
        }
    }

    // Add file:// prefix
    std::string uri = "file://";
    if (!absolute.empty() && absolute[0] != '/') {
        uri += '/'; // Windows paths need extra slash
    }
    uri += absolute;

    return uri;
}

// Resolve module path relative to importing document
static std::string ResolveModulePath(const std::string &importingDocUri,
                                     const std::string &moduleName,
                                     const std::string &workspaceRoot) {
    std::filesystem::path importingPath = UriToPath(importingDocUri);
    std::filesystem::path moduleNamePath(moduleName);

    // Try relative to importing file
    std::filesystem::path baseDir = importingPath.parent_path();
    std::filesystem::path resolvedPath = baseDir / moduleNamePath;

    // Add .sif extension if not present
    if (resolvedPath.extension().empty()) {
        resolvedPath += ".sif";
    }

    if (std::filesystem::exists(resolvedPath)) {
        return PathToUri(resolvedPath.string());
    }

    // Try with explicit extension on original name
    resolvedPath = baseDir / moduleName;
    if (std::filesystem::exists(resolvedPath)) {
        return PathToUri(resolvedPath.string());
    }

    // Try relative to workspace root
    if (!workspaceRoot.empty()) {
        std::filesystem::path workspacePath = UriToPath(workspaceRoot);
        resolvedPath = workspacePath / moduleNamePath;

        if (resolvedPath.extension().empty()) {
            resolvedPath += ".sif";
        }

        if (std::filesystem::exists(resolvedPath)) {
            return PathToUri(resolvedPath.string());
        }

        resolvedPath = workspacePath / moduleName;
        if (std::filesystem::exists(resolvedPath)) {
            return PathToUri(resolvedPath.string());
        }
    }

    // Not found
    return "";
}

void DocumentManager::setWorkspaceRoot(const std::string &rootUri) { _workspaceRoot = rootUri; }

void DocumentManager::openDocument(const std::string &uri, const std::string &content,
                                   int version) {
    auto doc = MakeStrong<Document>();
    doc->uri = uri;
    doc->content = content;
    doc->version = version;

    parseDocument(*doc);
    _documents[uri] = doc;

    // Update dependency graph
    for (const auto &importUri : doc->imports) {
        _dependents[importUri].insert(uri);
    }
}

std::vector<std::string> DocumentManager::updateDocument(const std::string &uri,
                                                         const std::string &content, int version) {
    std::vector<std::string> affectedUris;

    auto it = _documents.find(uri);
    if (it != _documents.end()) {
        // Remove old dependencies
        for (const auto &oldImportUri : it->second->imports) {
            _dependents[oldImportUri].erase(uri);
        }

        it->second->content = content;
        it->second->version = version;
        parseDocument(*it->second);
        affectedUris.push_back(uri);

        // Add new dependencies
        for (const auto &newImportUri : it->second->imports) {
            _dependents[newImportUri].insert(uri);
        }

        // Re-parse open dependents
        auto depsIt = _dependents.find(uri);
        if (depsIt != _dependents.end()) {
            for (const auto &dependentUri : depsIt->second) {
                auto depDoc = getDocument(dependentUri);
                if (depDoc) {
                    parseDocument(*depDoc);
                    affectedUris.push_back(dependentUri);
                }
            }
        }
    }

    return affectedUris;
}

void DocumentManager::closeDocument(const std::string &uri) { _documents.erase(uri); }

Strong<Document> DocumentManager::getDocument(const std::string &uri) {
    auto it = _documents.find(uri);
    if (it != _documents.end()) {
        return it->second;
    }
    return nullptr;
}

const std::map<std::string, Strong<Document>> &DocumentManager::documents() const {
    return _documents;
}

DocumentManager::LSPModuleProvider::LSPModuleProvider(DocumentManager &manager,
                                                      const std::string &currentDocUri)
    : _manager(manager), _currentDocUri(currentDocUri) {}

Result<Strong<Module>, Error> DocumentManager::LSPModuleProvider::module(const std::string &name) {
    // Resolve module path
    std::string moduleUri = ResolveModulePath(_currentDocUri, name, _manager._workspaceRoot);

    if (moduleUri.empty()) {
        return Fail(Error(SourceRange(), Concat("Module not found: ", name)));
    }

    // Check if module is already open in the editor
    auto doc = _manager.getDocument(moduleUri);
    if (doc) {
        // Use already parsed document
        auto userModule =
            MakeStrong<UserModule>(name, doc->signatures, Mapping<std::string, Value>());
        return userModule;
    }

    // Module not open, need to parse it from disk
    std::filesystem::path modulePath = UriToPath(moduleUri);

    if (!std::filesystem::exists(modulePath)) {
        return Fail(Error(SourceRange(), Concat("Module file not found: ", modulePath.string())));
    }

    // Parse the module file
    auto scanner = MakeStrong<Scanner>();
    auto reader = MakeStrong<FileReader>(modulePath.string());
    CaptureReporter reporter;

    LSPModuleProvider nestedProvider(_manager, moduleUri);
    ParserConfig config{.scanner = *scanner,
                        .reader = *reader,
                        .moduleProvider = nestedProvider,
                        .reporter = reporter};

    Parser parser(config);

    Core coreModule;
    System systemModule;
    parser.declare(coreModule.signatures());
    parser.declare(systemModule.signatures());

    // Parse the module file (use statement() to match ModuleLoader behavior)
    auto ast = parser.statement();

    if (parser.failed()) {
        return Fail(Error(SourceRange(), Concat("Failed to parse module: ", name)));
    }

    // Use only declarations (functions defined in this module), not all signatures
    auto userModule =
        MakeStrong<UserModule>(name, parser.declarations(), Mapping<std::string, Value>());
    return userModule;
}

void DocumentManager::parseDocument(Document &doc) {
    doc.errors.clear();
    doc.signatures.clear();
    doc.variables.clear();
    doc.commentRanges.clear();
    doc.imports.clear();

    doc.reader = MakeStrong<StringReader>(doc.content);
    doc.scanner = MakeStrong<Scanner>();
    doc.scanner->reset(doc.content);

    CaptureReporter reporter;
    LSPModuleProvider moduleProvider(*this, doc.uri);

    ParserConfig config{.scanner = *doc.scanner,
                        .reader = *doc.reader,
                        .moduleProvider = moduleProvider,
                        .reporter = reporter};

    Parser parser(config);

    Core coreModule;
    System systemModule;
    parser.declare(coreModule.signatures());
    parser.declare(systemModule.signatures());

    doc.ast = parser.parseBlock({Token::Type::EndOfFile});

    doc.errors = reporter.errors();
    doc.signatures = parser.signatures();
    doc.variables = parser.variables();
    doc.commentRanges = parser.commentRanges();

    // Extract imports from AST by walking use statements
    if (doc.ast) {
        auto extractImports = [&](const Strong<Statement> &stmt) {
            if (auto block = Cast<Block>(stmt)) {
                for (auto &s : block->statements) {
                    if (auto useStmt = Cast<Use>(s)) {
                        std::string moduleName = useStmt->target.encodedStringLiteralOrWord();
                        std::string moduleUri =
                            ResolveModulePath(doc.uri, moduleName, _workspaceRoot);
                        if (!moduleUri.empty()) {
                            doc.imports.push_back(moduleUri);
                        }
                    } else if (auto usingStmt = Cast<Using>(s)) {
                        std::string moduleName = usingStmt->target.encodedStringLiteralOrWord();
                        std::string moduleUri =
                            ResolveModulePath(doc.uri, moduleName, _workspaceRoot);
                        if (!moduleUri.empty()) {
                            doc.imports.push_back(moduleUri);
                        }
                    }
                }
            }
        };
        extractImports(doc.ast);
    }
}

} // namespace lsp
SIF_NAMESPACE_END
