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

#include "Expressions.h"

HT_AST_NAMESPACE_BEGIN

void BinaryOp::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	out << "(";
	_left->prettyPrint(out, context);
	switch (_operator) {
	case Equal:
		out << " = ";
		break;
	case NotEqual:
		out << " <> ";
		break;
	case LessThan:
		out << " < ";
		break;
	case GreaterThan:
		out << " > ";
		break;
	case LessThanOrEqual:
		out << " <= ";
		break;
	case GreaterThanOrEqual:
		out << " >= ";
		break;
	case Plus:
		out << " + ";
		break;
	case Minus:
		out << " - ";
		break;
	case Multiply:
		out << " * ";
		break;
	case Divide:
		out << " / ";
		break;
	case IsIn:
		out << " is in ";
		break;
	case Contains:
		out << " contains ";
		break;
	case Or:
		out << " or ";
		break;
	case And:
		out << " and ";
		break;
	}
	_right->prettyPrint(out, context);
	out << ")";
}

void FunctionCall::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	_identifier->prettyPrint(out, context);
	out << "(";
	if (_arguments) {
		_arguments->prettyPrint(out, context);
	}
	out << ")";
}

void FloatLiteral::prettyPrint(std::ostream &out, PrettyPrintContext &) const {
	out << _value;
}

void IntLiteral::prettyPrint(std::ostream &out, PrettyPrintContext &) const {
	out << _value;
}

void StringLiteral::prettyPrint(std::ostream &out, PrettyPrintContext &) const {
	out << _value;
}

void Identifier::prettyPrint(std::ostream &out, PrettyPrintContext &) const {
	out << _name;
}

void IdentifierList::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	auto i = _identifiers.begin();
	while (i != _identifiers.end()) {
		(*i)->prettyPrint(out, context);
		i++;
		if (i != _identifiers.end()) {
			out << ", ";
		}
	}
}

void ExpressionList::prettyPrint(std::ostream &out, PrettyPrintContext &context) const {
	auto i = _expressions.begin();
	while (i < _expressions.end()) {
		(*i)->prettyPrint(out, context);
		i++;
		if (i != _expressions.end()) {
			out << ", ";
		}
	}
}

HT_AST_NAMESPACE_END
