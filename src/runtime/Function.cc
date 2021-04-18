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

#include "runtime/Function.h"
#include "Common.h"
#include "parser/Parser.h"
#include "runtime/Interpreter.h"
#include "utilities/devnull.h"

#include <chrono>
#include <sstream>

CH_RUNTIME_NAMESPACE_BEGIN

void Function::expectArgumentCount(const Message &m, int count) const {
    if (m.arguments.size() != count) {
        throw ArgumentsError(String("expected ", count, (count == 1 ? " argument" : " arguments"),
                                  " here, but got ", m.arguments.size()));
    }
}

void Function::expectNumberAt(const Message &m, int index) const {
    if (!m.arguments[index].isNumber()) {
        throw InvalidArgumentError(
            String("expected number here, but got '", m.arguments[index].asString(), "'"),
            index);
    }
}

void Function::expectArguments(const Message &m) const {
    if (m.arguments.size() == 0) {
        throw ArgumentsError(String("expected arguments for function '", m.name, "'"));
    }
}

void Function::expectAllNumbers(const Message &m) const {
    for (int i = 0; i < m.arguments.size(); i++) {
        auto &arg = m.arguments[i];
        if (!arg.isNumber()) {
            throw InvalidArgumentError(String("expected number for argument ", i+1, ", but got '", arg.asString(), "'"), i);
        }
    }
}

Value MaxFunction::valueOf(Interpreter &, const Message &m) const {
    expectArguments(m);
    expectAllNumbers(m);
    return *std::max_element(m.arguments.begin(), m.arguments.end());
}

Value MinFunction::valueOf(Interpreter &, const Message &m) const {
    expectArguments(m);
    expectAllNumbers(m);
    return *std::min_element(m.arguments.begin(), m.arguments.end());
}

Value SumFunction::valueOf(Interpreter &, const Message &m) const {
    expectArguments(m);
    expectAllNumbers(m);
    return std::accumulate(m.arguments.begin(), m.arguments.end(), Value(0));
}

Value MeanFunction::valueOf(Interpreter &, const Message &m) const {
    expectArguments(m);
    expectAllNumbers(m);
    return Value(std::accumulate(m.arguments.begin(), m.arguments.end(), Value(0)).asFloat() / (double)m.arguments.size());
}

Value LengthFunction::valueOf(Interpreter &, const Message &m) const {
    expectArgumentCount(m, 1);
    return m.arguments[0].asString().length();
}

Value OffsetFunction::valueOf(Interpreter &, const Message &m) const {
    expectArgumentCount(m, 2);
    auto &v1 = m.arguments[0];
    auto &v2 = m.arguments[1];

    auto result = v2.asString().find(v1.asString());
    return result == std::string::npos ? Value(0) : Value(result + 1);
}

Value SecondsFunction::valueOf(Interpreter &, const Message &m) const {
    expectArgumentCount(m, 0);

    std::chrono::system_clock::time_point timePoint = std::chrono::system_clock::now();
    std::chrono::system_clock::duration duration = timePoint.time_since_epoch();
    return Value(duration.count() * std::chrono::system_clock::period::num /
                 std::chrono::system_clock::period::den);
}

Value ValueFunction::valueOf(Interpreter &interpreter, const Message &m) const {
    expectArgumentCount(m, 1);
    auto expressionString = m.arguments[0].asString();

    Parser parser(ParserConfig("<interpreter>", devnull));
    Owned<ast::Expression> result;
    if ((result = parser.parseExpression(expressionString)) == nullptr) {
        return expressionString;
    }

    return std::any_cast<Value>(result->accept(interpreter));
}

Value RandomFunction::valueOf(Interpreter &interpreter, const Message &m) const {
    expectArgumentCount(m, 1);
    expectNumberAt(m, 0);

    auto max = m.arguments[0].asInteger();
    return long(interpreter.random()() * max) + 1;
}

Value ParamFunction::valueOf(Interpreter &interpreter, const Message &m) const {
    expectArgumentCount(m, 1);
    expectNumberAt(m, 0);
    
    auto index = m.arguments[0].asInteger();
    if (index < 0)
        return Value();
    if (index == 0)
        return interpreter.currentFrame().message.name;
    if (index - 1 >= interpreter.currentFrame().message.arguments.size())
        return Value();
    return interpreter.currentFrame().message.arguments[index - 1];
}

Value ParamsFunction::valueOf(Interpreter &r, const Message &m) const {
    std::ostringstream ss;
    auto &message = r.currentFrame().message;
    ss << message.name;
    if (message.arguments.size()) {
        ss << " ";
        auto i = message.arguments.begin();
        while (i != message.arguments.end()) {
            ss << "\"" << *i << "\"";
            i++;
            if (i != message.arguments.end()) {
                ss << ",";
            }
        }
    }
    return ss.str();
}

Value ParamCountFunction::valueOf(Interpreter &r, const Message &m) const {
    expectArgumentCount(m, 0);

    return r.currentFrame().message.arguments.size();
}

Value ResultFunction::valueOf(Interpreter &r, const Message &m) const {
    expectArgumentCount(m, 0);

    return r.currentFrame().resultValue;
}

Value TargetFunction::valueOf(Interpreter &r, const Message &m) const {
    expectArgumentCount(m, 0);

    return Value(r.currentFrame().target);
}

CH_RUNTIME_NAMESPACE_END
