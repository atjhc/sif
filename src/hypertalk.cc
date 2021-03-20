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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <getopt.h>

#include "ast/ast.h"
using namespace hypertalk::ast;

// clang-format off
#include "Parser.h"
#include "Scanner.h"
// clang-format on

extern int yydebug;
static int prettyPrint = 0;

int yyparse(yyscan_t scanner, ParserContext &);

static int run(const std::string &fileName) {
    std::string source;

    if (!fileName.empty()) {
        std::ifstream file(fileName);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            source = ss.str();
        }
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();
    }

    ParserContext context;
    context.scanner = NULL;
    context.fileName = (fileName.empty() ? "<stdin>" : fileName);

    if (yylex_init(&context.scanner)) {
        return -1;
    }

    YY_BUFFER_STATE buf = yy_scan_string(source.c_str(), context.scanner);
    // There seems to be a bug with Flex 2.5.35 where yylineno is uninitialized.
    yyset_lineno(1, context.scanner);

    if (yyparse((yyscan_t)context.scanner, context)) {
        return -1;
    }
    yy_delete_buffer(buf, context.scanner);
    yylex_destroy(context.scanner);

    if (prettyPrint && context.script != nullptr) {
        auto prettyPrintContext = PrettyPrintContext();
        context.script->prettyPrint(std::cout, prettyPrintContext);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"trace-parse", no_argument, &yydebug, 1},
        {"pretty", no_argument, &prettyPrint, 'p'},
        // {"file", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    std::cout << "debug: " << yydebug << std::endl;

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "p", long_options, &option_index)) != -1) {
        switch (c) {
        // case 'd':
        // std::cout << "debug flag enabled" << std::endl;
        //     yydebug = 1;
        //     break;
        case 'p':
            prettyPrint = 1;
            break;
        default:
            break;
        }
    }

    std::string fileName;
    if (optind < argc) {
        fileName = argv[optind];
    }

    return run(fileName);
}
