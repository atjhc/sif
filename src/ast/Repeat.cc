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

#include "ast/Repeat.h"
#include "ast/Statement.h"

SIF_NAMESPACE_BEGIN

Repeat::Repeat(Strong<Statement> statement) : statement(statement) {}

RepeatCondition::RepeatCondition(Strong<Statement> statement, Strong<Expression> condition,
                                 RepeatCondition::Conjunction conjunction)
    : Repeat(std::move(statement)), condition(condition), conjunction(conjunction) {}

RepeatFor::RepeatFor(Strong<Statement> statement, std::vector<Strong<Variable>> variables,
                     Strong<Expression> expression)
    : Repeat(statement), variables(variables), expression(expression) {}

SIF_NAMESPACE_END
