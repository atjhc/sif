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
#include "Statements.h"

#include <vector>
#include <ostream>

HT_AST_NAMESPACE_BEGIN

struct Handler;
struct Statement;
struct StatementList;
struct IdentifierList;
struct Identifier;
struct Expression;

struct Script: Node {
	std::vector<std::unique_ptr<Handler>> handlers;

	Script() {}

	void add(Handler *handler) {
		handlers.push_back(std::unique_ptr<Handler>(handler));
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};


struct Handler: Node {
	enum Kind {
		HandlerKind,
		FunctionKind
	};

	Kind kind;

	std::unique_ptr<Identifier> messageKey;
	std::unique_ptr<IdentifierList> arguments;
	std::unique_ptr<StatementList> statements;

	Handler(Kind _kind, Identifier *_messageKey, IdentifierList *_arguments, StatementList *_statements) 
		: kind(_kind), messageKey(_messageKey), arguments(_arguments), statements(_statements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StatementList: Node {
	std::vector<std::unique_ptr<Statement>> statements;

	StatementList() {}

	StatementList(Statement *statement) {
		add(statement);
	}

	void add(Statement *statement) {
		statements.push_back(std::unique_ptr<Statement>(statement));
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IdentifierList: Node {
	std::vector<std::unique_ptr<Identifier>> identifiers;

	IdentifierList() {}

	void add(Identifier *identifier) {
		identifiers.push_back(std::unique_ptr<Identifier>(identifier));
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
