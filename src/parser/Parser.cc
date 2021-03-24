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

#include "Parser.h"

// clang-format off
using namespace chatter::ast;
#include "yyParser.h"
#include "yyScanner.h"
// clang-format on

#include <sstream>

extern int yyparse(yyscan_t scanner, chatter::ParserContext &);

CH_NAMESPACE_BEGIN

ParserContext::ParserContext(const ParserConfig &config, const std::string &source)
	: fileName(config.fileName), err(config.err) {

    auto ss = std::stringstream(source);
    std::string line;
    while (std::getline(ss, line, '\n')) {
        sourceLines.push_back(line);
    }
}

void ParserContext::error(const char *msg) {
    numberOfErrors++;
    auto lineNumber = currentLocation.lineNumber;
    // auto position = currentLocation.position;
    
    err << fileName << ":" << lineNumber << ": error: ";
    err << msg << std::endl;

    auto lineString = sourceLines[lineNumber - 1];
    err << lineString << std::endl;

/* TODO: Get characer position working.
    std::string indentString;
    for (int i = 0; i < position; i++) {
    	if (lineString.at(i) == '\t') indentString += '\t';
    	else indentString += ' ';
    }
    err << indentString << "^" << std::endl;
*/
}

std::unique_ptr<ast::Script> Parser::parse(const ParserConfig &config, const std::string &source) {
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
    	context.err 
    		<< context.numberOfErrors 
    		<< (context.numberOfErrors > 1 ? " errors " : " error ")
    		<< "generated." << std::endl
    	;
    }

    return std::unique_ptr<ast::Script>(context.script);
}

CH_NAMESPACE_END
