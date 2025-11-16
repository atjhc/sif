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

#include "sif/compiler/Grammar.h"
#include <sif/Utilities.h>

#include <algorithm>
#include <set>
#include <stack>
#include <vector>

SIF_NAMESPACE_BEGIN

bool Grammar::insert(const Signature &signature,
                     std::vector<Signature::Term>::const_iterator term) {
    if (term == signature.terms.end()) {
        if (this->signature) {
            return false;
        }
        this->signature = signature;
        return true;
    }

    bool result = true;
    auto insertToken = [&](Token token) {
        auto word = lowercase(token.text);
        auto it = terms.find(word);
        if (it == terms.end()) {
            auto grammar = MakeStrong<Grammar>();
            grammar->insert(signature, std::next(term));
            terms[word] = std::move(grammar);
            return;
        }
        if (!it->second->insert(signature, std::next(term))) {
            result = false;
        }
    };
    auto insertArgument = [&](Signature::Argument) {
        if (argument) {
            if (!argument->insert(signature, std::next(term))) {
                result = false;
            }
            return;
        }
        argument = MakeStrong<Grammar>();
        argument->insert(signature, std::next(term));
    };
    auto insertChoice = [&](Signature::Choice choice) {
        for (auto &token : choice.tokens) {
            insertToken(token);
        }
    };
    std::visit(
        Overload{
            insertToken,
            insertArgument,
            insertChoice,
            [&](Signature::Option option) {
                insertChoice(option.choice);
                if (!insert(signature, std::next(term))) {
                    result = false;
                }
            },
        },
        *term);
    return result;
}

std::vector<Signature> Grammar::allSignatures() const {
    auto grammar = this;

    std::stack<Grammar *> grammars;
    std::set<Signature> signatures;
    do {
        if (grammar->argument) {
            grammars.push(grammar->argument.get());
        }
        // Sort keys to ensure deterministic iteration order across different STL implementations
        const auto &terms = grammar->terms;
        std::vector<std::string> sortedKeys;
        for (auto &&pair : terms) {
            sortedKeys.push_back(pair.first);
        }
        std::sort(sortedKeys.begin(), sortedKeys.end());
        for (auto &&key : sortedKeys) {
            grammars.push(terms.at(key).get());
        }
        grammar = nullptr;
        if (grammars.size() > 0) {
            grammar = grammars.top();
            grammars.pop();

            if (grammar->signature && grammar->signature->terms.size() > 0) {
                signatures.insert(grammar->signature.value());
            }
        }
    } while (grammar);

    return std::vector<Signature>(signatures.begin(), signatures.end());
}

SIF_NAMESPACE_END
