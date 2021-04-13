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

#include <ostream>
#include <any>

CH_AST_NAMESPACE_BEGIN

struct Script;
struct Handler;
struct StatementList;
struct IdentifierList;
struct ExpressionList;
struct If;
struct Repeat;
struct RepeatCount;
struct RepeatRange;
struct RepeatCondition;
struct ExitRepeat;
struct NextRepeat;
struct Exit;
struct Pass;
struct Global;
struct Return;
struct Do;
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
struct FloatLiteral;
struct IntLiteral;
struct StringLiteral;
struct RangeChunk;
struct AnyChunk;
struct LastChunk;
struct MiddleChunk;
struct Command;
struct Put;
struct Get;
struct Ask;
struct Add;
struct Subtract;
struct Multiply;
struct Divide;

struct AnyVisitor {
    virtual std::any visitAny(const Script &) = 0;
    virtual std::any visitAny(const Handler &) = 0;
    virtual std::any visitAny(const StatementList &) = 0;
    virtual std::any visitAny(const IdentifierList &) = 0;
    virtual std::any visitAny(const ExpressionList &) = 0;
    virtual std::any visitAny(const If &) = 0;
    virtual std::any visitAny(const Repeat &) = 0;
    virtual std::any visitAny(const RepeatCount &) = 0;
    virtual std::any visitAny(const RepeatRange &) = 0;
    virtual std::any visitAny(const RepeatCondition &) = 0;
    virtual std::any visitAny(const ExitRepeat &) = 0;
    virtual std::any visitAny(const NextRepeat &) = 0;
    virtual std::any visitAny(const Exit &) = 0;
    virtual std::any visitAny(const Pass &) = 0;
    virtual std::any visitAny(const Global &) = 0;
    virtual std::any visitAny(const Return &) = 0;
    virtual std::any visitAny(const Do &) = 0;
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
    virtual std::any visitAny(const Command &) = 0;
    virtual std::any visitAny(const Put &) = 0;
    virtual std::any visitAny(const Get &) = 0;
    virtual std::any visitAny(const Ask &) = 0;
    virtual std::any visitAny(const Add &) = 0;
    virtual std::any visitAny(const Subtract &) = 0;
    virtual std::any visitAny(const Multiply &) = 0;
    virtual std::any visitAny(const Divide &) = 0;
};

struct Location {
    unsigned int position = 1;
    unsigned int lineNumber = 1;
};

struct Node {
    Location location;

    virtual ~Node() = default;

    virtual std::any accept(AnyVisitor &v) const = 0;
};

static inline std::ostream &operator<<(std::ostream &out, const Location &location) {
    return out << location.lineNumber << ":" << location.position;
}

CH_AST_NAMESPACE_END
