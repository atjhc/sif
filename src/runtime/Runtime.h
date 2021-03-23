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
#include "ast/ast.h"
#include "runtime/Variables.h"

#include <vector>
#include <stack>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <optional>
#include <iostream>

CH_NAMESPACE_BEGIN

using namespace ast;

struct RuntimeError: std::runtime_error {
    Location where;

    RuntimeError(const std::string &_what, const Location &_where)
        : std::runtime_error(_what), where(_where) {}
};

struct RuntimeConfig {
    std::ostream &stdout = std::cout;
    std::istream &stdin = std::cin;
    std::ostream &stderr = std::cerr;
};

struct RuntimeStackFrame {
    using StringSet = std::unordered_set<std::string>;

    std::string name;
    Variables variables;
    StringSet globals;
    Value returningValue;

    bool skippingRepeat = false;
    bool exitingRepeat = false;
    bool returning = false;
    bool passing = false;
    bool exiting = false;

    RuntimeStackFrame(const std::string &_name) 
        : name(_name) {}
};

class Runtime: StatementVisitor, ExpressionVisitor {
    using HandlerMap = std::unordered_map<std::string, std::reference_wrapper<std::unique_ptr<Handler>>>;

    RuntimeConfig config;
    std::string name;

    // AST
    std::unique_ptr<Script> script;
    HandlerMap handlersByName;
    HandlerMap functionsByName;

    // State information
    std::stack<RuntimeStackFrame> stack;
    Variables globals;

public:

    Runtime(const std::string &name, std::unique_ptr<Script> &s);

    void send(const std::string &name, const std::vector<Value> &arguments = {});
    Value call(const std::string &name, const std::vector<Value> &arguments = {});
    
private:

    void set(const std::string &name, const Value &value);
    Value get(const std::string &name) const;

    void execute(const std::unique_ptr<ast::Handler> &handler, const std::vector<Value> &arguments);
    void execute(const std::unique_ptr<ast::StatementList> &statements);

    void report(const RuntimeError &error);

#pragma mark - StatementVisitor

    void visit(const Message &s) override;
    void visit(const If &) override;
    void visit(const Repeat &) override;
    void visit(const RepeatCount &s) override;
    void visit(const RepeatRange &s) override;
    void visit(const RepeatCondition &s) override;
    void visit(const ExitRepeat &) override;
    void visit(const NextRepeat &) override;
    void visit(const Exit &) override;
    void visit(const Pass &) override;
    void visit(const Global &) override;
    void visit(const Return &) override;
    void visit(const Put &) override;
    void visit(const Get &) override;
    void visit(const Ask &) override;

#pragma mark - ExpressionVisitor

    Value valueOf(const Identifier &) override;
    Value valueOf(const FunctionCall &) override;
    Value valueOf(const BinaryOp &) override;
    Value valueOf(const Not &) override;
    Value valueOf(const Minus &) override;
    Value valueOf(const FloatLiteral &) override;
    Value valueOf(const IntLiteral &) override;
    Value valueOf(const StringLiteral &) override;
    Value valueOf(const RangeChunk &) override;
    Value valueOf(const AnyChunk &) override;
    Value valueOf(const LastChunk &) override;
    Value valueOf(const MiddleChunk &) override;
};

CH_NAMESPACE_END
