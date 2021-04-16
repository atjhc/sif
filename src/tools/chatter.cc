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

#include "ast/PrettyPrinter.h"
#include "parser/Parser.h"
#include "runtime/Interpreter.h"
#include "runtime/Object.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <getopt.h>
#include <libgen.h>

using namespace chatter;
using namespace chatter::ast;
using namespace chatter::runtime;

#if defined(DEBUG)
static int traceParsing = 0;
#endif

#if defined(DEBUG)
static int traceRuntime = 0;
#endif

static int prettyPrint(const std::string &fileName) {
    std::string source;
    ParserConfig config;

#if defined(DEBUG)
    config.enableTracing = traceParsing;
#endif

    if (!fileName.empty()) {
        std::ifstream file(fileName);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            source = ss.str();
        }
        config.fileName = fileName;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();
        config.fileName = "<stdin>";
    }

    Parser parser(config);
    Owned<Program> result;

    if ((result = parser.parseProgram(source)) == nullptr) {
        return -1;
    }

    auto prettyPrinter = PrettyPrinter();
    prettyPrinter.print(*result);

    return 0;
}

static int run(const std::string &fileName, const std::string &messageName,
               const std::vector<std::string> &arguments) {
    std::string source;
    std::string contextName = fileName;

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
        contextName = "<stdin>";
    }

    auto object = Object::Make(fileName, source);
    if (!object) {
        return -1;
    }

    std::vector<Value> values;
    for (auto &argument : arguments) {
        values.push_back(Value(argument));
    }

    InterpreterConfig coreConfig;
#if defined(DEBUG)
    coreConfig.enableTracing = traceRuntime;
#endif
    Interpreter core(coreConfig);

    try {
        core.send(Message(messageName, values), object);
    } catch (RuntimeError &error) {
        std::cerr << contextName << ":" << error.where << ": error: " << error.what() << std::endl;
    }

    return 0;
}

int usage(int argc, char *argv[]) {
    std::cout << "Usage: " << basename(argv[0]) << " [options...] [file]" << std::endl
#if defined(YYDEBUG)
              << "     --trace-parse"
              << "\t Output trace logging for the parser" << std::endl
              << "     --trace-runtime"
              << "\t Output trace logging for the runtime" << std::endl
#endif
              << " -m, --message-name"
              << "\t Run the specified message name (default is \"begin\")" << std::endl
              << " -p, --pretty-print"
              << "\t Pretty print the abstract syntax tree" << std::endl
              << " -h, --help"
              << "\t Print out this help and exit" << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {
    std::string messageName = "begin";
    bool shouldPrettyPrint = false;

    static struct option long_options[] = {
#if defined(YYDEBUG)
        {"trace-parse", no_argument, &traceParsing, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"message-name", required_argument, NULL, 'm'},
        {"pretty-print", no_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "m:ph", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'p':
            shouldPrettyPrint = true;
            break;
        case 'm':
            messageName = optarg;
            break;
        case 'h':
            return usage(argc, argv);
        default:
            break;
        }
    }
    argc -= optind;
    argv += optind;

    std::string fileName;
    if (argc > 0) {
        fileName = argv[0];
    }

    if (shouldPrettyPrint) {
        return prettyPrint(fileName);
    }

    std::vector<std::string> arguments;
    for (int i = 1; i < argc; i++) {
        arguments.push_back(argv[i]);
    }
    return run(fileName, messageName, arguments);
}
