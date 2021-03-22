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

CH_AST_NAMESPACE_BEGIN

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

CH_AST_NAMESPACE_END
