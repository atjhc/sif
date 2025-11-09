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

#include "sif/lsp/Server.h"
#include <iostream>

SIF_NAMESPACE_BEGIN
namespace lsp {

Server::Server(std::istream &in, std::ostream &out) : _rpc(in, out), _documentManager() {
    registerHandlers();
}

void Server::run() { _rpc.run(); }

void Server::registerHandlers() {
    _rpc.registerRequestHandler("initialize",
                                [this](const json &params) { return handleInitialize(params); });

    _rpc.registerNotificationHandler("initialized",
                                     [this](const json &params) { handleInitialized(params); });

    _rpc.registerRequestHandler("shutdown",
                                [this](const json &params) { return handleShutdown(params); });

    _rpc.registerNotificationHandler("exit", [this](const json &params) { handleExit(params); });

    _rpc.registerNotificationHandler("textDocument/didOpen",
                                     [this](const json &params) { handleDidOpen(params); });

    _rpc.registerNotificationHandler("textDocument/didChange",
                                     [this](const json &params) { handleDidChange(params); });

    _rpc.registerNotificationHandler("textDocument/didClose",
                                     [this](const json &params) { handleDidClose(params); });

    _rpc.registerRequestHandler("textDocument/semanticTokens/full", [this](const json &params) {
        return handleSemanticTokensFull(params);
    });

    _rpc.registerRequestHandler("textDocument/completion",
                                [this](const json &params) { return handleCompletion(params); });
}

json Server::handleInitialize(const json &params) {
    InitializeParams initParams = params.get<InitializeParams>();

    InitializeResult result;
    result.capabilities.textDocumentSync = json{{"openClose", true}, {"change", 1}};

    result.capabilities.completionProvider =
        json{{"triggerCharacters", json::array({" ", "{", "\""})}};

    SemanticTokensLegend legend;
    legend.tokenTypes = {"namespace",  "type",          "class",     "enum",     "interface",
                         "struct",     "typeParameter", "parameter", "variable", "property",
                         "enumMember", "event",         "function",  "method",   "macro",
                         "keyword",    "modifier",      "comment",   "string",   "number",
                         "regexp",     "operator"};
    legend.tokenModifiers = {"declaration",   "definition",    "readonly", "static",
                             "deprecated",    "abstract",      "async",    "modification",
                             "documentation", "defaultLibrary"};

    result.capabilities.semanticTokensProvider =
        json{{"legend", legend}, {"range", false}, {"full", true}};

    json response;
    to_json(response, result);
    return response;
}

void Server::handleInitialized(const json &params) {}

json Server::handleShutdown(const json &params) {
    _shutdownRequested = true;
    return nullptr;
}

void Server::handleExit(const json &params) { std::exit(_shutdownRequested ? 0 : 1); }

void Server::handleDidOpen(const json &params) {
    TextDocumentItem textDocument = params[JsonKeys::textDocument].get<TextDocumentItem>();
    _documentManager.openDocument(textDocument.uri, textDocument.text, textDocument.version);

    auto doc = _documentManager.getDocument(textDocument.uri);
    if (doc) {
        publishDiagnostics(textDocument.uri, doc->errors);
    }
}

void Server::handleDidChange(const json &params) {
    VersionedTextDocumentIdentifier textDocument =
        params[JsonKeys::textDocument].get<VersionedTextDocumentIdentifier>();

    std::vector<TextDocumentContentChangeEvent> contentChanges =
        params[JsonKeys::contentChanges].get<std::vector<TextDocumentContentChangeEvent>>();

    if (!contentChanges.empty()) {
        _documentManager.updateDocument(textDocument.uri, contentChanges[0].text,
                                        textDocument.version);

        auto doc = _documentManager.getDocument(textDocument.uri);
        if (doc) {
            publishDiagnostics(textDocument.uri, doc->errors);
        }
    }
}

void Server::handleDidClose(const json &params) {
    TextDocumentIdentifier textDocument = params[JsonKeys::textDocument].get<TextDocumentIdentifier>();
    _documentManager.closeDocument(textDocument.uri);
}

json Server::handleSemanticTokensFull(const json &params) {
    TextDocumentIdentifier textDocument = params[JsonKeys::textDocument].get<TextDocumentIdentifier>();

    auto doc = _documentManager.getDocument(textDocument.uri);
    if (!doc) {
        return json{{JsonKeys::data, json::array()}};
    }

    auto tokens = SemanticTokensProvider::encodeTokens(*doc);
    return json{{JsonKeys::data, tokens}};
}

json Server::handleCompletion(const json &params) {
    TextDocumentIdentifier textDocument = params[JsonKeys::textDocument].get<TextDocumentIdentifier>();
    [[maybe_unused]] Position position = params[JsonKeys::position].get<Position>();

    auto doc = _documentManager.getDocument(textDocument.uri);
    if (!doc) {
        return json::array();
    }

    return json::array();
}

void Server::publishDiagnostics(const std::string &uri, const std::vector<Error> &errors) {
    std::vector<Diagnostic> diagnostics;

    for (const auto &error : errors) {
        Diagnostic diag;
        diag.range = Range{
            {static_cast<int>(error.range.start.lineNumber),
             static_cast<int>(error.range.start.position)},
            {static_cast<int>(error.range.end.lineNumber),
             static_cast<int>(error.range.end.position)}};
        diag.severity = DiagnosticSeverity::Error;
        diag.message = error.what();
        diag.source = "sif";
        diagnostics.push_back(diag);
    }

    json params = {{JsonKeys::uri, uri}, {JsonKeys::diagnostics, diagnostics}};
    _rpc.sendNotification("textDocument/publishDiagnostics", params);
}

} // namespace lsp
SIF_NAMESPACE_END
