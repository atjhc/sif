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

#include "runtime/Interpreter.h"
#include "ast/Descriptor.h"
#include "ast/Property.h"
#include "runtime/Object.h"
#include "runtime/File.h"
#include "runtime/Folder.h"
#include "runtime/Property.h"
#include "runtime/Container.h"
#include "runtime/Descriptor.h"
#include "runtime/ChunkResolver.h"
#include "utilities/chunk.h"
#include "utilities/devnull.h"

#include <math.h>

#include <random>

CH_RUNTIME_NAMESPACE_BEGIN

std::function<float()> InterpreterConfig::defaultRandom() {
    static thread_local std::default_random_engine generator(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return [&]() {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        return distribution(generator);
    };
}

#if !defined(DEBUG)
#define trace(x)
#endif

Interpreter::Interpreter(const InterpreterConfig &config) : _config(config) {
    add(Property("sin"), new OneArgumentFunction<sin>());
    add(Property("cos"), new OneArgumentFunction<cos>());
    add(Property("tan"), new OneArgumentFunction<tan>());
    add(Property("atan"), new OneArgumentFunction<atan>());
    add(Property("abs"), new OneArgumentFunction<fabs>());
    add(Property("exp"), new OneArgumentFunction<exp>());
    add(Property("exp2"), new OneArgumentFunction<exp2>());
    add(Property("exp1"), new OneArgumentFunction<expm1>());
    add(Property("log2"), new OneArgumentFunction<log2>());
    add(Property("log10"), new OneArgumentFunction<log10>());
    add(Property("ln"), new OneArgumentFunction<log>());
    add(Property("ln1"), new OneArgumentFunction<log1p>());
    add(Property("round"), new OneArgumentFunction<round>());
    add(Property("sqrt"), new OneArgumentFunction<sqrt>());
    add(Property("trunc"), new OneArgumentFunction<trunc>());

    add(Property("max"), new MaxFunction());
    add(Property("min"), new MinFunction());
    add(Property("sum"), new SumFunction());
    add(Property("average"), new MeanFunction());
    add(Property("length"), new LengthFunction());
    add(Property("offset"), new OffsetFunction());
    add(Property("random"), new RandomFunction());
    add(Property("params"), new ParamsFunction());
    add(Property("paramcount"), new ParamCountFunction());
    add(Property("param"), new ParamFunction());
    add(Property("result"), new ResultFunction());
    add(Property("value"), new ValueFunction());
    add(Property("target"), new TargetFunction());
    add(Property("seconds"), new SecondsFunction());
    add(Property("secs"), new SecondsFunction());

    // TODO: add missing functions: date, time
    // Skipping these: ticks, annuity, charToNum, numToChar, compound

    add(Descriptor("file"), [=](Optional<Value> value) -> Strong<Object> {
        return std::static_pointer_cast<Object>(File::Make(value.value().asString()));
    });

    auto directoryFactory = [=](Optional<Value> value) -> Strong<Object> {
        return std::static_pointer_cast<Object>(Folder::Make(value.value().asString()));
    };
    add(Descriptor("folder"), directoryFactory);
    add(Descriptor("directory"), directoryFactory);

}

bool Interpreter::send(const Message &message, Strong<Object> target) {
    trace(std::string("send(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return false;
    }

    bool passing = true;
    auto handler = target->handlerFor(message);
    if (handler.has_value()) {
        _stack.push(InterpreterStackFrame(message, target));
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

Optional<Value> Interpreter::call(const Message &message, Strong<Object> target) {
    trace(std::string("call(") + message.name + ", " + (target ? target->name() : "null") + ")");

    if (target == nullptr) {
        return Empty;
    }

    Value result;
    bool passing = true;

    auto handler = target->functionFor(message);
    if (handler.has_value()) {
        _stack.push(InterpreterStackFrame(message.name, target));
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

const InterpreterStackFrame &Interpreter::currentFrame() { return _stack.top(); }

std::function<float()> Interpreter::random() { return _config.random; }

#pragma mark - Private

void Interpreter::add(const Property &property, Function *fn) {
    _functions[property] = Owned<Function>(fn);
}

void Interpreter::add(const Descriptor &descriptor, const ObjectFactory &factory) {
    _factories[descriptor] = factory;
}

void Interpreter::set(const std::string &name, const Value &value) {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);
    if (i != globalNames.end()) {
        _globals.set(name, value);
        return;
    }
    return _stack.top().locals.set(name, value);
}

Optional<Value> Interpreter::get(const std::string &name) const {
    const auto &globalNames = _stack.top().globals;
    const auto &i = globalNames.find(name);

    Optional<Value> value = Empty;
    if (i != globalNames.end()) {
        value = _globals.get(name);
    } else {
        value = _stack.top().locals.get(name);
    }

    return value;
}

void Interpreter::execute(const ast::Handler &handler, const std::vector<Value> &values) {
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

void Interpreter::execute(const ast::StatementList &statements) {
    for (auto &statement : statements.statements) {
        statement->accept(*this);

        auto &frame = _stack.top();
        if (frame.passing || frame.exiting || frame.exitingRepeat || frame.skippingRepeat || frame.returning) {
            break;
        }
    }
}

Value Interpreter::evaluate(const ast::Expression &expression) {
    return expression.accept(*this);
}

Value Interpreter::evaluateBuiltin(const Property &property, const std::vector<Value> &arguments) {
    auto fn = _functions.find(property);
    if (fn == _functions.end()) {
        if (property.names.size() == 1) {
            throw RuntimeError(String("unrecognized function or property '", property.description(), "'"));
        } else if (property.names.size() > 1) {
            throw RuntimeError(String("unrecognized property '", property.description(), "'"));
        }
    }
    auto message = Message(property.description(), arguments);
    return fn->second->valueOf(*this, message);
}

#if defined(DEBUG)
void Interpreter::trace(const std::string &msg) const {
    if (_config.enableTracing) {
        _config.stdout << "core: " << msg << std::endl;
    }
}
#endif

#pragma mark - Statement::Visitor

void Interpreter::visit(const ast::If &s) {
    auto condition = s.condition->accept(*this);
    if (condition.asBool()) {
        execute(*s.ifStatements);
    } else if (s.elseStatements) {
        execute(*s.elseStatements);
    }
}

void Interpreter::visit(const ast::Repeat &s) {
    while (true) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
    }
    _stack.top().skippingRepeat = false;
}

void Interpreter::visit(const ast::RepeatCount &s) {
    auto countValue = evaluate(*s.countExpression);
    auto count = countValue.asInteger();

    for (int i = 0; i < count; i++) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
    }
    _stack.top().skippingRepeat = false;
}

void Interpreter::visit(const ast::RepeatRange &s) {
    auto iteratorName = s.variable->name;
    auto startValue = evaluate(*s.startExpression).asInteger();
    auto endValue = evaluate(*s.endExpression).asInteger();

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
    _stack.top().skippingRepeat = false;
}

void Interpreter::visit(const ast::RepeatCondition &s) {
    auto conditionValue = s.condition->accept(*this).asBool();
    while (conditionValue == s.conditionValue) {
        execute(*s.statements);
        if (_stack.top().exitingRepeat) {
            _stack.top().exitingRepeat = false;
            break;
        }
        conditionValue = s.condition->accept(*this).asBool();
    }
    _stack.top().skippingRepeat = false;
}

void Interpreter::visit(const ast::ExitRepeat &) {
    _stack.top().exitingRepeat = true;
}

void Interpreter::visit(const ast::NextRepeat &) {
    _stack.top().skippingRepeat = true;
}

void Interpreter::visit(const ast::Exit &s) {
    trace("exit(" + s.messageKey->name + ")");
    _stack.top().exiting = true;
}

void Interpreter::visit(const ast::Pass &s) {
    trace("pass(" + s.messageKey->name + ")");
    _stack.top().passing = true;
}

void Interpreter::visit(const ast::Global &s) {
    Set<std::string> globals;
    for (auto &identifier : s.variables->identifiers) {
        globals.insert(identifier->name);
    }
    trace("global(" + describe(globals) + ")");
    _stack.top().globals.insert(globals.begin(), globals.end());
}

void Interpreter::visit(const ast::Return &s) {
    _stack.top().returning = true;
    if (s.expression) {
        auto value = evaluate(*s.expression);
        _stack.top().returningValue = value;
    }
}

void Interpreter::visit(const ast::Do &c) {
    if (c.language) {
        auto languageName = evaluate(*c.language);

        // TODO: Call out to another language.
        throw RuntimeError(String("unrecognized language '", languageName.asString(), "'"),
                           c.language->location);
    }

    auto value = evaluate(*c.expression);
    auto valueString = value.asString();

    Parser parser(ParserConfig("<runtime>", _config.stderr));
    Owned<ast::StatementList> statements;

    if ((statements = parser.parseStatements(valueString)) == nullptr) {
        throw RuntimeError("failed to parse script", c.location);
    }
    execute(*statements);
}

void Interpreter::visit(const ast::Command &c) {
    auto message = Message(c.name->name);
    if (c.arguments) {
        for (auto &expression : c.arguments->expressions) {
            auto arg = evaluate(*expression);
            message.arguments.push_back(arg);
        }
    }

    bool handled = send(message, _stack.top().target);
    if (!handled) {
        c.accept(*this);
    }
}

void Interpreter::visit(const ast::Put &statement) {
    auto value = evaluate(*statement.expression);
    if (!statement.target) {
        _config.stdout << value.asString() << std::endl;
        return;
    }

    auto container = Container(statement.target);

    // Optimization for simple assignment.
    if (statement.preposition == ast::Put::Into && container.chunkList.size() == 0) {
        set(container.name, value);
        return;
    }

    std::string targetValue = get(container.name).value_or(Value()).asString();
    chunk targetChunk = ChunkResolver::resolve(container.chunkList, *this, targetValue);

    switch (statement.preposition) {
    case ast::Put::Before:
        targetValue.replace(targetChunk.begin(), targetChunk.begin(), value.asString());
        break;
    case ast::Put::After:
        targetValue.replace(targetChunk.end(), targetChunk.end(), value.asString());
        break;
    case ast::Put::Into:
        targetValue.replace(targetChunk.begin(), targetChunk.end(), value.asString());
        break;
    }

    set(container.name, targetValue);
}

void Interpreter::visit(const ast::Get &s) {
    auto result = evaluate(*s.expression);
    _stack.top().locals.set("it", result);
}

void Interpreter::visit(const ast::Set &statement) {
    if (!statement.property->expression) {
        return;    
    }

    auto target = evaluate(*statement.property->expression);
    if (target.isObject()) {
        auto object = target.asObject();
        auto value = evaluate(*statement.expression);
        if (!object->setValueForProperty(value, Property(*statement.property))) {
            throw RuntimeError("unknown property", statement.property->location);
        }
    }
}

void Interpreter::visit(const ast::Ask &s) {
    auto question = evaluate(*s.expression);

    _config.stdout << question.asString();
    std::string result;
    std::getline(_config.stdin, result);

    _stack.top().locals.set("it", result);
}

static void expectNumber(const Value &value, const ast::Location &location) {
    if (!value.isNumber()) {
        throw RuntimeError(String("expected number, got '", value.asString(), "'"), location);
    }
}

void Interpreter::performArith(const Owned<ast::Expression> &expression,
                               const Owned<ast::Expression> &destination,
                               const std::function<Value(Value, Value)> &fn) {
    auto value = evaluate(*expression);
    auto container = Container(destination);

    auto targetValue = get(container.name).value_or(Value());
    if (container.chunkList.size() == 0) {
        expectNumber(value, expression->location);
        expectNumber(targetValue, destination->location);
        return set(container.name, fn(targetValue, value));
    }

    auto targetString = targetValue.asString();
    chunk targetChunk = ChunkResolver::resolve(container.chunkList, *this, targetString);
    targetValue = Value(targetChunk.get());
    
    expectNumber(value, expression->location);
    expectNumber(targetValue, destination->location);

    targetValue = fn(targetValue, value);
    targetString.replace(targetChunk.begin(), targetChunk.end(), targetValue.asString());

    set(container.name, targetString);                         
}

void Interpreter::visit(const ast::Add &statement) {
    performArith(statement.expression, statement.container, [](Value lhs, Value rhs) {
        return lhs + rhs;
    });
}

void Interpreter::visit(const ast::Subtract &statement) {
    performArith(statement.expression, statement.container, [](Value lhs, Value rhs) {
        return lhs - rhs;
    });
}

void Interpreter::visit(const ast::Multiply &statement) {
    performArith(statement.expression, statement.container, [](Value lhs, Value rhs) {
        return lhs * rhs;
    });
}

void Interpreter::visit(const ast::Divide &statement) {
    performArith(statement.expression, statement.container, [&statement](Value lhs, Value rhs) {
        if (rhs.asFloat() == 0) {
            throw RuntimeError("divide by zero", statement.expression->location);
        }
        return lhs / rhs;
    });
}

void Interpreter::visit(const ast::Delete &statement) {
    if (auto chunkExpression = dynamic_cast<ast::ChunkExpression *>(statement.expression.get())) {
        auto container = Container(statement.expression);

        std::string targetValue = get(container.name).value_or(Value()).asString();
        chunk targetChunk = ChunkResolver::resolve(container.chunkList, *this, targetValue);
        
        // TODO: move this logic into the chunk util classes.
        if ((targetChunk.chunk_type() == chunk::line || targetChunk.chunk_type() == chunk::item) &&
            targetChunk.end() < targetValue.end()) {
            targetValue.erase(targetChunk.begin(), targetChunk.end() + 1);
        } else {
            targetValue.erase(targetChunk.begin(), targetChunk.end());
        }
        set(container.name, targetValue);
    }
}

#pragma mark - Expression::Visitor<Value>

Value Interpreter::visit(const ast::Identifier &e) { return get(e.name).value_or(Value(e.name)); }

Value Interpreter::visit(const ast::FunctionCall &fn) {
    auto message = Message(fn.name->name);
    if (fn.arguments) {
        for (auto &argument : fn.arguments->expressions) {
            auto value = argument->accept(*this);
            message.arguments.push_back(value);
        }
    }

    auto result = call(message, _stack.top().target);
    if (result.has_value()) {
        return result.value();
    }

    try {
        return evaluateBuiltin(Property(fn), message.arguments);
    } catch (InvalidArgumentError &error) {
        error.where = fn.arguments->expressions[error.argumentIndex]->location;
        throw;
    } catch (RuntimeError &error) {
        error.where = fn.location;
        throw;
    }
}

Value Interpreter::visit(const ast::Property &p) {
    std::vector<Value> arguments;
    if (p.expression) {
        auto value = evaluate(*p.expression);
        if (value.isObject()) {
            auto property = runtime::Property(p);

            Optional<Value> result;
            try {
                result = value.asObject()->valueForProperty(property);
            } catch (RuntimeError &error) {
                error.where = p.location;
                throw;
            }

            if (!result.has_value()) {
                throw RuntimeError(String("unknown property '", property.description(), "' for object '", value.asString(), "'"), p.location);
            }
            return result.value();
        } else {
            arguments.push_back(value);
        }
    }

    // Property calls skip the message path.
    try {
        return evaluateBuiltin(Property(p), arguments);
    } catch (RuntimeError &error) {
        error.where = p.location;
        throw;
    }
}

Value Interpreter::visit(const ast::Descriptor &d) {
    auto descriptor = Descriptor(d);

    if (!d.value && descriptor.is("me")) {
        return Value(_stack.top().target);
    }
    if (!d.value && descriptor.names.size() == 1) {
        return get(descriptor.names[0]).value_or(Value(descriptor.names[0]));
    }

    Optional<Value> value;
    if (d.value) {
        value = evaluate(*d.value);
    } else {
        value = get(descriptor.names.back()).value_or(Value(descriptor.names.back()));
        descriptor.names.pop_back();
    }

    const auto &it = _factories.find(descriptor);
    if (it == _factories.end()) {
        throw RuntimeError(String("unrecognized descriptor '", descriptor.description(), "'"), d.location);
    }
    return it->second(value);
}

static void checkNumberOperand(const Value &value, const ast::Location &location) {
    if (!value.isNumber()) {
        throw RuntimeError(String("expected number value here, got '", value.asString(), "'"), location);
    }
}

Value Interpreter::visit(const ast::Binary &e) {
    auto lhs = evaluate(*e.leftExpression);
    auto rhs = evaluate(*e.rightExpression);

    if (e.binaryOperator == ast::Binary::IsA) {
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
            return lhs.isEmpty();
        }
        // TODO: Check for additional host defined types.
        throw RuntimeError(String("unknown type name '", rhs.asString(), "'"),
                           e.rightExpression->location);
    }

    switch (e.binaryOperator) {
    case ast::Binary::Equal:
        return (lhs == rhs);
    case ast::Binary::NotEqual:
        return (lhs != rhs);
    case ast::Binary::LessThan:
        return (lhs < rhs);
    case ast::Binary::GreaterThan:
        return (lhs > rhs);
    case ast::Binary::LessThanOrEqual:
        return (lhs <= rhs);
    case ast::Binary::GreaterThanOrEqual:
        return (lhs >= rhs);
    case ast::Binary::Plus:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs + rhs);
    case ast::Binary::Minus:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs - rhs);
    case ast::Binary::Multiply:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs * rhs);
    case ast::Binary::Divide:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        if (rhs.asFloat() == 0) {
            throw RuntimeError("divide by zero", e.rightExpression->location);
        }
        return (lhs / rhs);
    case ast::Binary::Exponent:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs ^ rhs);
    case ast::Binary::Mod:
        checkNumberOperand(lhs, e.leftExpression->location);
        checkNumberOperand(rhs, e.rightExpression->location);
        return (lhs % rhs);
    case ast::Binary::IsIn:
        return rhs.contains(lhs);
    case ast::Binary::Contains:
        return lhs.contains(rhs);
    case ast::Binary::Concat:
        return lhs.concat(rhs);
    case ast::Binary::ConcatWithSpace:
        return lhs.concatSpace(rhs);
    default:
        throw RuntimeError("unexpected operator", e.location);
    }
}

Value Interpreter::visit(const ast::Logical &e) {
    if (e.logicalOperator == ast::Logical::And) {
        auto lhs = evaluate(*e.leftExpression);
        if (!lhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.leftExpression->location);
        }
        if (!lhs.asBool()) {
            return false;
        }

        auto rhs = evaluate(*e.rightExpression);
        if (!rhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.rightExpression->location);
        }
        return rhs;
    }

    if (e.logicalOperator == ast::Logical::Or) {
        auto lhs = evaluate(*e.leftExpression);
        if (!lhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.leftExpression->location);
        }
        if (lhs.asBool()) {
            return true;
        }

        auto rhs = evaluate(*e.rightExpression);
        if (!rhs.isBool()) {
            throw RuntimeError("expected a boolean value here", e.rightExpression->location);
        }
        return rhs;
    }

    throw RuntimeError("unexpected operator", e.location);
}

Value Interpreter::visit(const ast::Unary &e) {
    auto value = evaluate(*e.expression);

    switch (e.unaryOperator) {
    case ast::Unary::ThereIsA:
        if (value.isObject()) {
            return value.asObject()->exists();
        }
        return !value.isEmpty();
    case ast::Unary::Not:
        if (!value.isBool()) {
            throw RuntimeError("expected a boolean value here", e.expression->location);
        }
        return !value.asBool();
    case ast::Unary::Minus:
        if (value.isInteger()) {
            return -value.asInteger();
        } else if (value.isFloat()) {
            return -value.asFloat();
        } else {
            throw RuntimeError("expected a number value here", e.expression->location);
        }
    }
}

Value Interpreter::visit(const ast::ChunkExpression &e) {
    auto value = evaluate(*e.expression).asString();
    auto resolver = runtime::ChunkResolver(*this, value);
    return resolver.resolve(*e.chunk).get();
}

Value Interpreter::visit(const ast::FloatLiteral &e) { return Value(e.value); }

Value Interpreter::visit(const ast::IntLiteral &e) { return Value(e.value); }

Value Interpreter::visit(const ast::StringLiteral &e) { return Value(e.value); }

CH_RUNTIME_NAMESPACE_END
