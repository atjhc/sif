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
#include "ast/Statement.h"

CH_AST_NAMESPACE_BEGIN

struct Expression;
struct ExpressionList;

struct Command;
struct Put;
struct Get;
struct Ask;
struct Add;
struct Subtract;
struct Multiply;
struct Divide;

struct CommandVisitor {
    virtual void perform(const Put &) = 0;
    virtual void perform(const Get &) = 0;
    virtual void perform(const Ask &) = 0;
    virtual void perform(const Add &) = 0;
    virtual void perform(const Subtract &) = 0;
    virtual void perform(const Multiply &) = 0;
    virtual void perform(const Divide &) = 0;
};

struct Command : Statement {
    Owned<Identifier> name;
    Owned<ExpressionList> arguments;

    Command(Owned<Identifier> &n, Owned<ExpressionList> &args);
    Command(Owned<Identifier> &n, Owned<Expression> &arg);
    Command(Owned<Identifier> &n);
    Command(Identifier *n);
    
    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    virtual void perform(CommandVisitor &visitor) const { 
        /* user commands are not executed by the runtime. */
    }

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Preposition : Node {
    enum PrepositionType { Before, Into, After } type;

    Preposition(PrepositionType t);

    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

#pragma mark Messages

struct Put : Command {
    Owned<Expression> expression;
    Owned<Preposition> preposition;
    Owned<Identifier> target;

    Put(Owned<Expression> &e, Owned<Preposition> &p, Owned<Identifier> &t);
    Put(Owned<Expression> &e);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Get : Command {
    Owned<Expression> expression;

    Get(Owned<Expression> &e);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Ask : Command {
    Owned<Expression> expression;

    Ask(Owned<Expression> &e);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Add : Command {
    Owned<Expression> expression;
    Owned<Identifier> destination;

    Add(Owned<Expression> &e, Owned<Identifier> &d);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Subtract : Command {
    Owned<Expression> expression;
    Owned<Identifier> destination;

    Subtract(Owned<Expression> &e, Owned<Identifier> &d);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Multiply : Command {
    Owned<Expression> expression;
    Owned<Identifier> destination;

    Multiply(Owned<Expression> &e, Owned<Identifier> &d);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

struct Divide : Command {
    Owned<Expression> expression;
    Owned<Identifier> destination;

    Divide(Owned<Expression> &e, Owned<Identifier> &d);

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }
    std::any accept(AnyVisitor &v) const override { return v.visitAny(*this); }
};

CH_AST_NAMESPACE_END
