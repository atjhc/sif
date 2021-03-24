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
#include "ast/Expressions.h"
#include "ast/Statements.h"

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
    virtual void perform(const Command &) = 0;
    virtual void perform(const Put &) = 0;
    virtual void perform(const Get &) = 0;
    virtual void perform(const Ask &) = 0;
    virtual void perform(const Add &) = 0;
    virtual void perform(const Subtract &) = 0;
    virtual void perform(const Multiply &) = 0;
    virtual void perform(const Divide &) = 0;
};

struct Command : Statement {
    std::unique_ptr<Identifier> name;
    std::unique_ptr<ExpressionList> arguments;

    Command(Identifier *_name, ExpressionList *_arguments = nullptr)
        : name(_name), arguments(_arguments) {}

    void accept(StatementVisitor &visitor) const override { visitor.visit(*this); }

    virtual void perform(CommandVisitor &visitor) const { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Preposition : Node {
    enum PrepositionType { Before, Into, After };

    PrepositionType type;

    Preposition(PrepositionType _type) : type(_type) {}

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

#pragma mark Messages

struct Put : Command {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Preposition> preposition;
    std::unique_ptr<Identifier> target;

    Put(Expression *_expression, Preposition *_preposition, Identifier *_target)
        : Command(new Identifier("put")), expression(_expression), preposition(_preposition),
          target(_target) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Get : Command {
    std::unique_ptr<Expression> expression;

    Get(Expression *_expression) : Command(new Identifier("get")), expression(_expression) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Ask : Command {
    std::unique_ptr<Expression> expression;

    Ask(Expression *_expression) : Command(new Identifier("ask")), expression(_expression) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Add : Command {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Identifier> destination;

    Add(Expression *_expression, Identifier *_destination)
        : Command(new Identifier("add")), expression(_expression), destination(_destination) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Subtract : Command {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Identifier> destination;

    Subtract(Expression *_expression, Identifier *_destination)
        : Command(new Identifier("subtract")), expression(_expression), destination(_destination) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Multiply : Command {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Identifier> destination;

    Multiply(Expression *_expression, Identifier *_destination)
        : Command(new Identifier("multiply")), expression(_expression), destination(_destination) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Divide : Command {
    std::unique_ptr<Expression> expression;
    std::unique_ptr<Identifier> destination;

    Divide(Expression *_expression, Identifier *_destination)
        : Command(new Identifier("divide")), expression(_expression), destination(_destination) {}

    void perform(CommandVisitor &visitor) const override { visitor.perform(*this); }

    void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
