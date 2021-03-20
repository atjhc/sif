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

#pragma once

#include "Defines.h"
#include "Base.h"
#include "Expressions.h"

#include <vector>
#include <ostream>

HT_AST_NAMESPACE_BEGIN

struct Statement: Node {
};

struct If: Statement {
	Expression *_condition;
	StatementList *_ifStatements;
	StatementList *_elseStatements;

	If(Expression *condition, StatementList *ifStatements, StatementList *elseStatements) 
		: _condition(condition), _ifStatements(ifStatements), _elseStatements(elseStatements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct ExitRepeat: Statement {
	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct NextRepeat: Statement {
	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Exit: Statement {
	Identifier *_messageKey;

	Exit(Identifier *messageKey) : _messageKey(messageKey) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Pass: Statement {
	Identifier *_messageKey;

	Pass(Identifier *messageKey) : _messageKey(messageKey) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Global: Statement {
	IdentifierList *_variables;

	Global(IdentifierList *variables) : _variables(variables) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Return: Statement {
	Expression *_expression;

	Return(Expression *expression) : _expression(expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;

};

struct Put: Statement {
	Expression *_expression;
	Identifier *_target;

	Put(Expression *expression, Identifier *target)
		: _expression(expression), _target(target) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Get: Statement {
	Expression *_expression;

	Get(Expression *expression)
		: _expression(expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
