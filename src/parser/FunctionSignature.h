//
//  Copyright (c) 2021 James Callender
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
#include "parser/Scanner.h"

#include <vector>
#include <variant>

CH_NAMESPACE_BEGIN

struct FunctionSignature {
    struct Argument {
        Optional<Token> token;
        Optional<Token> typeName;
    };
    struct Choice {
        std::vector<Token> tokens;
    };
    struct Option {
        Token token;
    };
    using Term = std::variant<Token, Argument, Choice, Option>;

    static FunctionSignature Make(const std::string &format);

    size_t arity() const;
    std::string name() const;
    std::string description() const;

    bool operator<(const FunctionSignature &signature) const;

    std::vector<Term> terms;
};

CH_NAMESPACE_END
