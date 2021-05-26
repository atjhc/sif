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

#include "parser/FunctionSignature.h"

CH_NAMESPACE_BEGIN

static inline std::string Name(const Token &token) {
    return token.text;
}

static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Name(token);
}

static inline std::string Name(const FunctionSignature::Choice &choice) {
    return Concat("(", Join(choice.tokens, "/"), ")");
}

static inline std::ostream &operator<<(std::ostream &out, const FunctionSignature::Choice &choice) {
    return out << Name(choice);
}

static inline std::string Name(const FunctionSignature::Option &option) {
    return Concat("(", option.token.text, ")");
}

static inline std::ostream &operator<<(std::ostream &out, const FunctionSignature::Option &option) {
    return out << Name(option);
}

static inline std::string Name(const FunctionSignature::Argument &argument) {
    return "(:)";
}

static inline std::ostream &operator<<(std::ostream &out, const FunctionSignature::Argument &argument) {
    out << "(";
    if (argument.token.has_value()) {
        out << argument.token.value();
    }
    out << ":";
    if (argument.typeName.has_value()) {
        out << " " << argument.typeName.value();
    }
    out << ")";
    return out;
}

static inline std::ostream &operator<<(std::ostream &out, const FunctionSignature::Term &term) {
    std::visit([&](auto && arg){ out << arg;}, term);
    return out;
}

std::string FunctionSignature::name() const {
    std::ostringstream ss;
    auto it = terms.begin();
    while (it != terms.end()) {
        std::visit([&](auto && arg){ ss << Name(arg); }, *it);
        it++;
        if (it != terms.end()) {
            ss << " ";
        }
    }
    return ss.str();
}

std::string FunctionSignature::description() const {
    return Join(terms, " ");
}

CH_NAMESPACE_END
