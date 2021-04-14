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
#include "ast/Program.h"
#include "utilities/chunk.h"

// clang-format off
using namespace chatter::ast;
#include "yyParser.h"
#include "yyScanner.h"
// clang-format on

#include <sstream>

// extern int yyparse(yyscan_t scanner, chatter::ParserContext &);

CH_NAMESPACE_BEGIN

ParserContext::ParserContext(const ParserConfig &c, const std::string &s) 
    : config(c), source(s) {}

void ParserContext::error(Location location, const std::string &msg) {
    numberOfErrors++;

    config.err << config.fileName << ":" << location << ": error: "
               << msg << std::endl;

    auto lineChunk = chunk(line, location.lineNumber - 1, source);
    auto lineString = lineChunk.get();
    config.err << lineString << std::endl;

    std::string indentString;
    for (int i = 0; i < location.position - 1 && i < lineString.size(); i++) {
        if (lineString.at(i) == '\t')
            indentString += '\t';
        else
            indentString += ' ';
    }
    config.err << indentString << "^" << std::endl;
}

void Parser::parse(ParserContext &context, const std::string &source) {
    if (yylex_init(&context.scanner)) {
        return;
    }

    YY_BUFFER_STATE buf = yy_scan_string(source.c_str(), context.scanner);

    // There seems to be a bug with Flex 2.5.35 where yylineno is uninitialized.
    yyset_lineno(1, context.scanner);

    yy::parser parser(context.scanner, context);
#if defined(DEBUG) && YYDEBUG == 1
    if (context.config.enableTracing) {
        parser.set_debug_level(1);
    }
#endif
    parser.parse();

    yy_delete_buffer(buf, context.scanner);
    yylex_destroy(context.scanner);

    if (context.numberOfErrors > 0) {
        context.config.err << context.numberOfErrors
                   << (context.numberOfErrors > 1 ? " errors " : " error ") << "generated."
                   << std::endl;
    }
}

Owned<ast::Program> Parser::parseProgram(const std::string &source) {
    ParserContext context(_config, source);
    context.parsingMode = ParserContext::Program;

    parse(context, source);

    if (context.numberOfErrors && context.program) {
        context.program = nullptr;
    }

    return std::move(context.program);
}

Owned<ast::StatementList> Parser::parseStatements(const std::string &source) {
    ParserContext context(_config, source);
    context.parsingMode = ParserContext::Statements;

    parse(context, source);

    if (context.numberOfErrors && context.statements) {
        context.statements = nullptr;
    }

    return std::move(context.statements);
}

Owned<ast::Expression> Parser::parseExpression(const std::string &source) {
    ParserContext context(_config, source);
    context.parsingMode = ParserContext::Expression;

    parse(context, source);

    if (context.numberOfErrors && context.expression) {
        context.expression = nullptr;
    }

    return std::move(context.expression);
}

CH_NAMESPACE_END
