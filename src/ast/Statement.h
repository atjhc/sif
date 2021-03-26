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

#include "ast/Base.h"
#include "ast/Expression.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct StatementList;
struct Identifier;
struct IdentifierList;
struct Expression;
struct Preposition;
struct If;
struct Repeat;
struct RepeatCount;
struct RepeatRange;
struct RepeatCondition;
struct ExitRepeat;
struct NextRepeat;
struct Exit;
struct Pass;
struct Global;
struct Return;
struct Put;
struct Get;
struct Ask;
struct Command;

class StatementVisitor {
  public:
    virtual void visit(const If &) = 0;
    virtual void visit(const Repeat &) = 0;
    virtual void visit(const RepeatCount &) = 0;
    virtual void visit(const RepeatRange &) = 0;
    virtual void visit(const RepeatCondition &) = 0;
    virtual void visit(const ExitRepeat &) = 0;
    virtual void visit(const NextRepeat &) = 0;
    virtual void visit(const Exit &) = 0;
    virtual void visit(const Pass &) = 0;
    virtual void visit(const Global &) = 0;
    virtual void visit(const Return &) = 0;
    virtual void visit(const Command &) = 0;
};

struct Statement : Node {
    virtual ~Statement() = default;
    virtual void accept(StatementVisitor &visitor) const = 0;
};

struct If : Statement {
    Owned<Expression> condition;
    Owned<StatementList> ifStatements;
    Owned<StatementList> elseStatements;

    If(Expression *_condition, StatementList *_ifStatements, StatementList *_elseStatements)
        : condition(_condition), ifStatements(_ifStatements), elseStatements(_elseStatements) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Exit : Statement {
    Owned<Identifier> messageKey;

    Exit(Identifier *_messageKey) : messageKey(_messageKey) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Pass : Statement {
    Owned<Identifier> messageKey;

    Pass(Identifier *_messageKey) : messageKey(_messageKey) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Global : Statement {
    Owned<IdentifierList> variables;

    Global(IdentifierList *_variables) : variables(_variables) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Return : Statement {
    Owned<Expression> expression;

    Return(Expression *_expression) : expression(_expression) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
