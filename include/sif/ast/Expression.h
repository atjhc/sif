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

#include <sif/Common.h>
#include <sif/ast/Node.h>
#include <sif/compiler/Scanner.h>
#include <sif/compiler/Signature.h>

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
struct StringInterpolation;

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
        virtual void visit(const StringInterpolation &) = 0;
    };

    virtual ~Expression() = default;

    virtual void accept(Visitor &v) const = 0;
};

struct Call : Expression {
    Signature signature;
    std::vector<Strong<Expression>> arguments;

    std::vector<SourceRange> ranges;

    Call(const Signature &signature, std::vector<Strong<Expression>> arguments);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
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

    Binary() {}
    Binary(Strong<Expression> leftExpression, Operator binaryOperator,
           Strong<Expression> rightExpression);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct Unary : Expression {
    enum Operator { Minus, Not };

    Operator unaryOperator;
    Strong<Expression> expression;

    struct {
        SourceRange operator_;
    } ranges;

    Unary(Operator unaryOperator) : unaryOperator(unaryOperator) {}
    Unary(Operator unaryOperator, Strong<Expression> expression);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct Grouping : Expression {
    Strong<Expression> expression;

    struct {
        SourceRange leftGrouping;
        Optional<SourceRange> rightGrouping;
    } ranges;

    Grouping() {}
    Grouping(Strong<Expression> expression);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct RangeLiteral : Expression {
    Strong<Expression> start;
    Strong<Expression> end;
    bool closed;

    struct {
        SourceRange operator_;
    } ranges;

    RangeLiteral() {}
    RangeLiteral(Strong<Expression> start, Strong<Expression> end, bool closed);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct ListLiteral : Expression {
    std::vector<Strong<Expression>> expressions;

    struct {
        Optional<SourceRange> leftBracket;
        Optional<SourceRange> rightBracket;
        std::vector<SourceRange> commas;
    } ranges;

    ListLiteral(std::vector<Strong<Expression>> expressions = {});

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct DictionaryLiteral : Expression {
    std::vector<std::pair<Strong<Expression>, Strong<Expression>>> values;

    struct {
        SourceRange leftBracket;
        SourceRange rightBracket;
        std::vector<SourceRange> colons;
        std::vector<SourceRange> commas;
    } ranges;

    DictionaryLiteral(std::vector<std::pair<Strong<Expression>, Strong<Expression>>> = {});

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct Variable : Expression {
    enum Scope { Local, Global };

    Optional<Token> name;
    Optional<Scope> scope;

    struct {
        Optional<SourceRange> scope;
    } ranges;

    Variable() {}
    Variable(const Optional<Token> &name, Optional<Scope> scope = None);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct Literal : Expression {
    Token token;

    Literal(Token token);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

struct StringInterpolation : Expression {
    Token left;
    Strong<Expression> expression;
    Strong<Expression> right;

    StringInterpolation() {}
    StringInterpolation(Token leftPart, Strong<Expression> expression,
                        Strong<Expression> rightPart = nullptr);

    void accept(Expression::Visitor &v) const override { v.visit(*this); }
};

SIF_NAMESPACE_END
