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
#include "runtime/Object.h"
#include "utilities/chunk.h"
#include "ast/Property.h"
#include "ast/Descriptor.h"

#include <cmath>

CH_RUNTIME_NAMESPACE_BEGIN

#if !defined(DEBUG)
    #define trace(x)
#endif

Runtime::Runtime(const RuntimeConfig &c)
    : config(c) {

    add("sin", new OneArgumentFunction<sinf>());
    add("cos", new OneArgumentFunction<cosf>());
    add("tan", new OneArgumentFunction<tanf>());
    add("atan", new OneArgumentFunction<atanf>());
    add("abs", new OneArgumentFunction<fabsf>());
    add("exp", new OneArgumentFunction<expf>());
    add("exp2", new OneArgumentFunction<exp2f>());
    add("log2", new OneArgumentFunction<log2f>());
    add("log10", new OneArgumentFunction<log10f>());
    add("ln", new OneArgumentFunction<logf>());
    add("round", new OneArgumentFunction<roundf>());
    add("sqrt", new OneArgumentFunction<sqrtf>());
    add("trunc", new OneArgumentFunction<truncf>());

    add("max", new MaxFunction());
    add("min", new MinFunction());
    add("sum", new SumFunction());
    add("average", new MeanFunction());    
    add("length", new LengthFunction());
    add("offset", new OffsetFunction());
    add("random", new RandomFunction());
    add("params", new ParamsFunction());
    add("paramCount", new ParamCountFunction());
    add("param", new ParamFunction());    
    add("result", new ResultFunction());
    add("value", new ValueFunction());

    // TODO: Add these missing functions
    // runtime.add("seconds", new OneArgumentFunction<float(float)>(roundf));
    // runtime.add("ticks", new OneArgumentFunction<float(float)>(roundf));
    // runtime.add("time", new OneArgumentFunction<float(float)>(roundf));
    // runtime.add("ln1", new OneArgumentFunction<float(float)>(log2f));
    // runtime.add("exp1", new OneArgumentFunction<float(float)>(expf));
    // runtime.add("annuity", new OneArgumentFunction<float(float)>(fabs));
    // runtime.add("charToNum", new OneArgumentFunction<float(float)>(fabs));
    // runtime.add("numToChar", new OneArgumentFunction<float(float)>(fabs));
    // runtime.add("compound", new OneArgumentFunction<float(float)>(fabs));
}

bool Runtime::send(const RuntimeMessage &message, Strong<Object> target) {
    trace(std::string("send(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return false;
    }

    bool passing = true;
    auto handler = target->handlerFor(message);
    if (handler.has_value()) {
        stack.push(RuntimeStackFrame(message, target));
        execute(*handler, message.arguments);
        passing = stack.top().passing;
        auto resultValue = stack.top().returningValue;

        stack.pop();
        if (stack.size()) {
            stack.top().resultValue = resultValue;
        }
    }

    bool handled = true;
    if (passing) {
        if (target) {
            target = target->parent();
        }
        handled = send(message, target);
    }

    return handled;
}

Value Runtime::call(const RuntimeMessage &message, Strong<Object> target) {
    trace(std::string("call(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return evaluateFunction(message);
    }

    Value result;
    bool passing = true;

    auto handler = target->functionFor(message);
    if (handler.has_value()) {
        stack.push(RuntimeStackFrame(message.name, target));
        execute(*handler, message.arguments);
        passing = stack.top().passing;
        result = stack.top().returningValue;
        stack.pop();
    }

    if (passing) {
        return call(message, target->parent());
    }

    return result;
}

#pragma mark - Private

void Runtime::add(const std::string &name, RuntimeFunction *fn) {
    functions[lowercase(name)] = Owned<RuntimeFunction>(fn);
}

void Runtime::set(const std::string &name, const Value &value) {
    const auto &globalNames = stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
        globals.set(name, value);
        return;
    }
    return stack.top().variables.set(name, value);
}

Value Runtime::get(const std::string &name) const {
    const auto &globalNames = stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
        return globals.get(name);
    }
    return stack.top().variables.get(name);
}

void Runtime::execute(const ast::Handler &handler, const std::vector<Value> &values) {
    if (handler.statements == nullptr) {
        return;
    }

    std::vector<std::string> argumentNames;
    if (handler.arguments) {
        for (auto &argument : handler.arguments->identifiers) {
            argumentNames.push_back(argument->name);
        }
    }

    stack.top().variables.insert(argumentNames, values);
    execute(*handler.statements);
}

void Runtime::execute(const ast::StatementList &statements) {
    for (auto &statement : statements.statements) {
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

void Runtime::report(const RuntimeError &error) const {
    auto lineNumber = error.where.lineNumber;

    config.stderr << stack.top().target->name() << ":" << lineNumber << ": runtime error: ";
    config.stderr << error.what() << std::endl;
}

#if defined(DEBUG)
void Runtime::trace(const std::string &msg) const {
    if (config.enableTracing) {
        config.stdout << "runtime: " << msg << std::endl;
    }
}
#endif

#pragma mark - StatementVisitor

void Runtime::visit(const If &s) {
    auto condition = s.condition->evaluate(*this);
    if (condition.asBool()) {
        execute(*s.ifStatements);
    } else if (s.elseStatements) {
        execute(*s.elseStatements);
    }
}

void Runtime::visit(const Repeat &s) {
    while (true) {
        execute(*s.statements);
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
        execute(*s.statements);
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
        execute(*s.statements);
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
        execute(*s.statements);
        if (stack.top().exitingRepeat) {
            stack.top().exitingRepeat = false;
            break;
        }
        conditionValue = s.condition->evaluate(*this).asBool();
    }
}

void Runtime::visit(const ExitRepeat &) { stack.top().exitingRepeat = true; }

void Runtime::visit(const NextRepeat &) { stack.top().skippingRepeat = true; }

void Runtime::visit(const Exit &s) {
    trace("exit(" + s.messageKey->name + ")");
    if (s.messageKey->name == stack.top().message.name) {
        stack.top().exiting = true;
    } else {
        throw RuntimeError("Unexpected identifier " + s.messageKey->name, s.location);
    }
}

void Runtime::visit(const Pass &s) {
    trace("pass(" + s.messageKey->name + ")");
    if (s.messageKey->name == stack.top().message.name) {
        stack.top().passing = true;
    } else {
        throw RuntimeError("Unexpected identifier " + s.messageKey->name, s.location);
    }
}

void Runtime::visit(const Global &s) {
    Set<std::string> globals;
    for (auto &identifier : s.variables->identifiers) {
        globals.insert(identifier->name);
    }
    trace("global(" + describe(globals) + ")");
    stack.top().globals.insert(globals.begin(), globals.end());
}

void Runtime::visit(const Return &s) {
    stack.top().returning = true;
    if (s.expression) {
        auto value = s.expression->evaluate(*this);
        stack.top().returningValue = value;
    }
}

void Runtime::visit(const Command &c) {
    auto message = RuntimeMessage(c.name->name);
    if (c.arguments) {
        for (auto &expression : c.arguments->expressions) {
            message.arguments.push_back(expression->evaluate(*this));
        }
    }

    bool handled = send(message, stack.top().target);
    if (!handled) {
        c.perform(*this);
    }
}

#pragma mark - Commands

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
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.expression->location);
    }

    set(targetName, targetValue.asFloat() + value.asFloat());
}

void Runtime::perform(const Subtract &c) {
    auto &targetName = c.destination->name;

    auto value = c.expression->evaluate(*this);
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() - value.asFloat());
}

void Runtime::perform(const Multiply &c) {
    auto &targetName = c.destination->name;

    auto value = c.expression->evaluate(*this);
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() * value.asFloat());
}

void Runtime::perform(const Divide &c) {
    auto value = c.expression->evaluate(*this);
    auto &targetName = c.destination->name;
    auto targetValue = get(targetName);
    if (!targetValue.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("Expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() / value.asFloat());
}

#pragma mark - Functions

Value Runtime::evaluateFunction(const RuntimeMessage &message) {
    auto fn = functions.find(lowercase(message.name));
    if (fn != functions.end()) {
        return fn->second->valueOf(*this, message);
    }
    return Value();
}

#pragma mark - ExpressionVisitor

Value Runtime::valueOf(const Identifier &e) { return get(e.name); }

Value Runtime::valueOf(const FunctionCall &e) {
    auto message = RuntimeMessage(e.identifier->name);
    if (e.arguments) {
        for (auto &argument : e.arguments->expressions) {
            auto value = argument->evaluate(*this);
            message.arguments.push_back(value);
        }
    }

    return call(message, stack.top().target);
}

Value Runtime::valueOf(const Property &p) {
    auto message = RuntimeMessage(p.name->name);
    if (p.expression) {
        auto value = p.expression->evaluate(*this);
        message.arguments.push_back(value);
    }

    // Property calls skip the message path.
    return call(message, nullptr);
}

Value Runtime::valueOf(const Descriptor &d) {
    auto& name = d.name->name;
    if (!d.value) {
        // Assume a variable lookup.
        return get(d.name->name);
    }

    // Check the responder chain for a function handler.
    auto message = RuntimeMessage(d.name->name);
    auto handler = stack.top().target->functionFor(message);
    if (handler.has_value()) {
        message.arguments.push_back(d.value->evaluate(*this));
        return call(message, stack.top().target);
    }

    // Check for a builtin function.
    auto fn = functions.find(lowercase(message.name));
    if (fn != functions.end()) {
        message.arguments.push_back(d.value->evaluate(*this));
        return fn->second->valueOf(*this, message);
    }

    // TODO: Find an object using the descriptor.
    throw RuntimeError("unrecognized descriptor '" + name + "'", d.location);
}

Value Runtime::valueOf(const BinaryOp &e) {
    auto lhs = e.left->evaluate(*this);
    auto rhs = e.right->evaluate(*this);

    switch (e.op) {
    case BinaryOp::Equal:
        return lhs == rhs;
    case BinaryOp::NotEqual:
        return lhs != rhs;
    case BinaryOp::LessThan:
        return lhs < rhs;
    case BinaryOp::GreaterThan:
        return lhs > rhs;
    case BinaryOp::LessThanOrEqual:
        return lhs <= rhs;
    case BinaryOp::GreaterThanOrEqual:
        return lhs >= rhs;
    case BinaryOp::Plus:
        return lhs + rhs;
    case BinaryOp::Minus:
        return lhs - rhs;
    case BinaryOp::Multiply:
        return lhs * rhs;
    case BinaryOp::Divide:
        return lhs / rhs;
    case BinaryOp::Exponent:
        return lhs ^ rhs;
    case BinaryOp::IsIn:
        return rhs.contains(lhs);
    case BinaryOp::Contains:
        return lhs.contains(rhs);
    case BinaryOp::Or:
        return lhs || rhs;
    case BinaryOp::And:
        return lhs && rhs;
    case BinaryOp::Mod:
        return lhs % rhs;
    case BinaryOp::Concat:
        return lhs.value + rhs.value;
    case BinaryOp::ConcatWithSpace:
        return lhs.value + " " + rhs.value;
    }
}

Value Runtime::valueOf(const Not &e) { return Value(!e.expression->evaluate(*this).asBool()); }

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

Value Runtime::valueOf(const FloatLiteral &e) { return Value(e.value); }

Value Runtime::valueOf(const IntLiteral &e) { return Value(e.value); }

Value Runtime::valueOf(const StringLiteral &e) { return Value(e.value); }

static type chunk_type(Chunk::Type t) {
    switch (t) {
    case Chunk::Char:
        return character;
    case Chunk::Word:
        return word;
    case Chunk::Item:
        return item;
    case Chunk::Line:
        return line;
    }
}

Value Runtime::valueOf(const RangeChunk &c) {
    auto value = c.expression->evaluate(*this);
    auto startValue = c.start->evaluate(*this);

    if (c.end) {
        auto endValue = c.end->evaluate(*this);
        return Value(range_chunk(chunk_type(c.type), startValue.asInteger() - 1,
                                 endValue.asInteger() - 1, value.value)
                         .get());
    } else {
        return Value(chunk(chunk_type(c.type), startValue.asInteger() - 1, value.value).get());
    }
}

Value Runtime::valueOf(const AnyChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(
        random_chunk(
            chunk_type(c.type), [this](int count) { return config.random() * count; }, value.value)
            .get());
}

Value Runtime::valueOf(const LastChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(last_chunk(chunk_type(c.type), value.value).get());
}

Value Runtime::valueOf(const MiddleChunk &c) {
    auto value = c.expression->evaluate(*this);
    return Value(middle_chunk(chunk_type(c.type), value.value).get());
}

CH_RUNTIME_NAMESPACE_END
