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

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct Identifier;
struct IdentifierList;

struct Expression : Node {
    virtual ~Expression() = default;

    virtual std::any accept(AnyVisitor &v) const = 0;
};

struct ExpressionList : Node {
    std::vector<Owned<Expression>> expressions;

    ExpressionList();
    ExpressionList(Owned<Expression> &e);

    void add(Owned<Expression> &e) {
        expressions.push_back(std::move(e));
    }

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct FunctionCall : Expression {
    Owned<Identifier> identifier;
    Owned<ExpressionList> arguments;

    FunctionCall(Owned<Identifier> &id, Owned<ExpressionList> &args);
    FunctionCall(Owned<Identifier> &id, Owned<Expression> &arg);
    FunctionCall(Owned<Identifier> &id);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Binary : Expression {
    enum Operator {
        IsA,
        Equal,
        NotEqual,
        LessThan,
        GreaterThan,
        LessThanOrEqual,
        GreaterThanOrEqual,
        Plus,
        Minus,
        Multiply,
        Divide,
        Mod,
        Exponent,
        IsIn,
        Contains,
        Concat,
        ConcatWithSpace
    };

    Operator binaryOperator;
    Owned<Expression> leftExpression, rightExpression;

    Binary(Operator binaryOperator, Owned<Expression> &leftExpression, Owned<Expression> &rightExpression);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Logical : Expression {
    enum Operator {
        And,
        Or
    };

    Operator logicalOperator;
    Owned<Expression> leftExpression, rightExpression;

    Logical(Operator logicalOperator, Owned<Expression> &leftExpression, Owned<Expression> &rightExpression);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Unary : Expression {
    enum Operator {
        ThereIsA,
        Minus,
        Not
    };

    Operator unaryOperator;
    Owned<Expression> expression;

    Unary(Operator unaryOperator, Owned<Expression> &expression);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct FloatLiteral : Expression {
    double value;

    FloatLiteral(double v);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int i);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct StringLiteral : Expression {
    std::string value;

    StringLiteral(const std::string &v);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
