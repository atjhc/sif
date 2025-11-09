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

#include <map>
#include <sif/Common.h>
#include <sif/ast/Statement.h>
#include <sif/compiler/Module.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Reporter.h>
#include <sif/compiler/Scanner.h>
#include <string>

SIF_NAMESPACE_BEGIN
namespace lsp {

struct Document {
    std::string uri;
    std::string content;
    int version = 0;

    Strong<Scanner> scanner;
    Strong<Reader> reader;
    Strong<Statement> ast;
    std::vector<Error> errors;

    std::vector<Signature> signatures;
    Set<std::string> variables;
    std::vector<SourceRange> commentRanges;
};

class DocumentManager {
  public:
    DocumentManager() {}

    void openDocument(const std::string &uri, const std::string &content, int version);
    void updateDocument(const std::string &uri, const std::string &content, int version);
    void closeDocument(const std::string &uri);
    Strong<Document> getDocument(const std::string &uri);
    const std::map<std::string, Strong<Document>> &documents() const;

  private:
    std::map<std::string, Strong<Document>> _documents;

    class NoopModuleProvider : public ModuleProvider {
      public:
        Result<Strong<Module>, Error> module(const std::string &name) override;
    };

    void parseDocument(Document &doc);
};

} // namespace lsp
SIF_NAMESPACE_END
