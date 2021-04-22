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
#include "ast/Expression.h"
#include "ast/Node.h"
#include "ast/Statement.h"

CH_AST_NAMESPACE_BEGIN

struct ExpressionList;

struct Command : Statement {
    Owned<Identifier> name;
    Owned<ExpressionList> arguments;

    Command(Owned<Identifier> &n, Owned<ExpressionList> &args);
    Command(Owned<Identifier> &n, Owned<Expression> &arg);
    Command(Owned<Identifier> &n);
    Command(Identifier *n);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Put : Command {
    Owned<Expression> expression;
    enum Preposition { Before, Into, After } preposition;
    Owned<Expression> target;

    Put(Owned<Expression> &expression, Preposition preposition, Owned<Expression> &target);
    Put(Owned<Expression> &expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Get : Command {
    Owned<Expression> expression;

    Get(Owned<Expression> &e);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Ask : Command {
    Owned<Expression> expression;

    Ask(Owned<Expression> &e);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Add : Command {
    Owned<Expression> expression;
    Owned<Expression> container;

    Add(Owned<Expression> &expression, Owned<Expression> &container);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Subtract : Command {
    Owned<Expression> expression;
    Owned<Expression> container;

    Subtract(Owned<Expression> &expression, Owned<Expression> &container);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Multiply : Command {
    Owned<Expression> expression;
    Owned<Expression> container;

    Multiply(Owned<Expression> &expression, Owned<Expression> &container);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Divide : Command {
    Owned<Expression> expression;
    Owned<Expression> container;

    Divide(Owned<Expression> &expression, Owned<Expression> &container);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Delete : Command {
    Owned<Expression> container;

    Delete(Owned<Expression> &container);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

CH_AST_NAMESPACE_END
