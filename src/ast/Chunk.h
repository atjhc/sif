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
    Owned<Expression> expression;

    Chunk(Type t, Owned<Expression> &e) : type(t), expression(std::move(e)) {}

    Chunk(Type t) : type(t), expression(nullptr) {}

    std::string ordinalName() const;
    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct RangeChunk : Chunk {
    Owned<Expression> start;
    Owned<Expression> end;

    RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee, Owned<Expression> &e)
        : Chunk(t, e), start(std::move(se)), end(std::move(ee)) {}

    RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee)
        : Chunk(t), start(std::move(se)), end(std::move(ee)) {}
    
    RangeChunk(Type t, Owned<Expression> &se)
        : Chunk(t), start(std::move(se)) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct LastChunk : Chunk {
    LastChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}

    LastChunk(Type t) : Chunk(t) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct MiddleChunk : Chunk {
    MiddleChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}
    MiddleChunk(Type t) : Chunk(t) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct AnyChunk : Chunk {
    AnyChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}
    AnyChunk(Type t) : Chunk(t) {}

    Value evaluate(ExpressionVisitor &v) const override { return v.valueOf(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
