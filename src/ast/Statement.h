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

    If(Owned<Expression> &c, Owned<StatementList> &is, Owned<StatementList> &es)
        : condition(std::move(c)), ifStatements(std::move(is)), elseStatements(std::move(es)) {}

    If(Owned<Expression> &c, Owned<StatementList> &isl)
        : condition(std::move(c)), ifStatements(std::move(isl)), elseStatements(nullptr) {}

    If(Owned<Expression> &c, Owned<Statement> &is)
        : condition(std::move(c)), ifStatements(MakeOwned<StatementList>(is)), elseStatements(nullptr) {}

    If(Owned<Expression> &c, Owned<Statement> &is, Owned<StatementList> &esl)
        : condition(std::move(c)), ifStatements(MakeOwned<StatementList>(is)), elseStatements(std::move(esl)) {}


    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Exit : Statement {
    Owned<Identifier> messageKey;

    Exit(Owned<Identifier> &m) : messageKey(std::move(m)) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Pass : Statement {
    Owned<Identifier> messageKey;

    Pass(Owned<Identifier> &m) : messageKey(std::move(m)) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Global : Statement {
    Owned<IdentifierList> variables;

    Global(Owned<IdentifierList> &v) : variables(std::move(v)) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Return : Statement {
    Owned<Expression> expression;

    Return(Owned<Expression> &e) : expression(std::move(e)) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
