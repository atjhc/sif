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

#include "runtime/Core.h"
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

std::function<float()> CoreConfig::defaultRandom() {
    static thread_local std::default_random_engine generator(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return [&]() {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        return distribution(generator);
    };
}

#if !defined(DEBUG)
    #define trace(x)
#endif

Core::Core(const CoreConfig &config)
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

bool Core::send(const Message &message, Strong<Object> target) {
    trace(std::string("send(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return false;
    }

    bool passing = true;
    auto handler = target->handlerFor(message);
    if (handler.has_value()) {
        _stack.push(CoreStackFrame(message, target));
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

Value Core::call(const Message &message, Strong<Object> target) {
    trace(std::string("call(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return evaluateFunction(message);
    }

    Value result;
    bool passing = true;

    auto handler = target->functionFor(message);
    if (handler.has_value()) {
        _stack.push(CoreStackFrame(message.name, target));
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

const CoreStackFrame& Core::currentFrame() {
    return _stack.top();
}

std::function<float()> Core::random() {
    return _config.random;
}

#pragma mark - Private

void Core::add(const std::string &name, Function *fn) {
    _functions[lowercase(name)] = Owned<Function>(fn);
}

void Core::set(const std::string &name, const Value &value) {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
       _globals.set(name, value);
        return;
    }
    return _stack.top().locals.set(name, value);
}

Value Core::get(const std::string &name) const {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
        return _globals.get(name);
    }
    return _stack.top().locals.get(name);
}

void Core::execute(const ast::Handler &handler, const std::vector<Value> &values) {
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

void Core::execute(const ast::StatementList &statements) {
    for (auto &statement : statements.statements) {
        statement->accept(*this);

        auto &frame = _stack.top();
        if (frame.passing || frame.exiting || frame.returning) {
            break;
        }
    }
}

#if defined(DEBUG)
void Core::trace(const std::string &msg) const {
    if (_config.enableTracing) {
       _config.stdout << "core: " << msg << std::endl;
    }
}
#endif

#pragma mark - Unused

std::any Core::visitAny(const ast::Program &) {
    return std::any();
}

std::any Core::visitAny(const ast::Handler &) {
    return std::any();
}

std::any Core::visitAny(const ast::StatementList &) {
    return std::any();
}

std::any Core::visitAny(const ast::IdentifierList &) {
    return std::any();
}

std::any Core::visitAny(const ast::ExpressionList &) {
    return std::any();
}

#pragma mark - StatementVisitor

std::any Core::visitAny(const If &s) {
    auto condition = std::any_cast<Value>(s.condition->accept(*this));
    if (condition.asBool()) {
        execute(*s.ifStatements);
    } else if (s.elseStatements) {
        execute(*s.elseStatements);
    }

    return std::any();
}

std::any Core::visitAny(const Repeat &s) {
    while (true) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
    }

    return std::any();
}

std::any Core::visitAny(const RepeatCount &s) {
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

std::any Core::visitAny(const RepeatRange &s) {
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

std::any Core::visitAny(const RepeatCondition &s) {
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

std::any Core::visitAny(const ExitRepeat &) { 
    _stack.top().exitingRepeat = true;
    return std::any();
}

std::any Core::visitAny(const NextRepeat &) { 
    _stack.top().skippingRepeat = true;
    return std::any();
}

std::any Core::visitAny(const Exit &s) {
    trace("exit(" + s.messageKey->name + ")");
    if (s.messageKey->name == _stack.top().message.name) {
        _stack.top().exiting = true;
    } else {
        throw RuntimeError("unexpected identifier " + s.messageKey->name, s.location);
    }

    return std::any();
}

std::any Core::visitAny(const Pass &s) {
    trace("pass(" + s.messageKey->name + ")");
    if (s.messageKey->name == _stack.top().message.name) {
        _stack.top().passing = true;
    } else {
        throw RuntimeError("unexpected identifier " + s.messageKey->name, s.location);
    }

    return std::any();
}

std::any Core::visitAny(const Global &s) {
    Set<std::string> globals;
    for (auto &identifier : s.variables->identifiers) {
        globals.insert(identifier->name);
    }
    trace("global(" + describe(globals) + ")");
    _stack.top().globals.insert(globals.begin(), globals.end());

    return std::any();
}

std::any Core::visitAny(const Return &s) {
    _stack.top().returning = true;
    if (s.expression) {
        auto value = std::any_cast<Value>(s.expression->accept(*this));
        _stack.top().returningValue = value;
    }

    return std::any();
}

std::any Core::visitAny(const Do &c) {
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

std::any Core::visitAny(const Command &c) {
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

std::any Core::visitAny(const Put &s) {
    auto value = std::any_cast<Value>(s.expression->accept(*this));
    if (s.target) {
        auto &name = s.target->name;
        switch (s.preposition) {
        case Put::Before: {
            auto targetValue = get(name);
            set(name, value.asString() + targetValue.asString());
            break;
        }
        case Put::After: {
            auto targetValue = get(name);
            set(name, targetValue.asString() + value.asString());
            break;
        }
        case Put::Into:
            set(name, value);
            break;
        }
    } else {
       _config.stdout << value.asString() << std::endl;
    }

    return std::any();
}

std::any Core::visitAny(const Get &s) {
    auto result = std::any_cast<Value>(s.expression->accept(*this));
    _stack.top().locals.set("it", result);

    return std::any();
}

std::any Core::visitAny(const Ask &s) {
    auto question = std::any_cast<Value>(s.expression->accept(*this));

    _config.stdout << question.asString();
    std::string result;
    std::getline(_config.stdin, result);

    _stack.top().locals.set("it", result);

    return std::any();
}

std::any Core::visitAny(const Add &c) {
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

std::any Core::visitAny(const Subtract &c) {
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

std::any Core::visitAny(const Multiply &c) {
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

std::any Core::visitAny(const Divide &c) {
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

Value Core::evaluateFunction(const Message &message) {
    auto fn = _functions.find(lowercase(message.name));
    if (fn != _functions.end()) {
        return fn->second->valueOf(*this, message);
    }
    throw RuntimeError("unrecognized handler " + message.name);
}

#pragma mark - ExpressionVisitor

std::any Core::visitAny(const Identifier &e) { return get(e.name); }

std::any Core::visitAny(const FunctionCall &e) {
    auto message = Message(e.identifier->name);
    if (e.arguments) {
        for (auto &argument : e.arguments->expressions) {
            auto value = std::any_cast<Value>(argument->accept(*this));
            message.arguments.push_back(value);
        }
    }

    return call(message, _stack.top().target);
}

std::any Core::visitAny(const ast::Property &p) {
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

std::any Core::visitAny(const ast::Descriptor &d) {
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

void checkNumberOperand(const Value &value, const Location &location) {
    if (!value.isNumber()) {
        throw RuntimeError("expected number value here, got '" + value.asString() + "'", location);
    }
}

std::any Core::visitAny(const Binary &e) {
    auto lhs = std::any_cast<Value>(e.leftExpression->accept(*this));
    auto rhs = std::any_cast<Value>(e.rightExpression->accept(*this));
    
    if (e.binaryOperator == Binary::IsA) {
        auto typeName = lowercase(rhs.asString());
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
        throw RuntimeError("unknown type name '" + rhs.asString() + "'", e.rightExpression->location);
    }

    switch (e.binaryOperator) {
    case Binary::Equal:
        return (lhs == rhs);
    case Binary::NotEqual:
        return (lhs != rhs);
    case Binary::LessThan:
        return (lhs < rhs);
    case Binary::GreaterThan:
        return (lhs > rhs);
    case Binary::LessThanOrEqual:
        return (lhs <= rhs);
    case Binary::GreaterThanOrEqual:
        return (lhs >= rhs);
    case Binary::Plus:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs + rhs);
    case Binary::Minus:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs - rhs);
    case Binary::Multiply:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs * rhs);
    case Binary::Divide:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        if (rhs.asFloat() == 0) {
            throw RuntimeError("divide by zero", e.rightExpression->location);
        }
        return (lhs / rhs);
    case Binary::Exponent:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return Value(lhs ^ rhs);
    case Binary::Mod:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs % rhs);
    case Binary::IsIn:
        return rhs.contains(lhs);
    case Binary::Contains:
        return lhs.contains(rhs);
    case Binary::Concat:
        return lhs.concat(rhs);
    case Binary::ConcatWithSpace:
        return lhs.concatSpace(rhs);
    default:
        throw RuntimeError("unexpected operator", e.location);
    }
}

std::any Core::visitAny(const Logical &e) {
    if (e.logicalOperator == Logical::And) {
        auto lhs = std::any_cast<Value>(e.leftExpression->accept(*this));
        if (!lhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.leftExpression->location);
        }
        if (!lhs.asBool()) {
            return Value(false);
        }

        auto rhs = std::any_cast<Value>(e.rightExpression->accept(*this));
        if (!rhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.rightExpression->location);
        }
        return Value(rhs.asBool());
    }

    if (e.logicalOperator == Logical::Or) {
        auto lhs = std::any_cast<Value>(e.leftExpression->accept(*this));
        if (!lhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.leftExpression->location);
        }
        if (lhs.asBool()) {
            return Value(true);
        }

        auto rhs = std::any_cast<Value>(e.rightExpression->accept(*this));
        if (!rhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.rightExpression->location);
        }
        return Value(rhs.asBool());
    }

    throw RuntimeError("unexpected operator", e.location);
}

std::any Core::visitAny(const Unary &e) {
    auto value = std::any_cast<Value>(e.expression->accept(*this));

    switch (e.unaryOperator) {
    case Unary::ThereIsA:
        return Value(!value.isEmpty());
    case Unary::Not:
        if (!value.isBool()) {
            throw RuntimeError("expected a boolean value here", e.expression->location);
        }
        return Value(!value.asBool());
    case Unary::Minus:
        if (value.isInteger()) {
            return Value(-value.asInteger());
        } else if (value.isFloat()) {
            return Value(-value.asFloat());
        } else {
            throw RuntimeError("expected a number value here", e.expression->location);
        }
    }
}

std::any Core::visitAny(const FloatLiteral &e) { return Value(e.value); }

std::any Core::visitAny(const IntLiteral &e) { return Value(e.value); }

std::any Core::visitAny(const StringLiteral &e) { return Value(e.value); }

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

std::any Core::visitAny(const RangeChunk &c) {
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

std::any Core::visitAny(const AnyChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(
        random_chunk(
            chunk_type(c.type), [this](int count) { return _config.random() * count; }, value)
            .get());
}

std::any Core::visitAny(const LastChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(last_chunk(chunk_type(c.type), value).get());
}

std::any Core::visitAny(const MiddleChunk &c) {
    auto value = std::any_cast<Value>(c.expression->accept(*this)).asString();
    return Value(middle_chunk(chunk_type(c.type), value).get());
}

CH_RUNTIME_NAMESPACE_END
