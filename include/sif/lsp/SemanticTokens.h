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

#include <sif/Common.h>
#include <sif/ast/SourceAnnotator.h>
#include <sif/compiler/Token.h>
#include <sif/lsp/DocumentManager.h>
#include <vector>

SIF_NAMESPACE_BEGIN
namespace lsp {

class SemanticTokensProvider {
  public:
    enum class TokenType : uint32_t {
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

    enum class TokenModifier : uint32_t {
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

    static std::vector<uint32_t> encodeTokens(const Document &doc);

  private:
    struct SemanticToken {
        SourceRange range;
        TokenType type;
        uint32_t modifiers = 0;

        bool operator<(const SemanticToken &other) const;
    };

    static TokenType annotationKindToTokenType(Annotation::Kind kind);
    static void collectFromAnnotations(std::vector<SemanticToken> &tokens, const Document &doc);
};

} // namespace lsp
SIF_NAMESPACE_END
