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

#include "Commands.h"

CH_AST_NAMESPACE_BEGIN

void Command::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << name->name;
	if (arguments) {
		arguments->prettyPrint(out, context);
	}
}

void Preposition::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	switch (type) {
	case Before:
		out << "before";
		break;
	case Into:
		out << "into";
		break;
	case After:
		out << "after";
		break;
	}
}

void Put::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "put ";
	expression->prettyPrint(out, context);
	if (preposition) {
		out << " ";
		preposition->prettyPrint(out, context);
	}
	if (target) {
		out << " ";
		target->prettyPrint(out, context);
	}
}

void Get::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "get ";
	expression->prettyPrint(out, context);
}

void Ask::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "ask ";
	expression->prettyPrint(out, context);
}

void Add::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "add ";
	expression->prettyPrint(out, context);
	out << " to ";
}

void Subtract::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "subtract ";
	expression->prettyPrint(out, context);
	out << " from ";
}

void Multiply::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "multiply ";
	expression->prettyPrint(out, context);
	out << " by ";
}

void Divide::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "divide ";
	expression->prettyPrint(out, context);
	out << " by ";
}

CH_AST_NAMESPACE_END
