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

#include "sif/lsp/CompletionUtils.h"
#include "extern/utf8.h"
#include <sstream>
#include <variant>

SIF_NAMESPACE_BEGIN
namespace lsp {

// Convert UTF-8 character position to byte offset
static size_t CharPosToByteOffset(const std::string &str, size_t charPos) {
    if (charPos == 0 || str.empty()) {
        return 0;
    }

    auto it = str.begin();
    size_t count = 0;

    while (it < str.end() && count < charPos) {
        utf8::unchecked::next(it);
        count++;
    }

    return std::distance(str.begin(), it);
}

std::vector<std::vector<Signature::Term>>
GenerateSignatureVariations(const std::vector<Signature::Term> &terms) {
    if (terms.empty()) {
        return {{}};
    }

    // Recursively process rest of terms
    auto rest = std::vector<Signature::Term>(terms.begin() + 1, terms.end());
    auto restVariations = GenerateSignatureVariations(rest);

    std::vector<std::vector<Signature::Term>> result;
    const auto &first = terms[0];

    std::visit(
        [&](auto &&term) {
            using T = std::decay_t<decltype(term)>;

            if constexpr (std::is_same_v<T, Signature::Option>) {
                // Option: generate versions with and without
                // Without the option
                result.insert(result.end(), restVariations.begin(), restVariations.end());

                // With the option (each choice token)
                for (const auto &token : term.choice.tokens) {
                    for (const auto &restVar : restVariations) {
                        std::vector<Signature::Term> var;
                        var.push_back(token);
                        var.insert(var.end(), restVar.begin(), restVar.end());
                        result.push_back(var);
                    }
                }
            } else if constexpr (std::is_same_v<T, Signature::Choice>) {
                // Choice: one variation per choice token
                for (const auto &token : term.tokens) {
                    for (const auto &restVar : restVariations) {
                        std::vector<Signature::Term> var;
                        var.push_back(token);
                        var.insert(var.end(), restVar.begin(), restVar.end());
                        result.push_back(var);
                    }
                }
            } else {
                // Token or Argument: required, include as-is
                for (const auto &restVar : restVariations) {
                    std::vector<Signature::Term> var;
                    var.push_back(first);
                    var.insert(var.end(), restVar.begin(), restVar.end());
                    result.push_back(var);
                }
            }
        },
        first);

    return result;
}

std::string TermsToSnippet(const std::vector<Signature::Term> &terms) {
    std::ostringstream result;
    int tabStop = 1;

    for (size_t i = 0; i < terms.size(); ++i) {
        std::visit(
            [&](auto &&term) {
                using T = std::decay_t<decltype(term)>;

                if constexpr (std::is_same_v<T, Token>) {
                    result << term.text;
                } else if constexpr (std::is_same_v<T, Signature::Argument>) {
                    // Create LSP tab stop ${tabstop:placeholder}
                    result << "${" << tabStop++ << ":";

                    // Use first target's name if available
                    if (!term.targets.empty() && term.targets[0].name.has_value()) {
                        result << term.targets[0].name.value().text;
                    } else {
                        result << "value";
                    }

                    result << "}";
                }
            },
            terms[i]);

        if (i + 1 < terms.size()) {
            result << " ";
        }
    }

    return result.str();
}

std::string TermsToText(const std::vector<Signature::Term> &terms) {
    std::ostringstream result;

    for (size_t i = 0; i < terms.size(); ++i) {
        std::visit(
            [&](auto &&term) {
                using T = std::decay_t<decltype(term)>;

                if constexpr (std::is_same_v<T, Token>) {
                    result << term.text;
                } else if constexpr (std::is_same_v<T, Signature::Argument>) {
                    result << "{";
                    if (!term.targets.empty() && term.targets[0].name.has_value()) {
                        result << term.targets[0].name.value().text;
                    } else {
                        result << "value";
                    }
                    result << "}";
                }
            },
            terms[i]);

        if (i + 1 < terms.size()) {
            result << " ";
        }
    }

    return result.str();
}

std::string GetTextBeforeCursor(const std::string &content, const Position &position) {
    std::istringstream stream(content);
    std::string line;
    int currentLine = 0;

    while (std::getline(stream, line)) {
        if (currentLine == position.line) {
            // Convert character position to byte offset for UTF-8 strings
            size_t byteOffset = CharPosToByteOffset(line, position.character);
            return line.substr(0, std::min(byteOffset, line.size()));
        }
        currentLine++;
    }

    return "";
}

size_t FindPrefixLength(const std::string &textBeforeCursor, const std::string &completionText) {
    if (textBeforeCursor.empty()) {
        return 0;
    }

    // Split both into words
    auto splitWords = [](const std::string &text) -> std::vector<std::string> {
        std::vector<std::string> words;
        std::istringstream stream(text);
        std::string word;
        while (stream >> word) {
            words.push_back(word);
        }
        return words;
    };

    auto typedWords = splitWords(textBeforeCursor);
    auto completionWords = splitWords(completionText);

    if (typedWords.empty()) {
        return 0;
    }

    // Find longest matching prefix sequence
    size_t matchCount = 0;
    for (size_t i = 0; i < std::min(typedWords.size(), completionWords.size()); ++i) {
        if (typedWords[typedWords.size() - i - 1] == completionWords[i]) {
            matchCount = i + 1;
        } else {
            break;
        }
    }

    if (matchCount == 0) {
        return 0;
    }

    // Calculate character length to replace (UTF-8 character count, not bytes)
    size_t length = 0;
    for (size_t i = typedWords.size() - matchCount; i < typedWords.size(); ++i) {
        // Count UTF-8 characters, not bytes
        length += utf8::unchecked::distance(typedWords[i].begin(), typedWords[i].end());
        if (i + 1 < typedWords.size()) {
            length += 1; // Space between words
        }
    }

    // If text ends with whitespace, include it in the replacement
    if (!textBeforeCursor.empty() && std::isspace(textBeforeCursor.back())) {
        length += 1;
    }

    return length;
}

} // namespace lsp
SIF_NAMESPACE_END
