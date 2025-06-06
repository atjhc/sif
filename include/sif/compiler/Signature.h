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

#include <sif/Common.h>
#include <sif/compiler/Scanner.h>

#include <variant>
#include <vector>

SIF_NAMESPACE_BEGIN

struct Signature {
    struct Argument {
        struct Target {
            Optional<Token> name;
            Optional<Token> typeName;
        };
        std::vector<Target> targets;
    };
    struct Choice {
        std::vector<Token> tokens;
    };
    struct Option {
        Choice choice;
    };
    using Term = std::variant<Token, Choice, Argument, Option>;

    struct Hash {
        size_t operator()(const Signature &) const;
    };

    static Result<Signature, Error> Make(const std::string &format);

    std::string name() const;
    std::string description() const;
    bool endsWithArgument() const;
    bool isValid() const;
    std::vector<Signature::Argument> arguments() const;

    bool operator<(const Signature &signature) const;
    bool operator==(const Signature &signature) const;

    std::vector<Term> terms;
    Optional<Token> typeName;
};

SIF_NAMESPACE_END
