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
#include <sif/compiler/Signature.h>
#include <sif/lsp/Protocol.h>
#include <string>
#include <vector>

SIF_NAMESPACE_BEGIN
namespace lsp {

// Generate all valid variations of a signature by expanding Options and Choices
std::vector<std::vector<Signature::Term>>
GenerateSignatureVariations(const std::vector<Signature::Term> &terms);

// Convert a specific variation of terms to an LSP snippet with tab stops
std::string TermsToSnippet(const std::vector<Signature::Term> &terms);

// Convert terms to display text (without tab stops)
std::string TermsToText(const std::vector<Signature::Term> &terms);

// Extract the line text before cursor position
std::string GetTextBeforeCursor(const std::string &content, const Position &position);

// Find the length of the prefix that should be replaced (matches on word boundaries)
size_t FindPrefixLength(const std::string &textBeforeCursor, const std::string &completionText);

} // namespace lsp
SIF_NAMESPACE_END
