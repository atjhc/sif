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
#include "ast/Handler.h"
#include "runtime/Value.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct Identifier;
struct FunctionCall;
struct Property;
struct ObjectProperty;
struct Descriptor;
struct BinaryOp;
struct Not;
struct Minus;
struct FloatLiteral;
struct IntLiteral;
struct StringLiteral;
struct RangeChunk;
struct AnyChunk;
struct MiddleChunk;
struct LastChunk;

class ExpressionVisitor {
  public:
    virtual runtime::Value valueOf(const Identifier &) = 0;
    virtual runtime::Value valueOf(const FunctionCall &) = 0;
    virtual runtime::Value valueOf(const Property &) = 0;
    virtual runtime::Value valueOf(const Descriptor &) = 0;
    virtual runtime::Value valueOf(const BinaryOp &) = 0;
    virtual runtime::Value valueOf(const Not &) = 0;
    virtual runtime::Value valueOf(const Minus &) = 0;
    virtual runtime::Value valueOf(const FloatLiteral &) = 0;
    virtual runtime::Value valueOf(const IntLiteral &) = 0;
    virtual runtime::Value valueOf(const StringLiteral &) = 0;
    virtual runtime::Value valueOf(const RangeChunk &) = 0;
    virtual runtime::Value valueOf(const AnyChunk &) = 0;
    virtual runtime::Value valueOf(const LastChunk &) = 0;
    virtual runtime::Value valueOf(const MiddleChunk &) = 0;
};

struct Expression : Node {
    virtual ~Expression() = default;
    virtual runtime::Value evaluate(ExpressionVisitor &) const = 0;
};

struct ExpressionList : Node {
    std::vector<Owned<Expression>> expressions;

    ExpressionList() {}

    ExpressionList(Owned<Expression> &e) { add(e); }

    void add(Owned<Expression> &e) {
        expressions.push_back(std::move(e));
    }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Identifier : Expression {
    std::string name;

    Identifier(const std::string &n) : name(n) {}
    Identifier(const char *n) : name(n) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FunctionCall : Expression {
    Owned<Identifier> identifier;
    Owned<ExpressionList> arguments;

    FunctionCall(Owned<Identifier> &id, Owned<ExpressionList> &args)
        : identifier(std::move(id)), arguments(std::move(args)) {}

    FunctionCall(Owned<Identifier> &id, Owned<Expression> &arg)
        : identifier(std::move(id)), arguments(MakeOwned<ExpressionList>(arg)) {}

    FunctionCall(Owned<Identifier> &id)
        : identifier(std::move(id)), arguments(nullptr) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
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

    BinaryOp(Operator o, Owned<Expression> &l, Owned<Expression> &r)
        : op(o), left(std::move(l)), right(std::move(r)) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Not : Expression {
    Owned<Expression> expression;

    Not(Owned<Expression> &e) : expression(std::move(e)) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Minus : Expression {
    Owned<Expression> expression;

    Minus(Owned<Expression> &e) : expression(std::move(e)) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FloatLiteral : Expression {
    float value;

    FloatLiteral(float v) : value(v) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int i) : value(i) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StringLiteral : Expression {
    std::string value;

    StringLiteral(const std::string &v) : value(v) {}

    runtime::Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
