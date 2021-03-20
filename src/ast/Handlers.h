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

struct Handler;
struct Statement;
struct StatementList;
struct IdentifierList;
struct Identifier;

struct Script: Node {
	std::vector<Handler*> handlers;

	Script() {}

	Script(const std::vector<Handler*> &_handlers) 
		: handlers(_handlers) {}

	void add(Handler *handler) {
		handlers.push_back(handler);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};


struct Handler: Node {
	enum Kind {
		HandlerKind,
		FunctionKind
	};

	Kind kind;
	Identifier *messageKey;
	IdentifierList *arguments;
	StatementList *statements;

	Handler(Kind _kind, Identifier *_messageKey, IdentifierList *_arguments, StatementList *_statements) 
		: kind(_kind), messageKey(_messageKey), arguments(_arguments), statements(_statements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StatementList: Node {
	std::vector<Statement*> statements;

	StatementList() {}

	StatementList(Statement *statement) {
		add(statement);
	}

	void add(Statement *statement) {
		statements.push_back(statement);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IdentifierList: Node {
	std::vector<Identifier *> identifiers;

	IdentifierList() {}

	void add(Identifier *identifier) {
		identifiers.push_back(identifier);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
