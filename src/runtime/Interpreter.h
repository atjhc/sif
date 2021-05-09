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
#include "ast/Node.h"
#include "ast/Program.h"
#include "ast/Repeat.h"
#include "parser/Parser.h"
#include "runtime/Environment.h"
#include "runtime/Error.h"
#include "runtime/Function.h"
#include "runtime/Message.h"
#include "runtime/Value.h"
#include "runtime/Names.h"
#include "runtime/Descriptor.h"

#include <iostream>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

CH_RUNTIME_NAMESPACE_BEGIN

class Object;
class Interpreter;

struct InterpreterConfig {
    std::ostream &stdout = std::cout;
    std::ostream &stderr = std::cerr;
    std::istream &stdin = std::cin;

    std::function<float()> random = defaultRandom();

#if defined(DEBUG)
    bool enableTracing = false;
#endif

    InterpreterConfig() : stdout(std::cout), stderr(std::cerr), stdin(std::cin) {}

    InterpreterConfig(std::ostream &out, std::ostream &err, std::istream &in)
        : stdout(out), stderr(err), stdin(in) {}

    static std::function<float()> defaultRandom();
};

struct InterpreterStackFrame {
    Message message;
    Strong<Object> target;

    Environment locals;
    Set<std::string> globals;

    Value returningValue;
    Value resultValue;

    bool skippingRepeat = false;
    bool exitingRepeat = false;
    bool returning = false;
    bool passing = false;
    bool exiting = false;

    InterpreterStackFrame(const Message &m, const Strong<Object> &t) : message(m), target(t) {}
};

class Interpreter : public ast::Statement::Visitor, public ast::Expression::Visitor<Value> {
  public:
    using ObjectFactory = std::function<Strong<Object>(Value)>;
    using Validator = std::function<void(Value)>;

    Interpreter(const InterpreterConfig &c = InterpreterConfig());

    bool send(const Message &message, Strong<Object> target = nullptr);
    Optional<Value> call(const Message &message, Strong<Object> target = nullptr);

    Value evaluate(const ast::Expression &expression);
    Value evaluate(Descriptor &descriptor);
    Value evaluateBuiltin(const Names &name, const std::vector<Value> &arguments);

    void addBuiltin(const Names &name, Function *fn);
    void addFactory(const Names &name, const ObjectFactory &factory);

    Optional<Value> valueForProperty(Names property) const;

    const InterpreterStackFrame &currentFrame();
    std::function<float()> random();

  private:
    void set(const std::string &name, const Value &value);
    Optional<Value> get(const std::string &name) const;

    void execute(const ast::Handler &handler, const std::vector<Value> &arguments);
    void execute(const ast::StatementList &statements);

#if defined(DEBUG)
    void trace(const std::string &msg) const;
#endif

    void performArith(const Owned<ast::Expression> &expression,
                      const Owned<ast::Expression> &container,
                      const std::function<Value(Value, Value)> &fn);

#pragma mark - Statement::Visitor

    void visit(const ast::If &) override;
    void visit(const ast::Repeat &) override;
    void visit(const ast::RepeatCount &) override;
    void visit(const ast::RepeatRange &) override;
    void visit(const ast::RepeatCondition &) override;
    void visit(const ast::ExitRepeat &) override;
    void visit(const ast::NextRepeat &) override;
    void visit(const ast::Exit &) override;
    void visit(const ast::Pass &) override;
    void visit(const ast::Global &) override;
    void visit(const ast::Return &) override;
    void visit(const ast::Do &) override;
    void visit(const ast::Command &) override;
    void visit(const ast::Put &) override;
    void visit(const ast::Get &) override;
    void visit(const ast::Set &) override;
    void visit(const ast::Ask &) override;
    void visit(const ast::Add &) override;
    void visit(const ast::Subtract &) override;
    void visit(const ast::Multiply &) override;
    void visit(const ast::Divide &) override;
    void visit(const ast::Delete &) override;

#pragma mark - Expression::Visitor<Value>

    Value visit(const ast::Identifier &) override;
    Value visit(const ast::FunctionCall &) override;
    Value visit(const ast::Property &) override;
    Value visit(const ast::Descriptor &) override;
    Value visit(const ast::Binary &) override;
    Value visit(const ast::Logical &) override;
    Value visit(const ast::Unary &) override;
    Value visit(const ast::ChunkExpression &) override;
    Value visit(const ast::CountExpression &) override;
    Value visit(const ast::FloatLiteral &) override;
    Value visit(const ast::IntLiteral &) override;
    Value visit(const ast::StringLiteral &) override;
    
  private:
    InterpreterConfig _config;

    Map<Names, Owned<Function>> _functions;
    Map<Names, ObjectFactory> _factories;
    Map<Names, Validator> _propertyValidators;

    Map<Names, Value> _properties;
    std::stack<InterpreterStackFrame> _stack;
    Environment _globals;
};

CH_RUNTIME_NAMESPACE_END
