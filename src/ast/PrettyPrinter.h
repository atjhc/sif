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
#include "ast/Statement.h"
#include "ast/Expression.h"
#include "ast/Chunk.h"

#include <iostream>

CH_AST_NAMESPACE_BEGIN

struct Program;
struct Handler;
struct StatementList;
struct IdentifierList;
struct Container;

struct PrettyPrinterConfig {
    std::ostream &out = std::cout;
    unsigned int tabSize = 2;
};

class PrettyPrinter : public Statement::Visitor, public Expression::VoidVisitor, public Chunk::VoidVisitor {
  public:
    PrettyPrinter(const PrettyPrinterConfig &config = PrettyPrinterConfig());

    void print(const Program &);

  private:
    void print(const Handler &);
    void print(const StatementList &);
    void print(const IdentifierList &, const std::string &sep);
    void print(const ExpressionList &);

#pragma mark - Statement::Visitor

    void visit(const If &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCount &) override;
    void visit(const RepeatRange &) override;
    void visit(const RepeatCondition &) override;
    void visit(const ExitRepeat &) override;
    void visit(const NextRepeat &) override;
    void visit(const Exit &) override;
    void visit(const Pass &) override;
    void visit(const Global &) override;
    void visit(const Return &) override;
    void visit(const Do &) override;
    void visit(const Command &) override;
    void visit(const Put &) override;
    void visit(const Get &) override;
    void visit(const Set &) override;
    void visit(const Ask &) override;
    void visit(const Add &) override;
    void visit(const Subtract &) override;
    void visit(const Multiply &) override;
    void visit(const Divide &) override;
    void visit(const Delete &) override;

#pragma mark - Expression::VoidVisitor

    void visit(const Identifier &) override;
    void visit(const FunctionCall &) override;
    void visit(const Property &) override;
    void visit(const Descriptor &) override;
    void visit(const Binary &) override;
    void visit(const Logical &e) override;
    void visit(const Unary &e) override;
    void visit(const FloatLiteral &) override;
    void visit(const IntLiteral &) override;
    void visit(const StringLiteral &) override;
    void visit(const ChunkExpression &) override;

#pragma mark - Chunk::VoidVisitor

    void visit(const RangeChunk &) override;
    void visit(const AnyChunk &) override;
    void visit(const LastChunk &) override;
    void visit(const MiddleChunk &) override;

  private:
    PrettyPrinterConfig _config = PrettyPrinterConfig();
    unsigned int _indentLevel = 0;
    std::ostream &out;

    std::string indentString() { return std::string(_indentLevel * _config.tabSize, ' '); }
};

CH_AST_NAMESPACE_END
