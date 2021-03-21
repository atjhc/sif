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
using namespace hypertalk::ast;
#include "yyParser.h"
#include "yyScanner.h"
// clang-format on

extern int yyparse(yyscan_t scanner, hypertalk::ParserContext &);

HT_NAMESPACE_BEGIN

std::unique_ptr<ast::Script> Parser::parse(const ParserConfig &config, const std::string &source) {
    ParserContext context(config);

    if (yylex_init(&context.scanner)) {
        return nullptr;
    }

    YY_BUFFER_STATE buf = yy_scan_string(source.c_str(), context.scanner);

    // There seems to be a bug with Flex 2.5.35 where yylineno is uninitialized.
    yyset_lineno(1, context.scanner);

    if (yyparse((yyscan_t)context.scanner, context)) {
        return nullptr;
    }
    yy_delete_buffer(buf, context.scanner);
    yylex_destroy(context.scanner);

    return std::unique_ptr<ast::Script>(context.script);
}

HT_NAMESPACE_END
