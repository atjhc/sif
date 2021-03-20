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
	std::vector<Handler*> _handlers;

	Script() {}

	Script(const std::vector<Handler*> handlers) 
		: _handlers(handlers) {}

	void add(Handler *handler) {
		_handlers.push_back(handler);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};


struct Handler: Node {
	enum Kind {
		HandlerKind,
		FunctionKind
	};

	Kind _kind;
	Identifier *_messageKey;
	IdentifierList *_arguments;
	StatementList *_statements;

	Handler(Kind kind, Identifier *messageKey, IdentifierList *arguments, StatementList *statements) 
		: _kind(kind), _messageKey(messageKey), _arguments(arguments), _statements(statements) {}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct StatementList: Node {
	std::vector<Statement*> _statements;

	StatementList() {}

	StatementList(Statement *statement) {
		add(statement);
	}

	void add(Statement *statement) {
		_statements.push_back(statement);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

struct IdentifierList: Node {
	std::vector<Identifier *> _identifiers;

	IdentifierList() {}

	void add(Identifier *identifier) {
		_identifiers.push_back(identifier);
	}

	void prettyPrint(std::ostream &, PrettyPrintContext &) const override;
};

HT_AST_NAMESPACE_END
