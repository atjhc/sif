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
#include "Handlers.h"

#include <vector>
#include <ostream>

HT_AST_NAMESPACE_BEGIN

struct Expression: Node {

};

struct ExpressionList: Node {
	std::vector<Expression *> expressions;

	ExpressionList() {}

	ExpressionList(Expression *expression) {
		add(expression);
	}

	void add(Expression *expression) {
		expressions.push_back(expression);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Identifier: Expression {
	std::string name;

	Identifier(const std::string &_name) : name(_name) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FunctionCall: Expression {
	Identifier *identifier;
	ExpressionList *arguments;

	FunctionCall(Identifier *_identifier, ExpressionList *_arguments) :
		identifier(_identifier), arguments(_arguments) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct BinaryOp: Expression {
	enum Operator {
		Equal,
		NotEqual,
		LessThan,
		GreaterThan,
		LessThanOrEqual,
		GreaterThanOrEqual,
		IsIn,
		Contains,
		Or,
		And,
		Plus,
		Minus,
		Multiply,
		Divide
	};

	Operator op;
	Expression *left, *right;

	BinaryOp(Operator _op, Expression *_left, Expression *_right)
		: op(_op), left(_left), right(_right) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Not: Expression {
	Expression *expression;

	Not(Expression *_expression) : expression(_expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FloatLiteral: Expression {
	float value;

	FloatLiteral(float _value) : value(_value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IntLiteral: Expression {
	int value;

	IntLiteral(int _value) : value(_value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StringLiteral: Expression {
	std::string value;

	StringLiteral(const std::string &_value) : value(_value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
