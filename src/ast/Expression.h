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
    virtual Value valueOf(const Identifier &) = 0;
    virtual Value valueOf(const FunctionCall &) = 0;
    virtual Value valueOf(const BinaryOp &) = 0;
    virtual Value valueOf(const Not &) = 0;
    virtual Value valueOf(const Minus &) = 0;
    virtual Value valueOf(const FloatLiteral &) = 0;
    virtual Value valueOf(const IntLiteral &) = 0;
    virtual Value valueOf(const StringLiteral &) = 0;
    virtual Value valueOf(const RangeChunk &) = 0;
    virtual Value valueOf(const AnyChunk &) = 0;
    virtual Value valueOf(const LastChunk &) = 0;
    virtual Value valueOf(const MiddleChunk &) = 0;
};

struct Expression : Node {
    virtual ~Expression() = default;
    virtual Value evaluate(ExpressionVisitor &) const = 0;
};

struct ExpressionList : Node {
    std::vector<std::unique_ptr<Expression>> expressions;

    ExpressionList() {}

    ExpressionList(Expression *expression) { add(expression); }

    void add(Expression *expression) {
        expressions.push_back(std::unique_ptr<Expression>(expression));
    }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Identifier : Expression {
    std::string name;

    Identifier(const std::string &_name) : name(_name) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FunctionCall : Expression {
    std::unique_ptr<Identifier> identifier;
    std::unique_ptr<ExpressionList> arguments;

    FunctionCall(Identifier *_identifier, ExpressionList *_arguments)
        : identifier(_identifier), arguments(_arguments) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

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
        Contains,
        Or,
        And,
        Plus,
        Minus,
        Multiply,
        Divide,
        Mod,
        Concat,
        ConcatWithSpace
    };

    Operator op;
    std::unique_ptr<Expression> left, right;

    BinaryOp(Operator _op, Expression *_left, Expression *_right)
        : op(_op), left(_left), right(_right) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Not : Expression {
    std::unique_ptr<Expression> expression;

    Not(Expression *_expression) : expression(_expression) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Minus : Expression {
    std::unique_ptr<Expression> expression;

    Minus(Expression *_expression) : expression(_expression) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FloatLiteral : Expression {
    float value;

    FloatLiteral(float _value) : value(_value) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int _value) : value(_value) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StringLiteral : Expression {
    std::string value;

    StringLiteral(const std::string &_value) : value(_value) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
