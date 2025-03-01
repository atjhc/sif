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

Repeat::Repeat(Strong<Statement> statement) : statement(std::move(statement)) {}

RepeatCondition::RepeatCondition(Strong<Statement> statement, Strong<Expression> condition,
                                 bool conditionValue)
    : Repeat(std::move(statement)), condition(std::move(condition)),
      conditionValue(conditionValue) {}

RepeatFor::RepeatFor(Strong<Statement> statement, std::vector<Strong<Variable>> variables,
                     Strong<Expression> expression)
    : Repeat(std::move(statement)), variables(std::move(variables)),
      expression(std::move(expression)) {}

SIF_NAMESPACE_END
