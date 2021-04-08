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

struct BinaryOp : Expression {
    enum Operator {
        Equal,
        NotEqual,
        LessThan,
        GreaterThan,
        LessThanOrEqual,
        GreaterThanOrEqual,
        IsIn,
        IsAn,
        Contains,
        Or,
        And,
        Plus,
        Minus,
        Multiply,
        Divide,
        Mod,
        Exponent,
        Concat,
        ConcatWithSpace
    };

    Operator op;
    Owned<Expression> left, right;

    BinaryOp(Operator op, Owned<Expression> &left, Owned<Expression> &right);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Not : Expression {
    Owned<Expression> expression;

    Not(Owned<Expression> &expression);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Minus : Expression {
    Owned<Expression> expression;

    Minus(Owned<Expression> &expression);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct FloatLiteral : Expression {
    float value;

    FloatLiteral(float v);

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
