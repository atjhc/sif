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
	for (auto& statement : statements) {
		out << context.indentString();
		statement->prettyPrint(out, context);
		out << std::endl;
	}
	context.indentLevel -= 1;
}

void If::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "if ";
	condition->prettyPrint(out, context);
	out << " then" << std::endl;
	ifStatements->prettyPrint(out, context);
	if (elseStatements) {
		out << context.indentString() << "else" << std::endl;
		elseStatements->prettyPrint(out, context);
	}
	out << context.indentString() << "end if";
}

void Repeat::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "repeat";
	prettyPrintCondition(out, context);
	out << std::endl;
	statements->prettyPrint(out, context);
	out << context.indentString() << "end repeat";
}

void RepeatRange::prettyPrintCondition(std::ostream &out, PrettyPrintContext &context) const {
	out << " with ";
	variable->prettyPrint(out, context);
	out << " = ";
	startExpression->prettyPrint(out, context);
	if (ascending) {
		out << " to ";
	} else {
		out << " down to ";
	}
	endExpression->prettyPrint(out, context);
}

void RepeatCount::prettyPrintCondition(std::ostream &out, PrettyPrintContext &context) const {
	out << " ";
	countExpression->prettyPrint(out, context);
}

void RepeatCondition::prettyPrintCondition(std::ostream &out, PrettyPrintContext &context) const {
	if (conditionValue) {
		out << " while ";
	} else {
		out << " until ";
	}
	condition->prettyPrint(out, context);
}

void NextRepeat::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "next repeat";
}

void ExitRepeat::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out <<  "exit repeat";
}

void Exit::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "exit ";
	messageKey->prettyPrint(out, context);
}

void Pass::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "pass ";
	messageKey->prettyPrint(out, context);
}

void Global::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "global ";
	variables->prettyPrint(out, context);
}

void Return::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "return ";
	if (expression) {
		expression->prettyPrint(out, context);
	}
}

void Put::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "put ";
	expression->prettyPrint(out, context);
	if (target) {
		target->prettyPrint(out, context);
	}
}

void Get::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "get ";
	expression->prettyPrint(out, context);
}

HT_AST_NAMESPACE_END
