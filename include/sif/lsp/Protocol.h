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

#pragma once

#include <extern/json.hpp>
#include <optional>
#include <sif/Common.h>
#include <string>
#include <vector>

SIF_NAMESPACE_BEGIN
namespace lsp {

using json = nlohmann::json;

// JSON property name constants
namespace JsonKeys {
    // Common properties
    constexpr const char* text = "text";
    constexpr const char* range = "range";
    constexpr const char* rangeLength = "rangeLength";
    constexpr const char* line = "line";
    constexpr const char* character = "character";
    constexpr const char* uri = "uri";
    constexpr const char* start = "start";
    constexpr const char* end = "end";

    // Diagnostic properties
    constexpr const char* severity = "severity";
    constexpr const char* message = "message";
    constexpr const char* code = "code";
    constexpr const char* source = "source";

    // Completion properties
    constexpr const char* label = "label";
    constexpr const char* kind = "kind";
    constexpr const char* detail = "detail";
    constexpr const char* documentation = "documentation";
    constexpr const char* insertText = "insertText";

    // Server capabilities
    constexpr const char* textDocumentSync = "textDocumentSync";
    constexpr const char* completionProvider = "completionProvider";
    constexpr const char* semanticTokensProvider = "semanticTokensProvider";
    constexpr const char* capabilities = "capabilities";

    // Initialize properties
    constexpr const char* processId = "processId";
    constexpr const char* rootUri = "rootUri";

    // Request/Response properties
    constexpr const char* textDocument = "textDocument";
    constexpr const char* contentChanges = "contentChanges";
    constexpr const char* position = "position";
    constexpr const char* data = "data";
    constexpr const char* diagnostics = "diagnostics";

    // JSON-RPC properties
    constexpr const char* jsonrpc = "jsonrpc";
    constexpr const char* id = "id";
    constexpr const char* method = "method";
    constexpr const char* params = "params";
    constexpr const char* result = "result";
    constexpr const char* error = "error";
}

struct Position {
    int line = 0;
    int character = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, line, character)
};

struct Range {
    Position start;
    Position end;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Range, start, end)
};

struct Location {
    std::string uri;
    Range range;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Location, uri, range)
};

struct TextDocumentIdentifier {
    std::string uri;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentIdentifier, uri)
};

struct VersionedTextDocumentIdentifier : TextDocumentIdentifier {
    int version = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VersionedTextDocumentIdentifier, uri, version)
};

struct TextDocumentItem {
    std::string uri;
    std::string languageId;
    int version = 0;
    std::string text;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentItem, uri, languageId, version, text)
};

struct TextDocumentContentChangeEvent {
    std::optional<Range> range;
    std::optional<int> rangeLength;
    std::string text;
};

inline void to_json(json &j, const TextDocumentContentChangeEvent &e) {
    j = json{{JsonKeys::text, e.text}};
    if (e.range) {
        j[JsonKeys::range] = *e.range;
    }
    if (e.rangeLength) {
        j[JsonKeys::rangeLength] = *e.rangeLength;
    }
}

inline void from_json(const json &j, TextDocumentContentChangeEvent &e) {
    j.at(JsonKeys::text).get_to(e.text);
    if (j.contains(JsonKeys::range)) {
        e.range = j.at(JsonKeys::range).get<Range>();
    }
    if (j.contains(JsonKeys::rangeLength)) {
        e.rangeLength = j.at(JsonKeys::rangeLength).get<int>();
    }
}

enum class DiagnosticSeverity { Error = 1, Warning = 2, Information = 3, Hint = 4 };

struct Diagnostic {
    Range range;
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::optional<std::string> code;
    std::optional<std::string> source;
    std::string message;
};

inline void to_json(json &j, const Diagnostic &d) {
    j = json{{JsonKeys::range, d.range},
             {JsonKeys::severity, static_cast<int>(d.severity)},
             {JsonKeys::message, d.message}};
    if (d.code) {
        j[JsonKeys::code] = *d.code;
    }
    if (d.source) {
        j[JsonKeys::source] = *d.source;
    }
}

inline void from_json(const json &j, Diagnostic &d) {
    j.at(JsonKeys::range).get_to(d.range);
    j.at(JsonKeys::severity).get_to(d.severity);
    j.at(JsonKeys::message).get_to(d.message);
    if (j.contains(JsonKeys::code)) {
        d.code = j.at(JsonKeys::code).get<std::string>();
    }
    if (j.contains(JsonKeys::source)) {
        d.source = j.at(JsonKeys::source).get<std::string>();
    }
}

enum class CompletionItemKind {
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25
};

struct CompletionItem {
    std::string label;
    std::optional<CompletionItemKind> kind;
    std::optional<std::string> detail;
    std::optional<std::string> documentation;
    std::optional<std::string> insertText;
};

inline void to_json(json &j, const CompletionItem &item) {
    j = json{{JsonKeys::label, item.label}};
    if (item.kind) {
        j[JsonKeys::kind] = static_cast<int>(*item.kind);
    }
    if (item.detail) {
        j[JsonKeys::detail] = *item.detail;
    }
    if (item.documentation) {
        j[JsonKeys::documentation] = *item.documentation;
    }
    if (item.insertText) {
        j[JsonKeys::insertText] = *item.insertText;
    }
}

enum class SemanticTokenTypes {
    Namespace = 0,
    Type = 1,
    Class = 2,
    Enum = 3,
    Interface = 4,
    Struct = 5,
    TypeParameter = 6,
    Parameter = 7,
    Variable = 8,
    Property = 9,
    EnumMember = 10,
    Event = 11,
    Function = 12,
    Method = 13,
    Macro = 14,
    Keyword = 15,
    Modifier = 16,
    Comment = 17,
    String = 18,
    Number = 19,
    Regexp = 20,
    Operator = 21
};

enum class SemanticTokenModifiers {
    Declaration = 0,
    Definition = 1,
    Readonly = 2,
    Static = 3,
    Deprecated = 4,
    Abstract = 5,
    Async = 6,
    Modification = 7,
    Documentation = 8,
    DefaultLibrary = 9
};

struct SemanticTokensLegend {
    std::vector<std::string> tokenTypes;
    std::vector<std::string> tokenModifiers;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SemanticTokensLegend, tokenTypes, tokenModifiers)
};

struct ServerCapabilities {
    json textDocumentSync;
    std::optional<json> completionProvider;
    std::optional<json> semanticTokensProvider;
};

inline void to_json(json &j, const ServerCapabilities &cap) {
    j = json{{JsonKeys::textDocumentSync, cap.textDocumentSync}};
    if (cap.completionProvider) {
        j[JsonKeys::completionProvider] = *cap.completionProvider;
    }
    if (cap.semanticTokensProvider) {
        j[JsonKeys::semanticTokensProvider] = *cap.semanticTokensProvider;
    }
}

struct InitializeParams {
    std::optional<int> processId;
    std::optional<std::string> rootUri;
    json capabilities;
};

inline void from_json(const json &j, InitializeParams &p) {
    if (j.contains(JsonKeys::processId) && !j[JsonKeys::processId].is_null()) {
        p.processId = j[JsonKeys::processId].get<int>();
    }
    if (j.contains(JsonKeys::rootUri)) {
        p.rootUri = j[JsonKeys::rootUri].get<std::string>();
    }
    if (j.contains(JsonKeys::capabilities)) {
        p.capabilities = j[JsonKeys::capabilities];
    }
}

struct InitializeResult {
    ServerCapabilities capabilities;
};

inline void to_json(json &j, const InitializeResult &result) {
    j[JsonKeys::capabilities] = json::object();
    to_json(j[JsonKeys::capabilities], result.capabilities);
}

inline void from_json(const json &j, InitializeResult &result) {
    [[maybe_unused]] auto &caps = result.capabilities;
}

} // namespace lsp
SIF_NAMESPACE_END
