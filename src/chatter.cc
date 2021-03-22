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
#include <libgen.h>

#include "parser/Parser.h"

using namespace hypertalk;
using namespace hypertalk::ast;

extern int yydebug;
static int prettyPrint = 0;

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

    ParserConfig config;
    config.fileName = fileName;

    std::unique_ptr<Script> result;
    if ((result = Parser().parse(config, source)) == nullptr) {
        return -1;
    }

    if (prettyPrint) {
        auto prettyPrintContext = PrettyPrintContext();
        result->prettyPrint(std::cout, prettyPrintContext);
    }

    return 0;
}

int usage(int argc, char *argv[]) {
    std::cout 
        << "Usage: " << basename(argv[0]) << " [options...] [file]" << std::endl
        << "     --trace-parse"     << "\t Output trace parsing logging"            << std::endl
        << " -p, --pretty-print"    << "\t Pretty print the abstract syntax tree"   << std::endl
        << " -h, --help"            << "\t Print out this help and exit"            << std::endl
    ;
    return -1;
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"trace-parse",     no_argument, &yydebug,       1},
        {"pretty-print",    no_argument, &prettyPrint, 'p'},
        {"help",            no_argument, &prettyPrint, 'h'},
        {0, 0, 0, 0}
    };

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "p", long_options, &option_index)) != -1) {
        switch (c) {
        case 'p':
            prettyPrint = 1;
            break;
        case 'h':
            return usage(argc, argv);
        default:
            break;
        }
    }

    std::string fileName;
    
    if (optind < argc) {
        fileName = argv[optind];
    } else {
        return usage(argc, argv);
    }

    return run(fileName);
}
