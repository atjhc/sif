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
#include "compiler/Signature.h"

#include <variant>
#include <vector>

SIF_NAMESPACE_BEGIN

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
    Signature signature;
    std::vector<Strong<Expression>> arguments;

    std::vector<SourceRange> ranges;

    Call(const Signature &signature, std::vector<Strong<Expression>> arguments);

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

    Strong<Expression> leftExpression;
    Operator binaryOperator;
    Strong<Expression> rightExpression;

    struct {
        SourceRange operator_;
    } ranges;

    Binary(Strong<Expression> leftExpression, Operator binaryOperator,
           Strong<Expression> rightExpression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Unary : Expression {
    enum Operator { Minus, Not };

    Operator unaryOperator;
    Strong<Expression> expression;

    struct {
        SourceRange operator_;
    } ranges;

    Unary(Operator unaryOperator, Strong<Expression> expression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Grouping : Expression {
    Strong<Expression> expression;

    struct {
        SourceRange leftGrouping;
        SourceRange rightGrouping;
    } ranges;

    Grouping(Strong<Expression> expression);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct RangeLiteral : Expression {
    Strong<Expression> start;
    Strong<Expression> end;
    bool closed;

    struct {
        SourceRange operator_;
    } ranges;

    RangeLiteral(Strong<Expression> start, Strong<Expression> end, bool closed);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct ListLiteral : Expression {
    std::vector<Strong<Expression>> expressions;

    struct {
        Optional<SourceRange> leftGrouping;
        Optional<SourceRange> rightGrouping;
        std::vector<SourceRange> commas;
    } ranges;

    ListLiteral(std::vector<Strong<Expression>> expressions = {});

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct DictionaryLiteral : Expression {
    Mapping<Strong<Expression>, Strong<Expression>> values;

    struct {
        SourceRange leftBrace;
        SourceRange rightBrace;
        std::vector<SourceRange> commas;
    } ranges;

    DictionaryLiteral(Mapping<Strong<Expression>, Strong<Expression>> = {});

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Variable : Expression {
    enum Scope { Local, Global };

    Token name;
    Optional<Scope> scope;

    struct {
        Optional<SourceRange> scope;
    } ranges;

    Variable(const Token &name, Optional<Scope> scope = None);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

struct Literal : Expression {
    Token token;

    Literal(Token token);

    void accept(Expression::Visitor &v) const override { return v.visit(*this); }
};

SIF_NAMESPACE_END
