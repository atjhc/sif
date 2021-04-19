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

struct IdentifierList;
struct Identifier;
struct FunctionCall;
struct Property;
struct Descriptor;
struct Binary;
struct Logical;
struct Unary;
struct ThereIsA;
struct Not;
struct Minus;
struct RangeChunk;
struct AnyChunk;
struct LastChunk;
struct MiddleChunk;

template <typename Type> struct Literal;
typedef Literal<double> FloatLiteral;
typedef Literal<int64_t> IntLiteral;
typedef Literal<std::string> StringLiteral;

struct Expression : Node {
    struct AnyVisitor {
        virtual std::any visitAny(const Identifier &) = 0;
        virtual std::any visitAny(const FunctionCall &) = 0;
        virtual std::any visitAny(const Property &) = 0;
        virtual std::any visitAny(const Descriptor &) = 0;
        virtual std::any visitAny(const Binary &) = 0;
        virtual std::any visitAny(const Logical &) = 0;
        virtual std::any visitAny(const Unary &) = 0;
        virtual std::any visitAny(const FloatLiteral &) = 0;
        virtual std::any visitAny(const IntLiteral &) = 0;
        virtual std::any visitAny(const StringLiteral &) = 0;
        virtual std::any visitAny(const RangeChunk &) = 0;
        virtual std::any visitAny(const AnyChunk &) = 0;
        virtual std::any visitAny(const LastChunk &) = 0;
        virtual std::any visitAny(const MiddleChunk &) = 0;
    };

    template <typename T>
    struct Visitor : AnyVisitor {
        std::any visitAny(const Identifier &e) { return visit(e); }
        std::any visitAny(const FunctionCall &e) { return visit(e); }
        std::any visitAny(const Property &e) { return visit(e); }
        std::any visitAny(const Descriptor &e) { return visit(e); }
        std::any visitAny(const Binary &e) { return visit(e); }
        std::any visitAny(const Logical &e) { return visit(e); }
        std::any visitAny(const Unary &e) { return visit(e); }
        std::any visitAny(const FloatLiteral &e) { return visit(e); }
        std::any visitAny(const IntLiteral &e) { return visit(e); }
        std::any visitAny(const StringLiteral &e) { return visit(e); }
        std::any visitAny(const RangeChunk &e) { return visit(e); }
        std::any visitAny(const AnyChunk &e) { return visit(e); }
        std::any visitAny(const LastChunk &e) { return visit(e); }
        std::any visitAny(const MiddleChunk &e) { return visit(e); }

        virtual T visit(const Identifier &) = 0;
        virtual T visit(const FunctionCall &) = 0;
        virtual T visit(const Property &) = 0;
        virtual T visit(const Descriptor &) = 0;
        virtual T visit(const Binary &) = 0;
        virtual T visit(const Logical &) = 0;
        virtual T visit(const Unary &) = 0;
        virtual T visit(const FloatLiteral &) = 0;
        virtual T visit(const IntLiteral &) = 0;
        virtual T visit(const StringLiteral &) = 0;
        virtual T visit(const RangeChunk &) = 0;
        virtual T visit(const AnyChunk &) = 0;
        virtual T visit(const LastChunk &) = 0;
        virtual T visit(const MiddleChunk &) = 0;
    };

    struct VoidVisitor : AnyVisitor {
        std::any visitAny(const Identifier &e) { visit(e); return std::any(); }
        std::any visitAny(const FunctionCall &e) { visit(e); return std::any(); }
        std::any visitAny(const Property &e) { visit(e); return std::any(); }
        std::any visitAny(const Descriptor &e) { visit(e); return std::any(); }
        std::any visitAny(const Binary &e) { visit(e); return std::any(); }
        std::any visitAny(const Logical &e) { visit(e); return std::any(); }
        std::any visitAny(const Unary &e) { visit(e); return std::any(); }
        std::any visitAny(const FloatLiteral &e) { visit(e); return std::any(); }
        std::any visitAny(const IntLiteral &e) { visit(e); return std::any(); }
        std::any visitAny(const StringLiteral &e) { visit(e); return std::any(); }
        std::any visitAny(const RangeChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const AnyChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const LastChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const MiddleChunk &e) { visit(e); return std::any(); }

        virtual void visit(const Identifier &) = 0;
        virtual void visit(const FunctionCall &) = 0;
        virtual void visit(const Property &) = 0;
        virtual void visit(const Descriptor &) = 0;
        virtual void visit(const Binary &) = 0;
        virtual void visit(const Logical &) = 0;
        virtual void visit(const Unary &) = 0;
        virtual void visit(const FloatLiteral &) = 0;
        virtual void visit(const IntLiteral &) = 0;
        virtual void visit(const StringLiteral &) = 0;
        virtual void visit(const RangeChunk &) = 0;
        virtual void visit(const AnyChunk &) = 0;
        virtual void visit(const LastChunk &) = 0;
        virtual void visit(const MiddleChunk &) = 0;
    };
    virtual ~Expression() = default;

	template <typename T>
	T accept(Visitor<T> &v) {
		return std::any_cast<T>(acceptAny(v));
	}

	void accept(VoidVisitor &v) {
		acceptAny(v);
	}

    virtual std::any acceptAny(AnyVisitor &v) const = 0;
};

struct ExpressionList : Node {
    std::vector<Owned<Expression>> expressions;

    ExpressionList();
    ExpressionList(Owned<Expression> &e);

    void add(Owned<Expression> &e) { expressions.push_back(std::move(e)); }
};

struct FunctionCall : Expression {
    Owned<Identifier> identifier;
    Owned<ExpressionList> arguments;

    FunctionCall(Owned<Identifier> &id, Owned<ExpressionList> &args);
    FunctionCall(Owned<Identifier> &id, Owned<Expression> &arg);
    FunctionCall(Owned<Identifier> &id);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
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

    Binary(Operator binaryOperator, Owned<Expression> &leftExpression,
           Owned<Expression> &rightExpression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Logical : Expression {
    enum Operator { And, Or };

    Operator logicalOperator;
    Owned<Expression> leftExpression, rightExpression;

    Logical(Operator logicalOperator, Owned<Expression> &leftExpression,
            Owned<Expression> &rightExpression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Unary : Expression {
    enum Operator { ThereIsA, Minus, Not };

    Operator unaryOperator;
    Owned<Expression> expression;

    Unary(Operator unaryOperator, Owned<Expression> &expression);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

template <typename Type> struct Literal : Expression {
    Type value;

    Literal(Type v) : value(v) {}

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

typedef Literal<double> FloatLiteral;
typedef Literal<int64_t> IntLiteral;
typedef Literal<std::string> StringLiteral;

CH_AST_NAMESPACE_END
