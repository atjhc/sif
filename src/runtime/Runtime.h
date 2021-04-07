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
#include "ast/Chunk.h"
#include "ast/Command.h"
#include "ast/Repeat.h"
#include "ast/Script.h"
#include "parser/Parser.h"
#include "runtime/Variables.h"
#include "runtime/Value.h"
#include "runtime/Function.h"
#include "runtime/Message.h"
#include "runtime/Error.h"

#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

using namespace ast;

class Object;
class Runtime;

struct RuntimeConfig {
    std::ostream &stdout = std::cout;
    std::ostream &stderr = std::cerr;
    std::istream &stdin = std::cin;

    RuntimeConfig()
        : stdout(std::cout), stderr(std::cerr), stdin(std::cin) {}

    RuntimeConfig(std::ostream &out, std::ostream &err, std::istream &in)
        : stdout(out), stderr(err), stdin(in) {}    

    std::function<float()> random;

#if defined(DEBUG)
    bool enableTracing = false;
#endif
};

struct RuntimeStackFrame {
    RuntimeMessage message;
    Strong<Object> target;

    Variables variables;
    Set<std::string> globals;
    
    Value returningValue;
    Value resultValue;

    bool skippingRepeat = false;
    bool exitingRepeat = false;
    bool returning = false;
    bool passing = false;
    bool exiting = false;

    RuntimeStackFrame(const RuntimeMessage &m, const Strong<Object> &t = nullptr) : message(m), target(t) {}
};

class Runtime : StatementVisitor, ExpressionVisitor, CommandVisitor {
    RuntimeConfig config;

    // State information
    std::stack<RuntimeStackFrame> stack;
    Variables globals;

    // Runtime functions
    Map<std::string, Owned<RuntimeFunction>> functions;

  public:

    Runtime(const RuntimeConfig &c = RuntimeConfig());

    bool send(const RuntimeMessage &message, Strong<Object> target = nullptr);
    Value call(const RuntimeMessage &message, Strong<Object> target = nullptr);

    void add(const std::string &name, RuntimeFunction *fn);

  private:

    void set(const std::string &name, const Value &value);
    Value get(const std::string &name) const;

    void execute(const ast::Handler &handler, const std::vector<Value> &arguments);
    void execute(const ast::StatementList &statements);

    runtime::Value evaluateFunction(const RuntimeMessage &message);

#if defined(DEBUG)
    void trace(const std::string &msg) const;
#endif

#pragma mark - StatementVisitor

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
    void visit(const Do &) override;
    void visit(const Command &) override;

#pragma mark CommandVisitor

    void perform(const Put &) override;
    void perform(const Get &) override;
    void perform(const Ask &) override;
    void perform(const Add &) override;
    void perform(const Subtract &) override;
    void perform(const Multiply &) override;
    void perform(const Divide &) override;

#pragma mark - ExpressionVisitor

    Value valueOf(const Identifier &) override;
    Value valueOf(const FunctionCall &) override;
    Value valueOf(const Property &) override;
    Value valueOf(const Descriptor &) override;
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

    friend ResultFunction;
    friend ParamFunction;
    friend ParamCountFunction;
    friend ParamsFunction;
    friend RandomFunction;
    friend ValueFunction;
};

CH_RUNTIME_NAMESPACE_END
