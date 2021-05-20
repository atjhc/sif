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

struct Block;
struct Set;
struct If;
struct Repeat;
struct RepeatCount;
struct RepeatRange;
struct RepeatCondition;
struct ExitRepeat;
struct NextRepeat;
struct Return;
struct ExpressionStatement;

struct Statement : Node {
    struct Visitor {
        virtual void visit(const Block &) = 0;
        virtual void visit(const Set &) = 0;
        virtual void visit(const If &) = 0;
        virtual void visit(const Repeat &) = 0;
        // virtual void visit(const RepeatCount &) = 0;
        // virtual void visit(const RepeatRange &) = 0;
        virtual void visit(const RepeatCondition &) = 0;
        virtual void visit(const ExitRepeat &) = 0;
        virtual void visit(const NextRepeat &) = 0;
        virtual void visit(const Return &) = 0;
        virtual void visit(const ExpressionStatement &) = 0;
    };

    virtual ~Statement() = default;

    virtual void accept(Visitor &v) const = 0;
};

struct Block : Statement {
    std::vector<Owned<Statement>> statements;

    Block(std::vector<Owned<Statement>> statements);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Set : Statement {
    Owned<Variable> variable;
    Owned<Expression> expression;

    Set(Owned<Variable> variable, Owned<Expression> expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct If : Statement {
    Owned<Expression> condition;
    Owned<Statement> ifStatement;
    Owned<Statement> elseStatement;

    If(Owned<Expression> condition, Owned<Statement> ifStatement, Owned<Statement> elseStatement);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Return : Statement {
    Owned<Expression> expression;

    Return(Owned<Expression> expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct ExpressionStatement : Statement {
    Owned<Expression> expression;

    ExpressionStatement(Owned<Expression> expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

CH_AST_NAMESPACE_END
