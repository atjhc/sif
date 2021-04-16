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

#include "ast/Chunk.h"

CH_AST_NAMESPACE_BEGIN

Chunk::Chunk(Type t, Owned<Expression> &e) : type(t), expression(std::move(e)) {}
Chunk::Chunk(Type t) : type(t), expression(nullptr) {}

RangeChunk::RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee, Owned<Expression> &e)
    : Chunk(t, e), start(std::move(se)), end(std::move(ee)) {}

RangeChunk::RangeChunk(Type t, Owned<Expression> &se, Owned<Expression> &ee)
    : Chunk(t), start(std::move(se)), end(std::move(ee)) {}

RangeChunk::RangeChunk(Type t, Owned<Expression> &se) : Chunk(t), start(std::move(se)) {}

LastChunk::LastChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}
LastChunk::LastChunk(Type t) : Chunk(t) {}

MiddleChunk::MiddleChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}
MiddleChunk::MiddleChunk(Type t) : Chunk(t) {}

AnyChunk::AnyChunk(Type t, Owned<Expression> &e) : Chunk(t, e) {}
AnyChunk::AnyChunk(Type t) : Chunk(t) {}

CH_AST_NAMESPACE_END
