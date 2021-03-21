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

#include <ostream>

HT_AST_NAMESPACE_BEGIN

struct Script;

struct Location {
	unsigned int position = 1;
	unsigned int lineNumber = 1;
};

struct ParserContext {
	void *scanner = nullptr;
	Script *script = nullptr;

	std::string fileName;
	Location currentLocation;
	Location lookAheadLocation;
};

struct PrettyPrintConfig {
	unsigned int tabSize = 2;
};

struct PrettyPrintContext {
	PrettyPrintConfig config = PrettyPrintConfig();
	unsigned int indentLevel = 0;

	std::string indentString() {
		return std::string(indentLevel * config.tabSize, ' ');
	}
};

struct Node {
	Location startLocation;
	Location endLocation;

	virtual void prettyPrint(std::ostream &, PrettyPrintContext &) const = 0;
};

HT_AST_NAMESPACE_END