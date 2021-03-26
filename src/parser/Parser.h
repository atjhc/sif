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
#include "ast/Script.h"

#include <iostream>

CH_NAMESPACE_BEGIN

struct ParserConfig {
    std::string fileName = "<stdin>";
    std::ostream &err = std::cerr;
};

struct ParserContext {
    ParserConfig config;

    void *scanner = nullptr;
    std::vector<std::string> sourceLines;

    ast::Script *script = nullptr;
    unsigned int numberOfErrors = 0;

    ParserContext(const ParserConfig &config, const std::string &source);

    void error(ast::Location location, const std::string &msg);
};

class Parser {
  public:
    Owned<ast::Script> parse(const ParserConfig &config, const std::string &source);
};

CH_NAMESPACE_END
