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
#include "ast/Statement.h"

#include <ostream>
#include <vector>

SIF_NAMESPACE_BEGIN

struct Repeat : Statement {
    Owned<Statement> statement;

    Repeat(Owned<Statement> statement);

    void accept(Visitor &v) const override { return v.visit(*this); }
};

// struct RepeatCount : Repeat {
//     Owned<Expression> countExpression;

//     RepeatCount(Owned<Expression> countExpression, Owned<StatementList> statements);

//     void accept(Visitor &v) const override { return v.visit(*this); }
// };

struct RepeatCondition : Repeat {
    Owned<Expression> condition;
    bool conditionValue;

    RepeatCondition(Owned<Statement> statement, Owned<Expression> condition, bool conditionValue);

    void accept(Visitor &v) const override { return v.visit(*this); }
};

struct RepeatForEach : Repeat {
    Owned<Variable> variable;
    Owned<Expression> expression;

    RepeatForEach(Owned<Statement> statement, Owned<Variable> variable, Owned<Expression> expression);

    void accept(Visitor &v) const override { return v.visit(*this); }
};

struct ExitRepeat : Statement {
    void accept(Visitor &v) const override { return v.visit(*this); }
};

struct NextRepeat : Statement {
    void accept(Visitor &v) const override { return v.visit(*this); }
};

SIF_NAMESPACE_END
