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

#include "Runtime.h"

#include <iomanip>

CH_NAMESPACE_BEGIN

Runtime::Runtime(std::unique_ptr<Script> &s)
    : script(std::move(s)) {

    for (auto& handler : script->handlers) {
        auto& name = handler->messageKey->name;

        auto map = &handlersByName;
        if (handler->kind == Handler::FunctionKind) {
            map = &functionsByName;
        }

        auto i = map->find(name);
        if (i != map->end()) {
            // TODO: error redefinition of handler
        }
        map->insert({{lowercase(name), handler}});
    }
}

void Runtime::run(const std::string &name, const std::vector<Value> &arguments) {
    auto normalizedName = lowercase(name);
    auto i = handlersByName.find(normalizedName);
    if (i == handlersByName.end()) {
        config.stderr << "could not find handler named " << name << std::endl; 
        return;
    } 

    stack.push(RuntimeStackFrame(name));
    execute(i->second.get(), arguments);
    stack.pop();
}

Value Runtime::call(const std::string &name, const std::vector<Value> &arguments) {
    auto normalizedName = lowercase(name);
    auto i = functionsByName.find(normalizedName);
    if (i == functionsByName.end()) {
        throw std::runtime_error("could not find function named \"" + name + "\"");
    }

    stack.push(RuntimeStackFrame(name));
    execute(i->second.get(), arguments);
    auto result = stack.top().returningValue;
    stack.pop();

    return result;
}

#pragma mark - Private

void Runtime::execute(const std::unique_ptr<ast::Handler> &handler, const std::vector<Value> &values) {
    std::vector<std::string> argumentNames;
    for (auto &argument : handler->arguments->identifiers) {
        argumentNames.push_back(argument->name);
    }

    stack.top().variables.insert(argumentNames, values);
    execute(handler.get()->statements);
}

void Runtime::execute(const std::unique_ptr<ast::StatementList> &statements) {
    for (auto& statement : statements->statements) {
        try {
            statement->accept(*this);
        } catch (std::runtime_error &error) {
            config.stderr << "error: " << error.what() << std::endl;
            return;
        }

        auto &frame = stack.top();
        if (frame.passing || frame.exiting || frame.returning) {
            break;
        }
    }
}

#pragma mark - StatementVisitor

void Runtime::visit(const If &e) {
    auto condition = e.condition->evaluate(*this);
    if (condition.asBool()) {
        execute(e.ifStatements);
    } else if (e.elseStatements) {
        execute(e.elseStatements);
    }
}

void Runtime::visit(const Repeat &s) {
    while (true) {
        execute(s.statements);
        if (stack.top().exitingRepeat) {
            stack.top().exitingRepeat = false;
            break;
        }
    }
}

void Runtime::visit(const RepeatCount &s) {
    auto countValue = s.countExpression->evaluate(*this);
    auto count = countValue.asInteger();
    for (int i = 0; i < count; i++) {
        execute(s.statements);
        if (stack.top().exitingRepeat) {
            stack.top().exitingRepeat = false;
            break;
        }
    }
}

void Runtime::visit(const RepeatRange &s) {
    auto iteratorName = s.variable->name;
    auto startValue = s.startExpression->evaluate(*this).asInteger();
    auto endValue = s.endExpression->evaluate(*this).asInteger();

    auto i = startValue;
    while ((s.ascending ? i <= endValue : i >= endValue)) {
        stack.top().variables.set(iteratorName, Value(i));
        execute(s.statements);
        if (stack.top().exitingRepeat) {
            stack.top().exitingRepeat = false;
            break;
        }
        if (s.ascending) {
            i++;
        } else {
            i--;
        }
    }
}

void Runtime::visit(const RepeatCondition &s) {
    auto conditionValue = s.condition->evaluate(*this).asBool();
    while (conditionValue == s.conditionValue) {
        execute(s.statements);
        if (stack.top().exitingRepeat) {
            stack.top().exitingRepeat = false;
            break;
        }
        conditionValue = s.condition->evaluate(*this).asBool();
    }
}

void Runtime::visit(const ExitRepeat &) {
    stack.top().exitingRepeat = true;
}

void Runtime::visit(const NextRepeat &) {
    stack.top().skippingRepeat = true;
}

void Runtime::visit(const Exit &e) {
    if (e.messageKey->name == stack.top().messageKey) {
        stack.top().exiting = true;
    } else {
        // runtime error
    }
}

void Runtime::visit(const Pass &) {
    stack.top().passing = true;
}

void Runtime::visit(const Global &s) {
    std::unordered_set<std::string> globals;
    for (auto &identifier : s.variables->identifiers) {
        globals.insert(identifier->name);
    }
    stack.top().globals = globals;
}

void Runtime::visit(const Return &s) {
    stack.top().returning = true;
    if (s.expression) {
        auto value = s.expression->evaluate(*this);
        stack.top().returningValue = value;
    }
}

void Runtime::visit(const Put &s) {
    auto value = s.expression->evaluate(*this).value;
    if (s.target) {
        switch (s.preposition->type) {
        case Preposition::Before:
            break;
        case Preposition::Into:
            stack.top().variables.set(s.target->name, value);
            break;
        case Preposition::After:
            break;
        }
    } else {
        config.stdout << value << std::endl;
    }
}

void Runtime::visit(const Get &s) {
    auto result = s.expression->evaluate(*this);
    stack.top().variables.set("it", result);
}

void Runtime::visit(const Ask &s) {
    auto question = s.expression->evaluate(*this);
    
    config.stdout << question.value;
    std::string result;
    std::getline(config.stdin, result);

    stack.top().variables.set("it", result);
}

#pragma mark - ExpressionVisitor

Value Runtime::valueOf(const Identifier &e) {
    const auto &name = e.name;
    const auto &i = stack.top().globals.find(name);
    if (i != stack.top().globals.end()) {
        return globals.get(name);
    }
    return stack.top().variables.get(name);
}

Value Runtime::valueOf(const FunctionCall &e) {
    std::vector<Value> arguments;
    if (e.arguments) {
        for (auto &argument : e.arguments->expressions) {
            auto value = argument->evaluate(*this);
            arguments.push_back(value);
        }
    }

    return call(e.identifier->name, arguments);
}

Value Runtime::valueOf(const BinaryOp &e) {
    auto lhs = e.left->evaluate(*this);
    auto rhs = e.right->evaluate(*this);

    switch (e.op) {
    case BinaryOp::Equal: return lhs == rhs;
    case BinaryOp::NotEqual: return lhs != rhs;
    case BinaryOp::LessThan: return lhs < rhs;
    case BinaryOp::GreaterThan: return lhs > rhs;
    case BinaryOp::LessThanOrEqual: return lhs <= rhs;
    case BinaryOp::GreaterThanOrEqual: return lhs >= rhs;
    case BinaryOp::Plus: return lhs + rhs;
    case BinaryOp::Minus: return lhs - rhs;
    case BinaryOp::Multiply: return lhs * rhs;
    case BinaryOp::Divide: return lhs / rhs;
    case BinaryOp::IsIn: return rhs.contains(lhs);
    case BinaryOp::Contains: return lhs.contains(rhs);
    case BinaryOp::Or: return lhs || rhs;
    case BinaryOp::And: return lhs && rhs;
    case BinaryOp::Mod: return lhs % rhs;
    case BinaryOp::Concat: return lhs.value + rhs.value;
    case BinaryOp::ConcatWithSpace: return lhs.value + " " + rhs.value;
    }
}

Value Runtime::valueOf(const Not &e) {
    return Value(!e.expression->evaluate(*this).asBool());
}

Value Runtime::valueOf(const Minus &e) {
    auto value = e.expression->evaluate(*this);
    if (value.isInteger()) {
        return Value(-e.expression->evaluate(*this).asInteger());
    } else if (value.isFloat()) {
        return Value(-e.expression->evaluate(*this).asFloat());
    } else {
        throw std::runtime_error("expected number");
    }
}

Value Runtime::valueOf(const FloatLiteral &e) {
    return Value(e.value);
}

Value Runtime::valueOf(const IntLiteral &e) {
    return Value(e.value);
}

Value Runtime::valueOf(const StringLiteral &e) {
    return Value(e.value);
}

Value Runtime::valueOf(const RangeChunk &) {
    return Value();
}

Value Runtime::valueOf(const AnyChunk &) {
    return Value();
}

Value Runtime::valueOf(const LastChunk &) {
    return Value();
}

Value Runtime::valueOf(const MiddleChunk &) {
    return Value();
}

CH_NAMESPACE_END
