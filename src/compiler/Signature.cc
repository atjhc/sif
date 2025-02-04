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

#include "compiler/Signature.h"
#include "Utilities.h"
#include "compiler/Parser.h"
#include "compiler/Scanner.h"
#include "runtime/ModuleLoader.h"

SIF_NAMESPACE_BEGIN

static inline std::string Name(const Token &token) { return lowercase(token.text); }

static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Name(token);
}

static inline std::string Name(const Signature::Choice &choice) { return Join(choice.tokens, "/"); }

static inline std::ostream &operator<<(std::ostream &out, const Signature::Choice &choice) {
    return out << Name(choice);
}

static inline std::string Name(const Signature::Option &option) {
    return Concat("(", lowercase(option.token.text), ")");
}

static inline std::ostream &operator<<(std::ostream &out, const Signature::Option &option) {
    return out << Name(option);
}

static inline std::string Name(const Signature::Argument &argument) { return "{}"; }

static inline std::ostream &operator<<(std::ostream &out, const Signature::Argument &argument) {
    out << "{";
    if (argument.token.has_value()) {
        out << argument.token.value();
    }
    out << ":";
    if (argument.typeName.has_value()) {
        out << " " << argument.typeName.value();
    }
    out << "}";
    return out;
}

static inline std::ostream &operator<<(std::ostream &out, const Signature::Term &term) {
    std::visit([&](auto &&arg) { out << arg; }, term);
    return out;
}

Result<Signature, Error> Signature::Make(const std::string &format) {
    auto scanner = Scanner();
    auto reader = StringReader(format);
    auto loader = ModuleLoader();
    auto reporter = CaptureReporter();
    ParserConfig config{scanner, reader, loader, reporter};
    Parser parser(config);
    auto signature = parser.signature();
    if (!signature) {
        return Fail(reporter.errors()[0]);
    }
    return signature.value();
}

std::string Signature::name() const {
    std::ostringstream ss;
    auto it = terms.begin();
    while (it != terms.end()) {
        std::visit([&](auto &&arg) { ss << Name(arg); }, *it);
        it++;
        if (it != terms.end()) {
            ss << " ";
        }
    }
    return ss.str();
}

std::string Signature::description() const {
    std::ostringstream ss;
    ss << Join(terms, " ");
    if (typeName.has_value()) {
        ss << " -> " << typeName.value();
    }
    return ss.str();
}

bool Signature::endsWithArgument() const { return std::holds_alternative<Argument>(terms.back()); }

std::vector<Signature::Argument> Signature::arguments() const {
    std::vector<Argument> result;
    for (auto &&term : terms) {
        if (std::holds_alternative<Argument>(term)) {
            result.push_back(std::get<Argument>(term));
        }
    }
    return result;
}

bool Signature::operator<(const Signature &signature) const {
    int i = 0;
    while (i < terms.size() || i < signature.terms.size()) {
        if (i == terms.size())
            return false;
        if (i == signature.terms.size())
            return true;
        if (terms[i].index() < signature.terms[i].index()) {
            return true;
        }
        if (std::holds_alternative<Token>(terms[i]) &&
            std::holds_alternative<Token>(signature.terms[i])) {
            if (std::get<Token>(terms[i]).text < std::get<Token>(signature.terms[i]).text) {
                return true;
            }
        }
        i++;
    }
    return false;
}

bool Signature::operator==(const Signature &signature) const { return name() == signature.name(); }

size_t Signature::Hash::operator()(const Signature &signature) const {
    return std::hash<std::string>{}(signature.name());
}

SIF_NAMESPACE_END
