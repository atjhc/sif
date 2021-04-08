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
#include "runtime/Property.h"
#include "utilities/chunk.h"
#include "utilities/devnull.h"
#include "ast/Property.h"
#include "ast/Descriptor.h"

#include <math.h>

#include <random>

CH_RUNTIME_NAMESPACE_BEGIN

using namespace ast;

std::function<float()> RuntimeConfig::defaultRandom() {
    static thread_local std::default_random_engine generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return [&]() {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        return distribution(generator);
    };
}

#if !defined(DEBUG)
    #define trace(x)
#endif

Runtime::Runtime(const RuntimeConfig &config)
    : _config(config) {

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
    add("target", new TargetFunction());

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

bool Runtime::send(const Message &message, Strong<Object> target) {
    trace(std::string("send(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return false;
    }

    bool passing = true;
    auto handler = target->handlerFor(message);
    if (handler.has_value()) {
        _stack.push(RuntimeStackFrame(message, target));
        execute(*handler, message.arguments);
        passing = _stack.top().passing;
        auto resultValue = _stack.top().returningValue;

        _stack.pop();
        if (_stack.size()) {
            _stack.top().resultValue = resultValue;
        }
    }

    bool handled = true;
    if (passing) {
        handled = send(message, target->parent());
    }

    return handled;
}

Value Runtime::call(const Message &message, Strong<Object> target) {
    trace(std::string("call(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return evaluateFunction(message);
    }

    Value result;
    bool passing = true;

    auto handler = target->functionFor(message);
    if (handler.has_value()) {
        _stack.push(RuntimeStackFrame(message.name, target));
        execute(*handler, message.arguments);
        passing = _stack.top().passing;
        result = _stack.top().returningValue;
        _stack.pop();
    }

    if (passing) {
        return call(message, target->parent());
    }

    return result;
}

const RuntimeStackFrame& Runtime::currentFrame() {
    return _stack.top();
}

std::function<float()> Runtime::random() {
    return _config.random;
}

#pragma mark - Private

void Runtime::add(const std::string &name, Function *fn) {
    _functions[lowercase(name)] = Owned<Function>(fn);
}

void Runtime::set(const std::string &name, const Value &value) {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
       _globals.set(name, value);
        return;
    }
    return _stack.top().locals.set(name, value);
}

Value Runtime::get(const std::string &name) const {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
        return _globals.get(name);
    }
    return _stack.top().locals.get(name);
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

    _stack.top().locals.insert(argumentNames, values);
    execute(*handler.statements);
}

void Runtime::execute(const ast::StatementList &statements) {
    for (auto &statement : statements.statements) {
        statement->accept(*this);

        auto &frame = _stack.top();
        if (frame.passing || frame.exiting || frame.returning) {
            break;
        }
    }
}

#if defined(DEBUG)
void Runtime::trace(const std::string &msg) const {
    if (_config.enableTracing) {
       _config.stdout << "runtime: " << msg << std::endl;
    }
}
#endif

#pragma mark - Unused

std::any Runtime::visitAny(const ast::Script &) {
    return std::any();
}

std::any Runtime::visitAny(const ast::Handler &) {
    return std::any();
}

std::any Runtime::visitAny(const ast::StatementList &) {
    return std::any();
}

std::any Runtime::visitAny(const ast::IdentifierList &) {
    return std::any();
}

std::any Runtime::visitAny(const ast::ExpressionList &) {
    return std::any();
}

#pragma mark - StatementVisitor

std::any Runtime::visitAny(const If &s) {
    auto condition = std::any_cast<Value>(s.condition->accept(*this));
    if (condition.asBool()) {
        execute(*s.ifStatements);
    } else if (s.elseStatements) {
        execute(*s.elseStatements);
    }

    return std::any();
}

std::any Runtime::visitAny(const Repeat &s) {
    while (true) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
    }

    return std::any();
}

std::any Runtime::visitAny(const RepeatCount &s) {
    auto countValue = std::any_cast<Value>(s.countExpression->accept(*this));
    auto count = countValue.asInteger();
    for (int i = 0; i < count; i++) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
    }

    return std::any();
}

std::any Runtime::visitAny(const RepeatRange &s) {
    auto iteratorName = s.variable->name;
    auto startValue = std::any_cast<Value>(s.startExpression->accept(*this)).asInteger();
    auto endValue = std::any_cast<Value>(s.endExpression->accept(*this)).asInteger();

    auto i = startValue;
    while ((s.ascending ? i <= endValue : i >= endValue)) {
        _stack.top().locals.set(iteratorName, Value(i));
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
        if (s.ascending) {
            i++;
        } else {
            i--;
        }
    }

    return std::any();
}

std::any Runtime::visitAny(const RepeatCondition &s) {
    auto conditionValue = std::any_cast<Value>(s.condition->accept(*this)).asBool();
    while (conditionValue == s.conditionValue) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
        conditionValue = std::any_cast<Value>(s.condition->accept(*this)).asBool();
    }

    return std::any();
}

std::any Runtime::visitAny(const ExitRepeat &) { 
    _stack.top().exitingRepeat = true;
    return std::any();
}

std::any Runtime::visitAny(const NextRepeat &) { 
    _stack.top().skippingRepeat = true;
    return std::any();
}

std::any Runtime::visitAny(const Exit &s) {
    trace("exit(" + s.messageKey->name + ")");
    if (s.messageKey->name == _stack.top().message.name) {
        _stack.top().exiting = true;
    } else {
        throw RuntimeError("unexpected identifier " + s.messageKey->name, s.location);
    }

    return std::any();
}

std::any Runtime::visitAny(const Pass &s) {
    trace("pass(" + s.messageKey->name + ")");
    if (s.messageKey->name == _stack.top().message.name) {
        _stack.top().passing = true;
    } else {
        throw RuntimeError("unexpected identifier " + s.messageKey->name, s.location);
    }

    return std::any();
}

std::any Runtime::visitAny(const Global &s) {
    Set<std::string> globals;
    for (auto &identifier : s.variables->identifiers) {
        globals.insert(identifier->name);
    }
    trace("global(" + describe(globals) + ")");
    _stack.top().globals.insert(globals.begin(), globals.end());

    return std::any();
}

std::any Runtime::visitAny(const Return &s) {
    _stack.top().returning = true;
    if (s.expression) {
        auto value = std::any_cast<Value>(s.expression->accept(*this));
        _stack.top().returningValue = value;
    }

    return std::any();
}

std::any Runtime::visitAny(const Do &c) {
    if (c.language) {
        auto languageName = std::any_cast<Value>(c.language->accept(*this));

        // TODO: Call out to another language.
        throw RuntimeError("unrecognized language '" + languageName.asString() + "'", c.language->location);
    }

    auto value = std::any_cast<Value>(c.expression->accept(*this));
    auto valueString = value.asString();

    Parser parser(ParserConfig("<runtime>",_config.stderr));
    Owned<StatementList> result;

    if ((result = parser.parseStatements(valueString)) == nullptr) {
        throw RuntimeError("failed to parse script", c.location);
    }
    execute(*result);

    return std::any();
}

std::any Runtime::visitAny(const Command &c) {
    auto message = Message(c.name->name);
    if (c.arguments) {
        for (auto &expression : c.arguments->expressions) {
            auto arg = std::any_cast<Value>(std::any_cast<Value>(expression->accept(*this)));
            message.arguments.push_back(arg);
        }
    }

    bool handled = send(message, _stack.top().target);
    if (!handled) {
        c.accept(*this);
    }

    return std::any();
}

#pragma mark - Commands

std::any Runtime::visitAny(const Put &s) {
    auto value = std::any_cast<Value>(s.expression->accept(*this));
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
       _config.stdout << value.asString() << std::endl;
    }

    return std::any();
}

std::any Runtime::visitAny(const Get &s) {
    auto result = std::any_cast<Value>(s.expression->accept(*this));
    _stack.top().locals.set("it", result);

    return std::any();
}

std::any Runtime::visitAny(const Ask &s) {
    auto question = std::any_cast<Value>(s.expression->accept(*this));

    _config.stdout << question.asString();
    std::string result;
    std::getline(_config.stdin, result);

    _stack.top().locals.set("it", result);

    return std::any();
}

std::any Runtime::visitAny(const Add &c) {
    auto &targetName = c.destination->name;

    auto value = std::any_cast<Value>(c.expression->accept(*this));
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.expression->location);
    }

    set(targetName, targetValue.asFloat() + value.asFloat());

    return std::any();
}

std::any Runtime::visitAny(const Subtract &c) {
    auto &targetName = c.destination->name;

    auto value = std::any_cast<Value>(c.expression->accept(*this));
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() - value.asFloat());

    return std::any();
}

std::any Runtime::visitAny(const Multiply &c) {
    auto &targetName = c.destination->name;

    auto value = std::any_cast<Value>(c.expression->accept(*this));
    auto targetValue = get(targetName);

    if (!targetValue.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() * value.asFloat());

    return std::any();
}

std::any Runtime::visitAny(const Divide &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this));
    auto &targetName = c.destination->name;
    auto targetValue = get(targetName);
    if (!targetValue.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.destination->location);
    }
    if (!value.isNumber()) {
        throw RuntimeError("expected number, got " + targetValue.asString(),
                           c.expression->location);
    }
    set(targetName, targetValue.asFloat() / value.asFloat());

    return std::any();
}

#pragma mark - Functions

Value Runtime::evaluateFunction(const Message &message) {
    auto fn = _functions.find(lowercase(message.name));
    if (fn != _functions.end()) {
        return fn->second->valueOf(*this, message);
    }
    throw RuntimeError("unrecognized handler " + message.name);
}

#pragma mark - ExpressionVisitor

std::any Runtime::visitAny(const ast::Preposition &) {
    return std::any();
}

std::any Runtime::visitAny(const Identifier &e) { return get(e.name); }

std::any Runtime::visitAny(const FunctionCall &e) {
    auto message = Message(e.identifier->name);
    if (e.arguments) {
        for (auto &argument : e.arguments->expressions) {
            auto value = std::any_cast<Value>(argument->accept(*this));
            message.arguments.push_back(value);
        }
    }

    return call(message, _stack.top().target);
}

std::any Runtime::visitAny(const ast::Property &p) {
    auto message = Message(p.name->name);
    if (p.expression) {
        auto value = std::any_cast<Value>(p.expression->accept(*this));
        if (value.isObject()) {
            auto property = runtime::Property(p);
            return value.asObject()->valueForProperty(property);
        } else {
            message.arguments.push_back(value);
        }
    }

    // Property calls skip the message path.
    return call(message, nullptr);
}

std::any Runtime::visitAny(const ast::Descriptor &d) {
    auto& name = d.name->name;
    if (!d.value) {
        auto& name = d.name->name;
        // Check for special "me" descriptor.
        if (name == "me") {
            return Value(_stack.top().target);
        }
        // Assume a variable lookup.
        return get(d.name->name);
    }

    // Check the responder chain for a function handler.
    auto message = Message(d.name->name);
    auto handler = _stack.top().target->functionFor(message);
    if (handler.has_value()) {
        auto arg = std::any_cast<Value>(d.value->accept(*this));
        message.arguments.push_back(arg);
        return call(message, _stack.top().target);
    }

    // Check for a builtin function.
    auto fn = _functions.find(lowercase(message.name));
    if (fn != _functions.end()) {
        auto arg = std::any_cast<Value>(d.value->accept(*this));
        message.arguments.push_back(arg);
        return fn->second->valueOf(*this, message);
    }

    // TODO: Find an object using the descriptor.
    throw RuntimeError("unrecognized descriptor '" + name + "'", d.location);
}

std::any Runtime::visitAny(const BinaryOp &e) {
    auto lhs = std::any_cast<Value>(e.left->accept(*this));
    auto rhs = std::any_cast<Value>(e.right->accept(*this));
    
    if (e.op == BinaryOp::IsAn) {
        auto typeName = rhs.asString();
        if (typeName == "number") {
            return Value(lhs.isNumber());
        }
        if (typeName == "integer") {
            return Value(lhs.isInteger());
        }
        if (typeName == "logical") {
            return Value(lhs.isBool());
        }
        // TODO: Hack since "empty" is a constant for empty string.
        if (typeName == "empty" || typeName == "") {
            return Value(lhs.isEmpty());
        }
        // TODO: Check for additional host defined types.
        throw RuntimeError("unknown type name '" + rhs.asString() + "'", e.right->location);
    }

    switch (e.op) {
    case BinaryOp::Equal:
        return Value(lhs == rhs);
    case BinaryOp::NotEqual:
        return Value(lhs != rhs);
    case BinaryOp::LessThan:
        return Value(lhs < rhs);
    case BinaryOp::GreaterThan:
        return Value(lhs > rhs);
    case BinaryOp::LessThanOrEqual:
        return Value(lhs <= rhs);
    case BinaryOp::GreaterThanOrEqual:
        return Value(lhs >= rhs);
    case BinaryOp::Plus:
        return Value(lhs + rhs);
    case BinaryOp::Minus:
        return Value(lhs - rhs);
    case BinaryOp::Multiply:
        return Value(lhs * rhs);
    case BinaryOp::Divide:
        return Value(lhs / rhs);
    case BinaryOp::Exponent:
        return Value(lhs ^ rhs);
    case BinaryOp::IsIn:
        return Value(rhs.contains(lhs));
    case BinaryOp::Contains:
        return Value(lhs.contains(rhs));
    case BinaryOp::Or:
        return Value(lhs || rhs);
    case BinaryOp::And:
        return Value(lhs && rhs);
    case BinaryOp::Mod:
        return Value(lhs % rhs);
    case BinaryOp::Concat:
        return Value(lhs.asString() + rhs.asString());
    case BinaryOp::ConcatWithSpace:
        return Value(lhs.asString() + " " + rhs.asString());
    default:
        throw RuntimeError("unexpected operator", e.location);
    }
}

std::any Runtime::visitAny(const Not &e) { 
    auto value = std::any_cast<Value>(e.expression->accept(*this));
    return Value(!value.asBool());
}

std::any Runtime::visitAny(const Minus &e) {
    auto value = std::any_cast<Value>(e.expression->accept(*this));
    if (value.isInteger()) {
        return Value(-value.asInteger());
    } else if (value.isFloat()) {
        return Value(-value.asFloat());
    } else {
        throw RuntimeError("expected number; got \"" + value.asString() + "\"", e.location);
    }
}

std::any Runtime::visitAny(const FloatLiteral &e) { return Value(e.value); }

std::any Runtime::visitAny(const IntLiteral &e) { return Value(e.value); }

std::any Runtime::visitAny(const StringLiteral &e) { return Value(e.value); }

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

std::any Runtime::visitAny(const RangeChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    auto startValue = std::any_cast<Value>(c.start->accept(*this));

    if (c.end) {
        auto endValue = std::any_cast<Value>(c.end->accept(*this));
        return Value(range_chunk(chunk_type(c.type), startValue.asInteger() - 1,
                                 endValue.asInteger() - 1, value)
                         .get());
    } else {
        return Value(chunk(chunk_type(c.type), startValue.asInteger() - 1, value).get());
    }
}

std::any Runtime::visitAny(const AnyChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(
        random_chunk(
            chunk_type(c.type), [this](int count) { return _config.random() * count; }, value)
            .get());
}

std::any Runtime::visitAny(const LastChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(last_chunk(chunk_type(c.type), value).get());
}

std::any Runtime::visitAny(const MiddleChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(middle_chunk(chunk_type(c.type), value).get());
}

CH_RUNTIME_NAMESPACE_END
