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

#include "sif/lsp/SemanticTokens.h"

#include <algorithm>
#include "extern/utf8.h"

SIF_NAMESPACE_BEGIN
namespace lsp {

// Convert byte offset to character count in a UTF-8 string
static size_t ByteOffsetToCharCount(const std::string &str, size_t byteStart, size_t byteEnd) {
    if (byteStart >= str.size() || byteEnd > str.size() || byteStart > byteEnd) {
        return 0;
    }
    auto start = str.begin() + byteStart;
    auto end = str.begin() + byteEnd;
    return utf8::unchecked::distance(start, end);
}

bool SemanticTokensProvider::SemanticToken::operator<(const SemanticToken &other) const {
    if (range.start.offset != other.range.start.offset) {
        return range.start.offset < other.range.start.offset;
    }
    return range.end.offset < other.range.end.offset;
}

std::vector<uint32_t> SemanticTokensProvider::encodeTokens(const Document &doc) {
    std::vector<SemanticToken> tokens;

    collectFromAnnotations(tokens, doc);

    std::sort(tokens.begin(), tokens.end());

    std::vector<size_t> lineStarts;
    lineStarts.push_back(0);
    for (size_t i = 0; i < doc.content.size(); ++i) {
        if (doc.content[i] == '\n') {
            lineStarts.push_back(i + 1);
        }
    }

    std::vector<uint32_t> data;
    uint32_t prevLine = 0;
    uint32_t prevChar = 0;

    for (const auto &token : tokens) {
        uint32_t startLine = static_cast<uint32_t>(token.range.start.lineNumber);
        uint32_t endLine = static_cast<uint32_t>(token.range.end.lineNumber);

        for (uint32_t currentLine = startLine; currentLine <= endLine; ++currentLine) {
            size_t tokenFilePos = static_cast<size_t>(token.range.start.offset);
            uint32_t character = 0;
            uint32_t length = 0;

            if (currentLine < lineStarts.size()) {
                size_t lineStartPos = lineStarts[currentLine];
                size_t lineEndPos = (currentLine + 1 < lineStarts.size())
                                        ? lineStarts[currentLine + 1] - 1
                                        : doc.content.size();

                if (currentLine == startLine) {
                    character =
                        static_cast<uint32_t>(ByteOffsetToCharCount(doc.content, lineStartPos, tokenFilePos));
                    if (currentLine == endLine) {
                        length = static_cast<uint32_t>(ByteOffsetToCharCount(
                            doc.content, token.range.start.offset, token.range.end.offset));
                    } else {
                        length = static_cast<uint32_t>(
                            ByteOffsetToCharCount(doc.content, tokenFilePos, lineEndPos));
                    }
                } else if (currentLine == endLine) {
                    character = 0;
                    size_t tokenEndPos = token.range.end.offset;
                    length = static_cast<uint32_t>(
                        ByteOffsetToCharCount(doc.content, lineStartPos, tokenEndPos));
                } else {
                    character = 0;
                    length = static_cast<uint32_t>(
                        ByteOffsetToCharCount(doc.content, lineStartPos, lineEndPos));
                }
            }

            uint32_t deltaLine = currentLine - prevLine;
            uint32_t deltaChar = (deltaLine == 0) ? (character - prevChar) : character;

            data.push_back(deltaLine);
            data.push_back(deltaChar);
            data.push_back(length);
            data.push_back(static_cast<uint32_t>(token.type));
            data.push_back(token.modifiers);

            prevLine = currentLine;
            prevChar = character;
        }
    }

    return data;
}

void SemanticTokensProvider::collectFromAnnotations(std::vector<SemanticToken> &tokens,
                                                    const Document &doc) {
    if (!doc.ast) {
        return;
    }

    SourceAnnotator annotator;
    auto annotations = annotator.annotate(*doc.ast, doc.commentRanges);

    for (const auto &annotation : annotations) {
        SemanticToken token;
        token.range = annotation.range;
        token.type = annotationKindToTokenType(annotation.kind);
        token.modifiers = annotation.modifiers;
        tokens.push_back(token);
    }
}

SemanticTokensProvider::TokenType
SemanticTokensProvider::annotationKindToTokenType(Annotation::Kind kind) {
    switch (kind) {
    case Annotation::Kind::Keyword:
        return TokenType::Keyword;
    case Annotation::Kind::Function:
        return TokenType::Function;
    case Annotation::Kind::Variable:
        return TokenType::Variable;
    case Annotation::Kind::Operator:
        return TokenType::Operator;
    case Annotation::Kind::String:
        return TokenType::String;
    case Annotation::Kind::Number:
        return TokenType::Number;
    case Annotation::Kind::Comment:
        return TokenType::Comment;
    case Annotation::Kind::Namespace:
        return TokenType::Namespace;
    }
    return TokenType::Keyword; // unreachable
}

} // namespace lsp
SIF_NAMESPACE_END
