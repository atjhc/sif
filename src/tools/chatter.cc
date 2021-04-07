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
#include "runtime/Runtime.h"
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

static int prettyPrint = 0;

static int run(const std::string &fileName, const std::string &messageName,
               const std::vector<std::string> &arguments) {
    std::string source;
    std::string contextName;

    ParserConfig config;

    if (!fileName.empty()) {
        std::ifstream file(fileName);
        if (file) {
            std::ostringstream ss;
            ss << file.rdbuf();
            source = ss.str();
        }
        contextName = fileName;
    } else {
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        source = ss.str();
        contextName = "<stdin>";
    }

    config.fileName = contextName;

#if defined(DEBUG)
    config.enableTracing = traceParsing;
#endif

    Parser parser(config);
    Owned<Script> result;

    if ((result = parser.parseScript(source)) == nullptr) {
        return -1;
    }

    if (prettyPrint) {
        auto prettyPrintContext = PrettyPrintContext();
        result->prettyPrint(std::cout, prettyPrintContext);
        return 0;
    }

    std::vector<Value> values;
    for (auto &argument : arguments) {
        values.push_back(Value(argument));
    }

    RuntimeConfig runtimeConfig;

#if defined(DEBUG)
    runtimeConfig.enableTracing = traceRuntime;
#endif    

    // Configure the random number generator.
    std::default_random_engine generator(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> distribution(0.0, 1.0);
    runtimeConfig.random = [&]() { return distribution(generator); };

    Runtime runtime(runtimeConfig);

    auto object = MakeStrong<Object>(fileName, result);
    try {
        runtime.send(Message(messageName, values), object);
    } catch (RuntimeError &error) {
        std::cerr << contextName << ":" << error.where << ": error: "
                  << error.what() << std::endl;
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
    static struct option long_options[] = {
#if defined(YYDEBUG)
        {"trace-parse", no_argument, &traceParsing, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"message-name", required_argument, NULL, 'm'},
        {"pretty-print", no_argument, &prettyPrint, 'p'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    std::string messageName = "begin";

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "m:ph", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'p':
            prettyPrint = 1;
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

    std::vector<std::string> arguments;
    for (int i = 1; i < argc; i++) {
        arguments.push_back(argv[i]);
    }

    return run(fileName, messageName, arguments);
}
