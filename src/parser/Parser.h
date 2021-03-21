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
#include "ast/ast.h"

#include <iostream>

HT_NAMESPACE_BEGIN

struct ParserConfig {
	std::string fileName = "<stdin>";
	std::ostream &err = std::cerr;
};

struct ParserContext {
	void *scanner = nullptr;
	ast::Script *script = nullptr;

	std::string fileName;
	std::ostream &err;

	ast::Location currentLocation;
	ast::Location lookAheadLocation;

	ParserContext(const ParserConfig &config)
		: fileName(config.fileName), err(config.err) {}
};

class Parser {
public:
	std::unique_ptr<ast::Script> parse(const ParserConfig &config, const std::string &source);
};

HT_NAMESPACE_END
