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

#include "runtime/Runtime.h"
#include "chunk.h"

CH_NAMESPACE_BEGIN

Runtime::Runtime(const std::string &n, std::unique_ptr<Script> &s, RuntimeConfig c)
    : config(c), name(n), script(std::move(s)) {

    for (auto& handler : script->handlers) {
        auto& name = handler->messageKey->name;

        auto map = &handlersByName;
        if (handler->kind == Handler::FunctionKind) {
            map = &functionsByName;
        }

        auto i = map->find(name);
        if (i != map->end()) {
            throw RuntimeError("Redefinition of handler " + name, handler->location);
        }

        map->insert({{lowercase(name), handler}});
    }
}

bool Runtime::send(const std::string &name, const std::vector<Value> &arguments) {
    auto normalizedName = lowercase(name);
    auto i = handlersByName.find(normalizedName);
    if (i == handlersByName.end()) {
        // throw RuntimeError("Unrecognized handler \"" + name + "\"", Location());
        return true;
    } 

    stack.push(RuntimeStackFrame(name));
    execute(i->second.get(), arguments);
    bool passing = stack.top().passing;
    stack.pop();

    return passing;
}

Value Runtime::call(const std::string &name, const std::vector<Value> &arguments) {
    auto normalizedName = lowercase(name);
    auto i = functionsByName.find(normalizedName);
    if (i == functionsByName.end()) {
        throw RuntimeError("Call to unknown function \"" + name + "\"", Location());
    }

    stack.push(RuntimeStackFrame(name));
    execute(i->second.get(), arguments);
    auto result = stack.top().returningValue;
    stack.pop();

    return result;
}

#pragma mark - Private

void Runtime::set(const std::string &name, const Value &value) {
    const auto &i = stack.top().globals.find(name);
    if (i != stack.top().globals.end()) {
        return globals.set(name, value);
    }
    return stack.top().variables.set(name, value);

}

Value Runtime::get(const std::string &name) const {
    const auto &i = stack.top().globals.find(name);
    if (i != stack.top().globals.end()) {
        return globals.get(name);
    }
    return stack.top().variables.get(name);
}

void Runtime::execute(const std::unique_ptr<ast::Handler> &handler, const std::vector<Value> &values) {
    std::vector<std::string> argumentNames;
    if (handler->arguments) {
        for (auto &argument : handler->arguments->identifiers) {
            argumentNames.push_back(argument->name);
        }
    }

    stack.top().variables.insert(argumentNames, values);
    execute(handler.get()->statements);
}

void Runtime::execute(const std::unique_ptr<ast::StatementList> &statements) {
    for (auto& statement : statements->statements) {
        try {
            statement->accept(*this);
        } catch (RuntimeError &error) {
            report(error);
            return;
        }

        auto &frame = stack.top();
        if (frame.passing || frame.exiting || frame.returning) {
            break;
        }
    }
}

void Runtime::report(const RuntimeError &error) {
    auto lineNumber = error.where.lineNumber;
    
    config.stderr << name << ":" << lineNumber << ": runtime error: ";
    config.stderr << error.what() << std::endl;
}

#pragma mark - StatementVisitor

void Runtime::visit(const If &s) {
    auto condition = s.condition->evaluate(*this);
    if (condition.asBool()) {
        execute(s.ifStatements);
    } else if (s.elseStatements) {
        execute(s.elseStatements);
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
    if (e.messageKey->name == stack.top().name) {
        stack.top().exiting = true;
    } else {
        throw RuntimeError("Unexpected identifier " + e.messageKey->name, e.location);
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

void Runtime::visit(const Command &c) {
    auto name = lowercase(c.name->name);

    // Assume we are passing until we find a handler.
    bool passed = true;

    std::vector<Value> arguments;
    if (c.arguments) {
        for (auto &expression : c.arguments->expressions) {
            arguments.push_back(expression->evaluate(*this));
        }
    }
    passed = send(name, arguments);


    if (passed) {
        c.perform(*this);
    }
}

#pragma mark - Commands

void Runtime::perform(const Command &s) {
    /* no-op */
}

void Runtime::perform(const Put &s) {
    auto value = s.expression->evaluate(*this);
    if (s.target) {
        auto &name = s.target->name;
        switch (s.preposition->type) {
        case Preposition::Before: {
            auto targetValue = get(name);
            set(name, value.asString() + targetValue.asString());
            break;
        }
        case Preposition::After: {
            auto targetValue = get(name);
            set(name, targetValue.asString() + value.asString());
            break;
        }
        case Preposition::Into:
            set(name, value);
            break;
        }
    } else {
        config.stdout << value.asString() << std::endl;
    }
}

void Runtime::perform(const Get &s) {
    auto result = s.expression->evaluate(*this);
    stack.top().variables.set("it", result);
}

void Runtime::perform(const Ask &s) {
    auto question = s.expression->evaluate(*this);
    
    config.stdout << question.value;
    std::string result;
    std::getline(config.stdin, result);

    stack.top().variables.set("it", result);
}

void Runtime::perform(const Add &c) {
    auto &targetName = c.destination->name;

    auto value = c.expression->evaluate(*this);
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.expression->location);
    }

    set(targetName, targetValue.asFloat() + value.asFloat());
}

void Runtime::perform(const Subtract &c) {
    auto &targetName = c.destination->name;

    auto value = c.expression->evaluate(*this);
    auto targetValue = get(targetName);
    
    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.expression->location);
    }
    set(targetName, targetValue.asFloat() - value.asFloat());
}

void Runtime::perform(const Multiply &c) {
    auto &targetName = c.destination->name;

    auto value = c.expression->evaluate(*this);
    auto targetValue = get(targetName);
    
    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.expression->location);
    }
    set(targetName, targetValue.asFloat() * value.asFloat());
}

void Runtime::perform(const Divide &c) {
    auto value = c.expression->evaluate(*this);
    auto &targetName = c.destination->name;
    auto targetValue = get(targetName);
    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(), c.expression->location);
    }
    set(targetName, targetValue.asFloat() / value.asFloat());
}

#pragma mark - ExpressionVisitor

Value Runtime::valueOf(const Identifier &e) {
    return get(e.name);
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
        throw RuntimeError("Expected number; got \"" + value.asString() + "\"", e.location);
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

static type chunk_type(Chunk::Type t) {
    switch (t) {
    case Chunk::Char:   return character;
    case Chunk::Word:   return word;
    case Chunk::Item:   return item;
    case Chunk::Line:   return line;
    }
}

Value Runtime::valueOf(const RangeChunk &c) {
    auto value = c.expression->evaluate(*this);
    auto startValue = c.start->evaluate(*this);

    if (c.end) {
        auto endValue = c.end->evaluate(*this);
        return Value(range_chunk(chunk_type(c.type), startValue.asInteger() - 1, endValue.asInteger() - 1, value.value).get());
    } else {
        return Value(chunk(chunk_type(c.type), startValue.asInteger() - 1, value.value).get());
    }
}

Value Runtime::valueOf(const AnyChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(random_chunk(chunk_type(c.type), [this](int count) { return config.random() * count; }, value.value).get());
}

Value Runtime::valueOf(const LastChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(last_chunk(chunk_type(c.type), value.value).get());
}

Value Runtime::valueOf(const MiddleChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(middle_chunk(chunk_type(c.type), value.value).get());
}

CH_NAMESPACE_END
