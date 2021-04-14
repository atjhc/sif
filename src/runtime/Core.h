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
#include "ast/Program.h"
#include "ast/Node.h"
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
class Core;

struct CoreConfig {
    std::ostream &stdout = std::cout;
    std::ostream &stderr = std::cerr;
    std::istream &stdin = std::cin;

    std::function<float()> random = defaultRandom();

#if defined(DEBUG)
    bool enableTracing = false;
#endif

    CoreConfig()
        : stdout(std::cout), stderr(std::cerr), stdin(std::cin) {}

    CoreConfig(std::ostream &out, std::ostream &err, std::istream &in)
        : stdout(out), stderr(err), stdin(in) {}

    static std::function<float()> defaultRandom();
};

struct CoreStackFrame {
    Message message;
    Strong<Object> target;

    Variables locals;
    Set<std::string> globals;
    
    Value returningValue;
    Value resultValue;

    bool skippingRepeat = false;
    bool exitingRepeat = false;
    bool returning = false;
    bool passing = false;
    bool exiting = false;

    CoreStackFrame(const Message &m, const Strong<Object> &t)
        : message(m), target(t) {}
};

class Core : public ast::AnyVisitor {
  public:

    Core(const CoreConfig &c = CoreConfig());

    bool send(const Message &message, Strong<Object> target = nullptr);
    Value call(const Message &message, Strong<Object> target = nullptr);

    void add(const std::string &name, Function *fn);

    const CoreStackFrame& currentFrame();
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

#pragma mark - Unused

    std::any visitAny(const ast::Program &) override;
    std::any visitAny(const ast::Handler &) override;
    std::any visitAny(const ast::StatementList &) override;
    std::any visitAny(const ast::IdentifierList &) override;
    std::any visitAny(const ast::ExpressionList &) override;

#pragma mark - StatementVisitor

    std::any visitAny(const ast::If &) override;
    std::any visitAny(const ast::Repeat &) override;
    std::any visitAny(const ast::RepeatCount &) override;
    std::any visitAny(const ast::RepeatRange &) override;
    std::any visitAny(const ast::RepeatCondition &) override;
    std::any visitAny(const ast::ExitRepeat &) override;
    std::any visitAny(const ast::NextRepeat &) override;
    std::any visitAny(const ast::Exit &) override;
    std::any visitAny(const ast::Pass &) override;
    std::any visitAny(const ast::Global &) override;
    std::any visitAny(const ast::Return &) override;
    std::any visitAny(const ast::Do &) override;
    std::any visitAny(const ast::Command &) override;

#pragma mark CommandVisitor

    std::any visitAny(const ast::Put &) override;
    std::any visitAny(const ast::Get &) override;
    std::any visitAny(const ast::Ask &) override;
    std::any visitAny(const ast::Add &) override;
    std::any visitAny(const ast::Subtract &) override;
    std::any visitAny(const ast::Multiply &) override;
    std::any visitAny(const ast::Divide &) override;

#pragma mark - ExpressionVisitor

    std::any visitAny(const ast::Identifier &) override;
    std::any visitAny(const ast::FunctionCall &) override;
    std::any visitAny(const ast::Property &) override;
    std::any visitAny(const ast::Descriptor &) override;
    std::any visitAny(const ast::Binary &) override;
    std::any visitAny(const ast::Logical &) override;
    std::any visitAny(const ast::Unary &) override;
    std::any visitAny(const ast::FloatLiteral &) override;
    std::any visitAny(const ast::IntLiteral &) override;
    std::any visitAny(const ast::StringLiteral &) override;
    std::any visitAny(const ast::RangeChunk &) override;
    std::any visitAny(const ast::AnyChunk &) override;
    std::any visitAny(const ast::LastChunk &) override;
    std::any visitAny(const ast::MiddleChunk &) override;

  private:
    CoreConfig _config;

    Map<std::string, Owned<Function>> _functions;

    std::stack<CoreStackFrame> _stack;
    Variables _globals;
};

CH_RUNTIME_NAMESPACE_END
