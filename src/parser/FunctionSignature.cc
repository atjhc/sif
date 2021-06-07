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
#include "parser/Scanner.h"
#include "parser/Parser.h"
#include "Utilities.h"

CH_NAMESPACE_BEGIN

static inline std::string Name(const Token &token) {
    return lowercase(token.text);
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
    return Concat("(", lowercase(option.token.text), ")");
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

FunctionSignature FunctionSignature::Make(const std::string &format) {
    Scanner scanner(format.c_str(), format.c_str() + format.length());
    ParserConfig config;
    config.disableNatives = true;
    Parser parser(config, scanner);
    return parser.parseFunctionSignature();
}

size_t FunctionSignature::arity() const {
    size_t arity = 0;
    for (const auto &term : terms) {
        if (std::holds_alternative<FunctionSignature::Argument>(term)) {
            arity++;
        }
    }
    return arity;
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
    std::ostringstream ss;
    ss << Join(terms, " ");
    if (typeName.has_value()) {
        ss << " -> " << typeName.value();
    }
    return ss.str();
}

bool FunctionSignature::operator<(const FunctionSignature &signature) const {
    int i = 0;
    while (i < terms.size() || i < signature.terms.size()) {
        if (i == terms.size()) return false;
        if (i == signature.terms.size()) return true;
        if (terms[i].index() < signature.terms[i].index()) {
            return true;
        }
        if (std::holds_alternative<Token>(terms[i]) && std::holds_alternative<Token>(signature.terms[i])) {
            if (std::get<Token>(terms[i]).text < std::get<Token>(signature.terms[i]).text) {
                return true;
            }
        }
        i++;
    }
    return false;
}

bool FunctionSignature::operator==(const FunctionSignature &signature) const {
    return name() == signature.name();
}

CH_NAMESPACE_END
