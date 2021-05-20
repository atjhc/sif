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
#include "parser/Scanner.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct Binary;
struct Unary;
struct Grouping;
struct Variable;
struct List;
struct Literal;

struct Expression : Node {
    struct AnyVisitor {
        virtual std::any visitAny(const Binary &) = 0;
        virtual std::any visitAny(const Unary &) = 0;
        virtual std::any visitAny(const Grouping &) = 0;
        virtual std::any visitAny(const Variable &) = 0;
        virtual std::any visitAny(const List &) = 0;
        virtual std::any visitAny(const Literal &) = 0;
    };

    template <typename T>
    struct Visitor : AnyVisitor {
        std::any visitAny(const Binary &e) { return visit(e); }
        std::any visitAny(const Unary &e) { return visit(e); }
        std::any visitAny(const Grouping &e) { return visit(e); }
        std::any visitAny(const Variable &e) { return visit(e); }
        std::any visitAny(const List &e) { return visit(e); }
        std::any visitAny(const Literal &e) { return visit(e); }

        virtual T visit(const Binary &) = 0;
        virtual T visit(const Unary &) = 0;
        virtual T visit(const Grouping &) = 0;
        virtual T visit(const Variable &) = 0;
        virtual T visit(const List &) = 0;
        virtual T visit(const Literal &) = 0;
    };

    struct VoidVisitor : AnyVisitor {
        std::any visitAny(const Binary &e) { visit(e); return std::any(); }
        std::any visitAny(const Unary &e) { visit(e); return std::any(); }
        std::any visitAny(const Grouping &e) { visit(e); return std::any(); }
        std::any visitAny(const Variable &e) { visit(e); return std::any(); }
        std::any visitAny(const List &e) { visit(e); return std::any(); }
        std::any visitAny(const Literal &e) { visit(e); return std::any(); }

        virtual void visit(const Binary &) = 0;
        virtual void visit(const Unary &) = 0;
        virtual void visit(const Grouping &) = 0;
        virtual void visit(const Variable &) = 0;
        virtual void visit(const List &) = 0;
        virtual void visit(const Literal &) = 0;
    };

    virtual ~Expression() = default;

	template <typename T>
	T accept(Visitor<T> &v) const {
		return std::any_cast<T>(acceptAny(v));
	}

	void accept(VoidVisitor &v) const {
		acceptAny(v);
	}

    virtual std::any acceptAny(AnyVisitor &v) const = 0;
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
        Mod,
        Exponent,
    };

    Owned<Expression> leftExpression;
    Operator binaryOperator;
    Owned<Expression> rightExpression;

    Binary(Owned<Expression> leftExpression, Operator binaryOperator,
           Owned<Expression> rightExpression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Unary : Expression {
    enum Operator { ThereIsA, Minus, Not };

    Operator unaryOperator;
    Owned<Expression> expression;

    Unary(Operator unaryOperator, Owned<Expression> expression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Grouping : Expression {
    Owned<Expression> expression;

    Grouping(Owned<Expression> expression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct List : Expression {
    std::vector<Owned<Expression>> expressions;

    List(std::vector<Owned<Expression>> expressions);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Variable : Expression {
    std::vector<Token> tokens;

    Variable(const std::vector<Token> tokens);
    
    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Literal : Expression {
    Token token;

    Literal(Token token);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
