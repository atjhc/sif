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
	Expression *condition;
	StatementList *ifStatements;
	StatementList *elseStatements;

	If(Expression *_condition, StatementList *_ifStatements, StatementList *_elseStatements) 
		: condition(_condition), ifStatements(_ifStatements), elseStatements(_elseStatements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct ExitRepeat: Statement {
	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct NextRepeat: Statement {
	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Exit: Statement {
	Identifier *messageKey;

	Exit(Identifier *_messageKey) : messageKey(_messageKey) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Pass: Statement {
	Identifier *messageKey;

	Pass(Identifier *_messageKey) : messageKey(_messageKey) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Global: Statement {
	IdentifierList *variables;

	Global(IdentifierList *_variables) : variables(_variables) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Return: Statement {
	Expression *expression;

	Return(Expression *_expression) : expression(_expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;

};

struct Put: Statement {
	Expression *expression;
	Identifier *target;

	Put(Expression *_expression, Identifier *_target)
		: expression(_expression), target(_target) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Get: Statement {
	Expression *expression;

	Get(Expression *_expression) : expression(_expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Repeat: Statement {
	StatementList *statements;

	Repeat(StatementList *_statements) : statements(_statements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
	virtual void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const {}
};

struct RepeatCount: Repeat {
	Expression *countExpression;

	RepeatCount(Expression *_countExpression, StatementList *_statements)
		: countExpression(_countExpression), Repeat(_statements) {}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatRange: Repeat {
	Identifier *variable;
	Expression *startExpression;
	Expression *endExpression;
	bool ascending;

	RepeatRange(Identifier *_variable, Expression *_startExpression, Expression *_endExpression, bool _ascending, StatementList *_statements)
		: variable(_variable), startExpression(_startExpression), endExpression(_endExpression), ascending(_ascending), Repeat(_statements) {}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatCondition: Repeat {
	Expression *condition;
	bool conditionValue;

	RepeatCondition(Expression *_condition, bool _conditionValue, StatementList *_statements)
		: condition(_condition), conditionValue(_conditionValue), Repeat(_statements) {}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
