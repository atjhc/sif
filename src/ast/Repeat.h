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

#include "ast/Base.h"
#include "ast/Statement.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct Repeat : Statement {
    Owned<StatementList> statements;

    Repeat(Owned<StatementList> &s) : statements(std::move(s)) {}

    virtual void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
    virtual void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const {}
};

struct RepeatCount : Repeat {
    Owned<Expression> countExpression;

    RepeatCount(Owned<Expression> &cs, Owned<StatementList> &s)
        : Repeat(s), countExpression(std::move(cs)) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatRange : Repeat {
    Owned<Identifier> variable;
    Owned<Expression> startExpression;
    Owned<Expression> endExpression;
    bool ascending;

    RepeatRange(Owned<Identifier> &v, Owned<Expression> &se, Owned<Expression> &ee,
                bool asc, Owned<StatementList> &s)
        : Repeat(s), variable(std::move(v)), startExpression(std::move(se)),
          endExpression(std::move(ee)), ascending(asc) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatCondition : Repeat {
    Owned<Expression> condition;
    bool conditionValue;

    RepeatCondition(Owned<Expression> &c, bool cv, Owned<StatementList> &sl)
        : Repeat(sl), condition(std::move(c)), conditionValue(cv) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct ExitRepeat : Statement {
    void accept(StatementVisitor &v) const override { v.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct NextRepeat : Statement {
    void accept(StatementVisitor &v) const override { v.visit(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
