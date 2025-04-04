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

#include <ostream>
#include <variant>
#include <vector>

SIF_NAMESPACE_BEGIN

struct Block;
struct FunctionDecl;
struct Assignment;
struct If;
struct Try;
struct Use;
struct Using;
struct Repeat;
struct RepeatCondition;
struct RepeatFor;
struct ExitRepeat;
struct NextRepeat;
struct Return;
struct ExpressionStatement;

struct Statement : Node {
    struct Visitor {
        virtual void visit(const Block &) = 0;
        virtual void visit(const FunctionDecl &) = 0;
        virtual void visit(const Assignment &) = 0;
        virtual void visit(const If &) = 0;
        virtual void visit(const Try &) = 0;
        virtual void visit(const Use &) = 0;
        virtual void visit(const Using &) = 0;
        virtual void visit(const Repeat &) = 0;
        virtual void visit(const RepeatFor &) = 0;
        virtual void visit(const RepeatCondition &) = 0;
        virtual void visit(const ExitRepeat &) = 0;
        virtual void visit(const NextRepeat &) = 0;
        virtual void visit(const Return &) = 0;
        virtual void visit(const ExpressionStatement &) = 0;
    };

    virtual ~Statement() = default;

    virtual void accept(Visitor &v) const = 0;
};

struct Block : Statement {
    std::vector<Strong<Statement>> statements;

    Block(std::vector<Strong<Statement>> statements);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct FunctionDecl : Statement {
    Optional<Signature> signature;
    Strong<Statement> statement;

    struct {
        SourceRange function;
        std::vector<SourceRange> words;
        std::vector<SourceRange> variables;
        Optional<SourceRange> end;
        Optional<SourceRange> endFunction;
    } ranges;

    FunctionDecl() {}
    FunctionDecl(const Signature &signature, Strong<Statement> statement);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct AssignmentTarget : Node {
    Strong<Variable> variable;
    Optional<Token> typeName;
    std::vector<Strong<Expression>> subscripts;

    AssignmentTarget(Strong<Variable> variable, Optional<Token> typeName = None,
                     std::vector<Strong<Expression>> subscripts = {});
};

struct Assignment : Statement {
    std::vector<Strong<AssignmentTarget>> targets;
    Strong<Expression> expression;

    struct {
        SourceRange set;
        Optional<SourceRange> to;
    } ranges;

    Assignment() {}
    Assignment(std::vector<Strong<AssignmentTarget>> targets, Strong<Expression> expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct If : Statement {
    Strong<Expression> condition;
    Strong<Statement> ifStatement;
    Strong<Statement> elseStatement;

    struct {
        SourceRange if_;
        Optional<SourceRange> then;
        Optional<SourceRange> else_;
        Optional<SourceRange> end;
        Optional<SourceRange> endIf;
    } ranges;

    If() {}
    If(Strong<Expression> condition, Strong<Statement> ifStatement,
       Strong<Statement> elseStatement);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Try : Statement {
    Strong<Statement> statement;

    struct {
        SourceRange try_;
        Optional<SourceRange> end;
        Optional<SourceRange> endTry;
    } ranges;

    Try() {}
    Try(Strong<Statement> statement);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Use : Statement {
    Token target;

    struct {
        SourceRange use;
    } ranges;

    Use() {}
    Use(Token target);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Using : Statement {
    Token target;
    Strong<Statement> statement;

    struct {
        SourceRange using_;
        Optional<SourceRange> end;
        Optional<SourceRange> endUsing;
    } ranges;

    Using() {}
    Using(Token target, Strong<Statement> statement);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct Return : Statement {
    Strong<Expression> expression;

    struct {
        SourceRange return_;
    } ranges;

    Return() {}
    Return(Strong<Expression> expression);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

struct ExpressionStatement : Statement {
    Strong<Expression> expression;

    ExpressionStatement(Strong<Expression> expression = nullptr);

    void accept(Statement::Visitor &v) const override { v.visit(*this); }
};

SIF_NAMESPACE_END
