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

#include "Statements.h"

HT_AST_NAMESPACE_BEGIN

void StatementList::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	context.indentLevel += 1;
	for (auto statement : _statements) {
		out << context.indentString();
		statement->prettyPrint(out, context);
		out << std::endl;
	}
	context.indentLevel -= 1;
}

void If::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "if ";
	_condition->prettyPrint(out, context);
	out << " then" << std::endl;
	_ifStatements->prettyPrint(out, context);
	if (_elseStatements) {
		out << context.indentString() << "else" << std::endl;
		_elseStatements->prettyPrint(out, context);
	}
	out << context.indentString() << "end if";
}

void NextRepeat::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "next repeat";
}

void ExitRepeat::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out <<  "exit repeat";
}

void Exit::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "exit ";
	_messageKey->prettyPrint(out, context);
}

void Pass::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "pass ";
	_messageKey->prettyPrint(out, context);
}

void Global::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "global ";
	_variables->prettyPrint(out, context);
}

void Return::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "return ";
	if (_expression) {
		_expression->prettyPrint(out, context);
	}
}

void Put::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "put ";
	_expression->prettyPrint(out, context);
	if (_target) {
		_target->prettyPrint(out, context);
	}
}

void Get::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "get ";
	_expression->prettyPrint(out, context);
}

HT_AST_NAMESPACE_END
