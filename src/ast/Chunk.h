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

CH_AST_NAMESPACE_BEGIN

struct Expression;
struct RangeChunk;
struct AnyChunk;
struct LastChunk;
struct MiddleChunk;

struct Chunk : Node {
    struct AnyVisitor {
        virtual std::any visitAny(const RangeChunk &) = 0;
        virtual std::any visitAny(const AnyChunk &) = 0;
        virtual std::any visitAny(const LastChunk &) = 0;
        virtual std::any visitAny(const MiddleChunk &) = 0;
    };

    template <typename T>
    struct Visitor : AnyVisitor {
        std::any visitAny(const RangeChunk &e) { return visit(e); }
        std::any visitAny(const AnyChunk &e) { return visit(e); }
        std::any visitAny(const LastChunk &e) { return visit(e); }
        std::any visitAny(const MiddleChunk &e) { return visit(e); }

        virtual T visit(const RangeChunk &) = 0;
        virtual T visit(const AnyChunk &) = 0;
        virtual T visit(const LastChunk &) = 0;
        virtual T visit(const MiddleChunk &) = 0;
    };

    struct VoidVisitor : AnyVisitor {
        std::any visitAny(const RangeChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const AnyChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const LastChunk &e) { visit(e); return std::any(); }
        std::any visitAny(const MiddleChunk &e) { visit(e); return std::any(); }

        virtual void visit(const RangeChunk &) = 0;
        virtual void visit(const AnyChunk &) = 0;
        virtual void visit(const LastChunk &) = 0;
        virtual void visit(const MiddleChunk &) = 0;
    };

    enum Type { Char, Word, Item, Line } type;

    Chunk(Type type);

	template <typename T>
	T accept(Visitor<T> &v) const {
		return std::any_cast<T>(acceptAny(v));
	}

	void accept(VoidVisitor &v) const {
		acceptAny(v);
	}

    virtual std::any acceptAny(AnyVisitor &v) const = 0;
};

struct RangeChunk : Chunk {
    Owned<Expression> start;
    Owned<Expression> end;

    RangeChunk(Type type, Owned<Expression> &start, Owned<Expression> &end);
    RangeChunk(Type type, Owned<Expression> &start);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct LastChunk : Chunk {
    LastChunk(Type type);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct MiddleChunk : Chunk {
    MiddleChunk(Type type);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct AnyChunk : Chunk {
    AnyChunk(Type type);

    std::any acceptAny(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
