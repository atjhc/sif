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
#include "ast/Identifier.h"
#include "ast/Statement.h"

CH_AST_NAMESPACE_BEGIN

Repeat::Repeat(Owned<StatementList> &s) : statements(std::move(s)) {}

RepeatCount::RepeatCount(Owned<Expression> &cs, Owned<StatementList> &s)
    : Repeat(s), countExpression(std::move(cs)) {}

RepeatRange::RepeatRange(Owned<Identifier> &v, Owned<Expression> &se, Owned<Expression> &ee,
                         bool asc, Owned<StatementList> &s)
    : Repeat(s), variable(std::move(v)), startExpression(std::move(se)),
      endExpression(std::move(ee)), ascending(asc) {}

RepeatCondition::RepeatCondition(Owned<Expression> &c, bool cv, Owned<StatementList> &sl)
    : Repeat(sl), condition(std::move(c)), conditionValue(cv) {}

CH_AST_NAMESPACE_END
