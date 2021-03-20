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
	std::vector<Expression *> _expressions;

	ExpressionList() {}

	ExpressionList(Expression *expression) {
		add(expression);
	}

	void add(Expression *expression) {
		_expressions.push_back(expression);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Identifier: Expression {
	std::string _name;

	Identifier(const std::string &name) : _name(name) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FunctionCall: Expression {
	Identifier *_identifier;
	ExpressionList *_arguments;

	FunctionCall(Identifier *identifier, ExpressionList *arguments) :
		_identifier(identifier), _arguments(arguments) {}

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

	Operator _operator;
	Expression *_left, *_right;

	BinaryOp(Operator op, Expression *left, Expression *right)
		: _operator(op), _left(left), _right(right) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct Not: Expression {
	Expression *_expression;

	Not(Expression *expression) : _expression(expression) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct FloatLiteral: Expression {
	float _value;

	FloatLiteral(float value) : _value(value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IntLiteral: Expression {
	int _value;

	IntLiteral(int value) : _value(value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StringLiteral: Expression {
	std::string _value;

	StringLiteral(const std::string &value) : _value(value) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
