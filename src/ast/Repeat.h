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

#include "Common.h"
#include "Base.h"
#include "Expressions.h"
#include "Handlers.h"

#include <vector>
#include <ostream>

CH_AST_NAMESPACE_BEGIN

struct Repeat: Statement {
	std::unique_ptr<StatementList> statements;

	Repeat(StatementList *_statements) : statements(_statements) {}

	virtual void accept(StatementVisitor &visitor) const override {
		visitor.visit(*this);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
	virtual void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const {}
};

struct RepeatCount: Repeat {
	std::unique_ptr<Expression> countExpression;

	RepeatCount(Expression *_countExpression, StatementList *_statements)
		: Repeat(_statements), countExpression(_countExpression) {}

	void accept(StatementVisitor &visitor) const override {
		visitor.visit(*this);
	}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatRange: Repeat {
	std::unique_ptr<Identifier> variable;
	std::unique_ptr<Expression> startExpression;
	std::unique_ptr<Expression> endExpression;
	bool ascending;

	RepeatRange(Identifier *_variable, Expression *_startExpression, Expression *_endExpression, bool _ascending, StatementList *_statements)
		: Repeat(_statements), variable(_variable), startExpression(_startExpression), endExpression(_endExpression), ascending(_ascending) {}

	void accept(StatementVisitor &visitor) const override {
		visitor.visit(*this);
	}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct RepeatCondition: Repeat {
	std::unique_ptr<Expression> condition;
	bool conditionValue;

	RepeatCondition(Expression *_condition, bool _conditionValue, StatementList *_statements)
		: Repeat(_statements), condition(_condition), conditionValue(_conditionValue) {}

	void accept(StatementVisitor &visitor) const override {
		visitor.visit(*this);
	}

	void prettyPrintCondition(std::ostream &, PrettyPrintContext &) const override;
};

struct ExitRepeat: Statement {
	void accept(StatementVisitor &v) const override {
		v.visit(*this);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct NextRepeat: Statement {
	void accept(StatementVisitor &v) const override {
		v.visit(*this);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

CH_AST_NAMESPACE_END
