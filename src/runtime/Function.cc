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
#include "runtime/Runtime.h"
#include "parser/Parser.h"
#include "utilities/devnull.h"

#include <sstream>

CH_RUNTIME_NAMESPACE_BEGIN

Value ValueFunction::valueOf(Runtime &r, const Message &m) const {
    auto expression = m.arguments[0];

	Parser parser(ParserConfig("<runtime>", devnull));
    Owned<ast::Expression> result;
    if ((result = parser.parseExpression(expression.asString())) == nullptr) {
        return expression.asString();
    }

    return result->evaluate(r);
}

Value RandomFunction::valueOf(Runtime &r, const Message &m) const {
    auto max = m.arguments[0].asInteger();
    return long(r.config.random() * max) + 1;
}

Value ParamFunction::valueOf(Runtime &r, const Message &m) const {
	auto index = m.arguments[0].asInteger();
	if (index < 0) return Value();
	if (index == 0) return r.stack.top().message.name;
	if (index - 1 > r.stack.top().message.arguments.size()) return Value();
	return r.stack.top().message.arguments[index - 1];
}

Value ParamsFunction::valueOf(Runtime &r, const Message &m) const {
	std::ostringstream ss;
	auto &message = r.stack.top().message;
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

Value ParamCountFunction::valueOf(Runtime &r, const Message &m) const {
	return r.stack.top().message.arguments.size();
}

Value ResultFunction::valueOf(Runtime &r, const Message &) const {
	return r.stack.top().resultValue;
}

Value TargetFunction::valueOf(Runtime &r, const Message &) const {
	return Value(r.stack.top().target);
}

CH_RUNTIME_NAMESPACE_END
