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

#include "ast/Expression.h"
#include "ast/Descriptor.h"
#include "ast/Identifier.h"
#include "ast/Chunk.h"

CH_AST_NAMESPACE_BEGIN

ExpressionList::ExpressionList() {}
ExpressionList::ExpressionList(Owned<Expression> &e) { add(e); }

FunctionCall::FunctionCall(Owned<Identifier> &id, Owned<ExpressionList> &args)
    : identifier(std::move(id)), arguments(std::move(args)) {}

FunctionCall::FunctionCall(Owned<Identifier> &id, Owned<Expression> &arg)
    : identifier(std::move(id)), arguments(MakeOwned<ExpressionList>(arg)) {}

FunctionCall::FunctionCall(Owned<Identifier> &id) : identifier(std::move(id)), arguments(nullptr) {}

Binary::Binary(Operator o, Owned<Expression> &l, Owned<Expression> &r)
    : binaryOperator(o), leftExpression(std::move(l)), rightExpression(std::move(r)) {}

Logical::Logical(Operator o, Owned<Expression> &l, Owned<Expression> &r)
    : logicalOperator(o), leftExpression(std::move(l)), rightExpression(std::move(r)) {}

Unary::Unary(Operator o, Owned<Expression> &e) : unaryOperator(o), expression(std::move(e)) {}

ChunkExpression::ChunkExpression(Owned<Chunk> &c, Owned<Expression> &e) 
    : chunk(std::move(c)), expression(std::move(e)) {}

CH_AST_NAMESPACE_END
