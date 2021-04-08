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
#include "ast/Node.h"

#include <iostream>

CH_AST_NAMESPACE_BEGIN

struct PrettyPrinterConfig {
    std::ostream &out = std::cout;
    unsigned int tabSize = 2;
};

class PrettyPrinter: AnyVisitor {
  public:
    PrettyPrinter(const PrettyPrinterConfig &config = PrettyPrinterConfig());

    void print(const Node &node);

  private:
    std::any visitAny(const Script &) override;
    std::any visitAny(const Handler &) override;
    std::any visitAny(const StatementList &) override;
    std::any visitAny(const IdentifierList &) override;
    std::any visitAny(const ExpressionList &) override;
    std::any visitAny(const If &) override;
    std::any visitAny(const Repeat &) override;
    std::any visitAny(const RepeatCount &) override;
    std::any visitAny(const RepeatRange &) override;
    std::any visitAny(const RepeatCondition &) override;
    std::any visitAny(const ExitRepeat &) override;
    std::any visitAny(const NextRepeat &) override;
    std::any visitAny(const Exit &) override;
    std::any visitAny(const Pass &) override;
    std::any visitAny(const Global &) override;
    std::any visitAny(const Return &) override;
    std::any visitAny(const Do &) override;
    std::any visitAny(const Preposition &) override;
    std::any visitAny(const Identifier &) override;
    std::any visitAny(const FunctionCall &) override;
    std::any visitAny(const Property &) override;
    std::any visitAny(const Descriptor &) override;
    std::any visitAny(const BinaryOp &) override;
    std::any visitAny(const Not &) override;
    std::any visitAny(const Minus &) override;
    std::any visitAny(const FloatLiteral &) override;
    std::any visitAny(const IntLiteral &) override;
    std::any visitAny(const StringLiteral &) override;
    std::any visitAny(const RangeChunk &) override;
    std::any visitAny(const AnyChunk &) override;
    std::any visitAny(const LastChunk &) override;
    std::any visitAny(const MiddleChunk &) override;
    std::any visitAny(const Command &) override;
    std::any visitAny(const Put &) override;
    std::any visitAny(const Get &) override;
    std::any visitAny(const Ask &) override;
    std::any visitAny(const Add &) override;
    std::any visitAny(const Subtract &) override;
    std::any visitAny(const Multiply &) override;
    std::any visitAny(const Divide &) override;

  private:
    PrettyPrinterConfig _config = PrettyPrinterConfig();
    unsigned int _indentLevel = 0;
    std::ostream &out;

    std::string indentString() { return std::string(_indentLevel * _config.tabSize, ' '); }
};

CH_AST_NAMESPACE_END
