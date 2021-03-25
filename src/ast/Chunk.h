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

CH_AST_NAMESPACE_BEGIN

struct Chunk : Expression {
    enum Type { Char, Word, Item, Line };

    Type type;
    Expression *expression;

    Chunk(Type _type, Expression *_expression) : type(_type), expression(_expression) {}

    std::string ordinalName() const;
    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct RangeChunk : Chunk {
    Expression *start;
    Expression *end;

    RangeChunk(Type _type, Expression *_start, Expression *_end, Expression *_expression = nullptr)
        : Chunk(_type, _expression), start(_start), end(_end) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct LastChunk : Chunk {
    LastChunk(Type _type, Expression *_expression = nullptr) : Chunk(_type, _expression) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct MiddleChunk : Chunk {
    MiddleChunk(Type _type, Expression *_expression = nullptr) : Chunk(_type, _expression) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct AnyChunk : Chunk {
    AnyChunk(Type _type, Expression *_expression = nullptr) : Chunk(_type, _expression) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
