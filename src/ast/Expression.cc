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

Call::Call(const Signature &signature, std::vector<Strong<Expression>> arguments)
    : signature(signature), arguments(arguments) {}

Binary::Binary(Strong<Expression> leftExpression, Operator binaryOperator,
               Strong<Expression> rightExpression)
    : leftExpression(leftExpression), binaryOperator(binaryOperator),
      rightExpression(rightExpression) {}

Unary::Unary(Operator unaryOperator, Strong<Expression> expression)
    : unaryOperator(unaryOperator), expression(expression) {}

Grouping::Grouping(Strong<Expression> expression) : expression(expression) {}

Variable::Variable(const Token &name, Optional<Scope> scope) : name(name), scope(scope) {}

RangeLiteral::RangeLiteral(Strong<Expression> start, Strong<Expression> end, bool closed)
    : start(start), end(end), closed(closed) {}

ListLiteral::ListLiteral(std::vector<Strong<Expression>> expressions) : expressions(expressions) {}

DictionaryLiteral::DictionaryLiteral(Mapping<Strong<Expression>, Strong<Expression>> values)
    : values(values) {}

Literal::Literal(Token token) : token(token) {}

StringInterpolation::StringInterpolation(Token leftPart, Strong<Expression> expression,
                                         Strong<Expression> rightPart)
    : left(leftPart), expression(expression), right(rightPart) {}

SIF_NAMESPACE_END
