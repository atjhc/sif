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
#include "ast/Expression.h"

#include <ostream>
#include <vector>

CH_AST_NAMESPACE_BEGIN

struct Statement : Node {
    virtual ~Statement() = default;

    virtual std::any accept(AnyVisitor &v) const = 0;
};

struct StatementList : Node {
    std::vector<Owned<Statement>> statements;

    StatementList();
    StatementList(Owned<Statement> &statement);

    void add(Owned<Statement> &statement) {
        statements.push_back(std::move(statement));
    }

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct If : Statement {
    Owned<Expression> condition;
    Owned<StatementList> ifStatements;
    Owned<StatementList> elseStatements;

    If(Owned<Expression> &c, Owned<Statement> &is);
    If(Owned<Expression> &c, Owned<Statement> &is, Owned<Statement> &es);
    If(Owned<Expression> &c, Owned<StatementList> &isl);
    If(Owned<Expression> &c, Owned<StatementList> &is, Owned<StatementList> &es);
    If(Owned<Expression> &c, Owned<Statement> &is, Owned<StatementList> &esl);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Exit : Statement {
    Owned<Identifier> messageKey;

    Exit(Owned<Identifier> &m);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Pass : Statement {
    Owned<Identifier> messageKey;

    Pass(Owned<Identifier> &m);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Global : Statement {
    Owned<IdentifierList> variables;

    Global(Owned<IdentifierList> &v);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Return : Statement {
    Owned<Expression> expression;

    Return(Owned<Expression> &e);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Do : Statement {
    Owned<Expression> expression;
    Owned<Expression> language;

    Do(Owned<Expression> &expression);
    Do(Owned<Expression> &expression, Owned<Expression> &language);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
