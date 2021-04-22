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
#include "ast/Expression.h"
#include "ast/Node.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

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
struct Do;
struct Command;
struct Put;
struct Get;
struct Ask;
struct Add;
struct Subtract;
struct Multiply;
struct Divide;
struct Delete;

struct Statement : Node {
    struct Visitor {
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
        virtual void visit(const Do &) = 0;
        virtual void visit(const Command &) = 0;
        virtual void visit(const Put &) = 0;
        virtual void visit(const Get &) = 0;
        virtual void visit(const Ask &) = 0;
        virtual void visit(const Add &) = 0;
        virtual void visit(const Subtract &) = 0;
        virtual void visit(const Multiply &) = 0;
        virtual void visit(const Divide &) = 0;
        virtual void visit(const Delete &) = 0;
    };

    virtual ~Statement() = default;

    virtual void accept(Visitor &v) const = 0;
};

struct StatementList : Node {
    std::vector<Owned<Statement>> statements;

    StatementList();
    StatementList(Owned<Statement> &statement);

    void add(Owned<Statement> &statement) { statements.push_back(std::move(statement)); }
};

struct If : Statement {
    Owned<Expression> condition;
    Owned<StatementList> ifStatements;
    Owned<StatementList> elseStatements;

    If(Owned<Expression> &c, Owned<Statement> &is);
    If(Owned<Expression> &c, Owned<Statement> &is, Owned<Statement> &es);
    If(Owned<Expression> &c, Owned<StatementList> &isl);
    If(Owned<Expression> &c, Owned<StatementList> &is, Owned<StatementList> &es);
    If(Owned<Expression> &c, Owned<Statement> &is, Owned<StatementList> &esl);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Exit : Statement {
    Owned<Identifier> messageKey;

    Exit(Owned<Identifier> &m);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Pass : Statement {
    Owned<Identifier> messageKey;

    Pass(Owned<Identifier> &m);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Global : Statement {
    Owned<IdentifierList> variables;

    Global(Owned<IdentifierList> &v);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Return : Statement {
    Owned<Expression> expression;

    Return(Owned<Expression> &e);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Do : Statement {
    Owned<Expression> expression;
    Owned<Expression> language;

    Do(Owned<Expression> &expression);
    Do(Owned<Expression> &expression, Owned<Expression> &language);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

CH_AST_NAMESPACE_END
