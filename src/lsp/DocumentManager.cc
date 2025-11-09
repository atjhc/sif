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

SIF_NAMESPACE_BEGIN
namespace lsp {

void DocumentManager::openDocument(const std::string &uri, const std::string &content,
                                   int version) {
    auto doc = MakeStrong<Document>();
    doc->uri = uri;
    doc->content = content;
    doc->version = version;

    parseDocument(*doc);
    _documents[uri] = doc;
}

void DocumentManager::updateDocument(const std::string &uri, const std::string &content,
                                     int version) {
    auto it = _documents.find(uri);
    if (it != _documents.end()) {
        it->second->content = content;
        it->second->version = version;
        parseDocument(*it->second);
    }
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

Result<Strong<Module>, Error> DocumentManager::NoopModuleProvider::module(const std::string &name) {
    return Fail(Error(SourceRange(), "Module loading not supported in LSP"));
}

void DocumentManager::parseDocument(Document &doc) {
    doc.errors.clear();
    doc.signatures.clear();
    doc.variables.clear();
    doc.commentRanges.clear();

    doc.reader = MakeStrong<StringReader>(doc.content);
    doc.scanner = MakeStrong<Scanner>();
    doc.scanner->reset(doc.content);

    CaptureReporter reporter;
    NoopModuleProvider moduleProvider;

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
}

} // namespace lsp
SIF_NAMESPACE_END
