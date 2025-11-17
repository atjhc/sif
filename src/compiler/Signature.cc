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

#include "sif/compiler/Signature.h"
#include "sif/compiler/Parser.h"
#include "sif/compiler/Scanner.h"
#include "sif/runtime/ModuleLoader.h"
#include <sif/Utilities.h>

SIF_NAMESPACE_BEGIN

// The purpose of Name() is to return a normalized version of the Signature that
// may be compared for equality or used as a key.
static inline std::string Name(const Token &token) { return lowercase(token.text); }

static inline std::string Name(const Signature::Choice &choice) {
    std::vector<Token> tokens = choice.tokens;
    std::sort(tokens.begin(), tokens.end(),
              [](const Token &a, const Token &b) { return a.text < b.text; });
    return Join(tokens, "/");
}

static inline std::string Name(const Signature::Option &option) {
    return Concat("(", Name(option.choice), ")");
}

static inline std::string Name(const Signature::Argument &argument) { return "{}"; }

// The operator<< overloads are intended only for debugging purposes, and are meant to
// replicate how the Signature was originally parsed. No normalization is done.
static inline std::ostream &operator<<(std::ostream &out, const Token &token) {
    return out << Name(token);
}
static inline std::ostream &operator<<(std::ostream &out, const Signature::Choice &choice) {
    return out << Name(choice);
}

static inline std::ostream &operator<<(std::ostream &out, const Signature::Option &option) {
    return out << "(" << option.choice << ")";
}

static inline std::ostream &operator<<(std::ostream &out, const Signature::Argument &argument) {
    out << "{";

    auto it = argument.targets.begin();
    while (it != argument.targets.end()) {
        if (it->name.has_value()) {
            out << it->name.value();
        }
        if (it->typeName.has_value()) {
            out << ": " << it->typeName.value();
        }

        it++;
        if (it != argument.targets.end()) {
            out << ", ";
        }
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

bool Signature::isValid() const {
    bool hasAtLeastOneTokenTerm = false;
    for (auto &&term : terms) {
        if (std::holds_alternative<Token>(term) || std::holds_alternative<Choice>(term)) {
            hasAtLeastOneTokenTerm = true;
        }
    }
    return hasAtLeastOneTokenTerm;
}

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
            return true; // this is shorter, so it's "less than"
        if (i == signature.terms.size())
            return false; // other is shorter, so this is "greater than"

        // Compare indices first
        if (terms[i].index() != signature.terms[i].index()) {
            return terms[i].index() < signature.terms[i].index();
        }

        // Indices are equal, compare token text if both are tokens
        if (std::holds_alternative<Token>(terms[i]) &&
            std::holds_alternative<Token>(signature.terms[i])) {
            const auto &thisText = std::get<Token>(terms[i]).text;
            const auto &otherText = std::get<Token>(signature.terms[i]).text;
            if (thisText != otherText) {
                return thisText < otherText;
            }
        }

        // Everything equal so far, continue to next term
        i++;
    }
    return false; // all terms are equal
}

bool Signature::operator==(const Signature &signature) const { return name() == signature.name(); }

size_t Signature::Hash::operator()(const Signature &signature) const {
    return std::hash<std::string>{}(signature.name());
}

SIF_NAMESPACE_END
