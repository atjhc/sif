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
#include "compiler/Scanner.h"
#include "compiler/FunctionSignature.h"

#include <variant>
#include <vector>

CH_NAMESPACE_BEGIN

struct Call;
struct Binary;
struct Unary;
struct Grouping;
struct Variable;
struct RangeLiteral;
struct ListLiteral;
struct DictionaryLiteral;
struct Literal;

struct Expression : Node {
    struct Visitor {
        virtual void visit(const Call &) = 0;
        virtual void visit(const Binary &) = 0;
        virtual void visit(const Unary &) = 0;
        virtual void visit(const Grouping &) = 0;
        virtual void visit(const Variable &) = 0;
        virtual void visit(const RangeLiteral &) = 0;
        virtual void visit(const ListLiteral &) = 0;
        virtual void visit(const DictionaryLiteral &) = 0;
        virtual void visit(const Literal &) = 0;
    };

    virtual ~Expression() = default;

    virtual void accept(Visitor &v) const = 0;
};

struct Call : Expression {
    FunctionSignature signature;

    std::vector<Optional<Token>> tokens;
    std::vector<Owned<Expression>> arguments;

    Call(const FunctionSignature &signature, const std::vector<Optional<Token>> &tokens,
         std::vector<Owned<Expression>> arguments);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Binary : Expression {
    enum Operator {
        And,
        Or,
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
        Modulo,
        Exponent,
        Subscript
    };

    Owned<Expression> leftExpression;
    Operator binaryOperator;
    Owned<Expression> rightExpression;

    Binary(Owned<Expression> leftExpression, Operator binaryOperator,
           Owned<Expression> rightExpression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Unary : Expression {
    enum Operator { Minus, Not };

    Operator unaryOperator;
    Owned<Expression> expression;

    Unary(Operator unaryOperator, Owned<Expression> expression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Grouping : Expression {
    Owned<Expression> expression;

    Grouping(Owned<Expression> expression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct RangeLiteral : Expression {
    Owned<Expression> start;
    Owned<Expression> end;
    bool closed;

    RangeLiteral(Owned<Expression> start, Owned<Expression> end, bool closed);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct ListLiteral : Expression {
    std::vector<Owned<Expression>> expressions;

    ListLiteral(std::vector<Owned<Expression>> expressions);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct DictionaryLiteral : Expression {
    Map<Owned<Expression>, Owned<Expression>> values;

    DictionaryLiteral(Map<Owned<Expression>, Owned<Expression>>);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Variable : Expression {
    Token token;
    Optional<Token> typeName;

    Variable(const Token &token, Optional<Token> typeName = None);
    
    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Literal : Expression {
    Token token;

    Literal(Token token);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

CH_NAMESPACE_END
