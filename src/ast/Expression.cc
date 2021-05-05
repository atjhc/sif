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

FunctionCall::FunctionCall(Owned<Identifier> &name, Owned<ExpressionList> &arguments)
    : name(std::move(name)), arguments(std::move(arguments)) {}

FunctionCall::FunctionCall(Owned<Identifier> &name) : name(std::move(name)), arguments(nullptr) {}

Binary::Binary(Operator o, Owned<Expression> &l, Owned<Expression> &r)
    : binaryOperator(o), leftExpression(std::move(l)), rightExpression(std::move(r)) {}

Binary::Binary(Operator binaryOperator, Owned<Expression> &rightExpression)
    : binaryOperator(binaryOperator), leftExpression(nullptr), rightExpression(std::move(rightExpression)) {}

Logical::Logical(Operator o, Owned<Expression> &l, Owned<Expression> &r)
    : logicalOperator(o), leftExpression(std::move(l)), rightExpression(std::move(r)) {}

Unary::Unary(Operator o, Owned<Expression> &e) : unaryOperator(o), expression(std::move(e)) {}

ChunkExpression::ChunkExpression(Owned<Chunk> &c, Owned<Expression> &e)
    : chunk(std::move(c)), expression(std::move(e)) {}

CountExpression::CountExpression(Owned<Identifier> &identifier, Owned<Expression> &container)
    : identifier(std::move(identifier)), container(std::move(container)) {}

CH_AST_NAMESPACE_END
