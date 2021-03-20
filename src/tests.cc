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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>

#include "ast/ast.h"
using namespace hypertalk::ast;

// clang-format off
#include "Parser.h"
#include "Scanner.h"
// clang-format on

int yyparse(yyscan_t scanner, ParserContext &);

static int parseTest(const std::string &path) {
    assert(!path.empty());

    std::string source;
    std::ifstream file(path);
    if (file) {
        std::ostringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    } else {
        std::cerr << "Could not open file at path: " << path << std::endl;
        return -1;
    }

    ParserContext context;
    context.scanner = NULL;
    context.fileName = path;

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

    return (context.script == nullptr);
}

static int runTests(const std::string &path) {
    DIR *testsDirectory = opendir(path.c_str());
    if (!testsDirectory) {
        std::cerr << "Could not open directory at path: " << path << std::endl;
    }
    
    std::vector<std::string> fileNames;
    while (struct dirent *entry = readdir(testsDirectory)) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        fileNames.push_back(name);
    }
    
    int failureCount = 0;
    int passCount = 0;
    std::cout << "Running " << fileNames.size() << " parse tests." << std::endl;
    for (auto fileName : fileNames) {
        int result = parseTest(path + "/" + fileName);
        if (result) {
            failureCount++;
        } else {
            passCount++;
        }
    }

    std::cout << "Ran " << fileNames.size() << " parse tests with " 
        << failureCount << " failures and " << passCount << " successes." << std::endl;

    return 0;
}

int main(int argc, char *argv[]) {
    return runTests(argv[1]);
}
