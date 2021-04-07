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
    Message message;
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

    RuntimeStackFrame(const Message &m, const Strong<Object> &t)
        : message(m), target(t) {}
};

class Runtime : public ast::StatementVisitor, public ast::ExpressionVisitor, public ast::CommandVisitor {
  public:

    Runtime(const RuntimeConfig &c = RuntimeConfig());

    bool send(const Message &message, Strong<Object> target = nullptr);
    Value call(const Message &message, Strong<Object> target = nullptr);

    void add(const std::string &name, RuntimeFunction *fn);

    const RuntimeStackFrame& currentFrame();
    std::function<float()> random();

  private:

    void set(const std::string &name, const Value &value);
    Value get(const std::string &name) const;

    void execute(const ast::Handler &handler, const std::vector<Value> &arguments);
    void execute(const ast::StatementList &statements);

    Value evaluateFunction(const Message &message);

#if defined(DEBUG)
    void trace(const std::string &msg) const;
#endif

#pragma mark - StatementVisitor

    void visit(const ast::If &) override;
    void visit(const ast::Repeat &) override;
    void visit(const ast::RepeatCount &s) override;
    void visit(const ast::RepeatRange &s) override;
    void visit(const ast::RepeatCondition &s) override;
    void visit(const ast::ExitRepeat &) override;
    void visit(const ast::NextRepeat &) override;
    void visit(const ast::Exit &) override;
    void visit(const ast::Pass &) override;
    void visit(const ast::Global &) override;
    void visit(const ast::Return &) override;
    void visit(const ast::Do &) override;
    void visit(const ast::Command &) override;

#pragma mark CommandVisitor

    void perform(const ast::Put &) override;
    void perform(const ast::Get &) override;
    void perform(const ast::Ask &) override;
    void perform(const ast::Add &) override;
    void perform(const ast::Subtract &) override;
    void perform(const ast::Multiply &) override;
    void perform(const ast::Divide &) override;

#pragma mark - ExpressionVisitor

    Value valueOf(const ast::Identifier &) override;
    Value valueOf(const ast::FunctionCall &) override;
    Value valueOf(const ast::Property &) override;
    Value valueOf(const ast::Descriptor &) override;
    Value valueOf(const ast::BinaryOp &) override;
    Value valueOf(const ast::Not &) override;
    Value valueOf(const ast::Minus &) override;
    Value valueOf(const ast::FloatLiteral &) override;
    Value valueOf(const ast::IntLiteral &) override;
    Value valueOf(const ast::StringLiteral &) override;
    Value valueOf(const ast::RangeChunk &) override;
    Value valueOf(const ast::AnyChunk &) override;
    Value valueOf(const ast::LastChunk &) override;
    Value valueOf(const ast::MiddleChunk &) override;

  private:
    RuntimeConfig _config;

    Map<std::string, Owned<RuntimeFunction>> _functions;

    std::stack<RuntimeStackFrame> _stack;
    Variables _globals;
};

CH_RUNTIME_NAMESPACE_END
