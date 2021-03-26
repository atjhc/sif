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

#include "parser/Parser.h"

#include "ast/Chunk.h"
#include "ast/Command.h"
#include "ast/Repeat.h"
#include "ast/Script.h"

// clang-format off
using namespace chatter::ast;
#include "yyParser.h"
#include "yyScanner.h"
// clang-format on

#include <sstream>

extern int yyparse(yyscan_t scanner, chatter::ParserContext &);

CH_NAMESPACE_BEGIN

ParserContext::ParserContext(const ParserConfig &c, const std::string &s) : config(c) {

    auto ss = std::stringstream(s);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        sourceLines.push_back(line);
    }
}

void ParserContext::error(Location location, const std::string &msg) {
    numberOfErrors++;

    config.err << config.fileName << ":" << location.lineNumber << ":" << location.position
               << ": error: ";
    config.err << msg << std::endl;

    auto lineString = sourceLines[location.lineNumber - 1];
    config.err << lineString << std::endl;

    std::string indentString;
    for (int i = 0; i < location.position; i++) {
        if (lineString.at(i) == '\t')
            indentString += '\t';
        else
            indentString += ' ';
    }
    config.err << indentString << "^" << std::endl;
}

Owned<ast::Script> Parser::parse(const ParserConfig &config, const std::string &source) {
    ParserContext context(config, source);

    if (yylex_init(&context.scanner)) {
        return nullptr;
    }

    YY_BUFFER_STATE buf = yy_scan_string(source.c_str(), context.scanner);

    // There seems to be a bug with Flex 2.5.35 where yylineno is uninitialized.
    yyset_lineno(1, context.scanner);
    yyparse((yyscan_t)context.scanner, context);
    yy_delete_buffer(buf, context.scanner);
    yylex_destroy(context.scanner);

    if (context.numberOfErrors > 0) {
        config.err << context.numberOfErrors
                   << (context.numberOfErrors > 1 ? " errors " : " error ") << "generated."
                   << std::endl;
    }

    return Owned<ast::Script>(context.script);
}

CH_NAMESPACE_END
