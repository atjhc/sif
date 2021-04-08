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

CH_AST_NAMESPACE_BEGIN

struct Repeat : Statement {
    Owned<StatementList> statements;

    Repeat(Owned<StatementList> &s);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct RepeatCount : Repeat {
    Owned<Expression> countExpression;

    RepeatCount(Owned<Expression> &cs, Owned<StatementList> &s);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct RepeatRange : Repeat {
    Owned<Identifier> variable;
    Owned<Expression> startExpression;
    Owned<Expression> endExpression;
    bool ascending;

    RepeatRange(Owned<Identifier> &v, Owned<Expression> &se, Owned<Expression> &ee,
                bool asc, Owned<StatementList> &s);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct RepeatCondition : Repeat {
    Owned<Expression> condition;
    bool conditionValue;

    RepeatCondition(Owned<Expression> &c, bool cv, Owned<StatementList> &sl);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct ExitRepeat : Statement {
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct NextRepeat : Statement {
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
