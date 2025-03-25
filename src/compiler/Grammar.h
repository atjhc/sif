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

#include "Common.h"
#include "compiler/Signature.h"

#include <vector>

SIF_NAMESPACE_BEGIN

struct Grammar {
    Owned<Grammar> argument;
    Mapping<std::string, Owned<Grammar>> terms;

    Optional<Signature> signature;

    template <class InputIt> bool insert(InputIt first, InputIt last) {
        bool result = true;
        auto it = first;
        while (it != last) {
            if (!insert(*it++)) {
                result = false;
            }
        }
        return result;
    }
    bool insert(const Signature &signature) { return insert(signature, signature.terms.cbegin()); }

    std::vector<Signature> allSignatures() const;
    bool isLeaf() const { return argument == nullptr && terms.size() == 0; }

  private:
    bool insert(const Signature &signature, std::vector<Signature::Term>::const_iterator term);
};

SIF_NAMESPACE_END
