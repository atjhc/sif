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

#include "Common.h"
#include "runtime/Function.h"
#include "runtime/Interpreter.h"
#include "parser/Parser.h"
#include "utilities/devnull.h"

#include <sstream>

CH_RUNTIME_NAMESPACE_BEGIN

Value ValueFunction::valueOf(Interpreter &r, const Message &m) const {
    auto expression = m.arguments[0];

	Parser parser(ParserConfig("<interpreter>", devnull));
    Owned<ast::Expression> result;
    if ((result = parser.parseExpression(expression.asString())) == nullptr) {
        return expression.asString();
    }

    return std::any_cast<Value>(result->accept(r));
}

Value RandomFunction::valueOf(Interpreter &r, const Message &m) const {
    auto max = m.arguments[0].asInteger();
    return long(r.random()() * max) + 1;
}

Value ParamFunction::valueOf(Interpreter &r, const Message &m) const {
	auto index = m.arguments[0].asInteger();
	if (index < 0) return Value();
	if (index == 0) return r.currentFrame().message.name;
	if (index - 1 > r.currentFrame().message.arguments.size()) return Value();
	return r.currentFrame().message.arguments[index - 1];
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
	return r.currentFrame().message.arguments.size();
}

Value ResultFunction::valueOf(Interpreter &r, const Message &) const {
	return r.currentFrame().resultValue;
}

Value TargetFunction::valueOf(Interpreter &r, const Message &) const {
	return Value(r.currentFrame().target);
}

CH_RUNTIME_NAMESPACE_END
