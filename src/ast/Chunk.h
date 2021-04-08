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
#include "ast/Expression.h"

CH_AST_NAMESPACE_BEGIN

struct Chunk : Expression {
    enum Type { Char, Word, Item, Line } type;
    Owned<Expression> expression;

    Chunk(Type t, Owned<Expression> &e);
    Chunk(Type t);

    virtual std::any accept(AnyVisitor &v) const override = 0;
};

struct RangeChunk : Chunk {
    Owned<Expression> start;
    Owned<Expression> end;

    RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee, Owned<Expression> &e);
    RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee);
    RangeChunk(Type t, Owned<Expression> &se);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct LastChunk : Chunk {
    LastChunk(Type t, Owned<Expression> &e);
    LastChunk(Type t);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct MiddleChunk : Chunk {
    MiddleChunk(Type t, Owned<Expression> &e);
    MiddleChunk(Type t);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct AnyChunk : Chunk {
    AnyChunk(Type t, Owned<Expression> &e);
    AnyChunk(Type t);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
