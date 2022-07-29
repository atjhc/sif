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

SIF_NAMESPACE_BEGIN

Call::Call(const Signature &signature, const std::vector<Optional<Token>> &tokens,
           std::vector<Owned<Expression>> arguments)
    : signature(signature), tokens(tokens), arguments(std::move(arguments)) {}

Binary::Binary(Owned<Expression> leftExpression, Operator binaryOperator,
               Owned<Expression> rightExpression)
    : leftExpression(std::move(leftExpression)), binaryOperator(binaryOperator),
      rightExpression(std::move(rightExpression)) {}

Unary::Unary(Operator unaryOperator, Owned<Expression> expression)
    : unaryOperator(unaryOperator), expression(std::move(expression)) {}

Grouping::Grouping(Owned<Expression> expression) : expression(std::move(expression)) {}

Variable::Variable(const Token &token, Optional<Token> typeName, Scope scope)
    : token(token), typeName(typeName), scope(scope) {}

RangeLiteral::RangeLiteral(Owned<Expression> start, Owned<Expression> end, bool closed)
    : start(std::move(start)), end(std::move(end)), closed(closed) {}

ListLiteral::ListLiteral(std::vector<Owned<Expression>> expressions)
    : expressions(std::move(expressions)) {}

DictionaryLiteral::DictionaryLiteral(Mapping<Owned<Expression>, Owned<Expression>> values)
    : values(std::move(values)) {}

Literal::Literal(Token token) : token(token) {}

SIF_NAMESPACE_END
