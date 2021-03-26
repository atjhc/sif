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

#if defined(YYDEBUG)
extern int yydebug;
#endif

#if defined(DEBUG)
static int traceRuntime = 0;
#endif

static int prettyPrint = 0;

static int run(const std::string &fileName, const std::string &messageName,
               const std::vector<std::string> &arguments) {
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

    Owned<Script> result;
    if ((result = Parser().parse(config, source)) == nullptr) {
        return -1;
    }

    if (prettyPrint) {
        auto prettyPrintContext = PrettyPrintContext();
        result->prettyPrint(std::cout, prettyPrintContext);
        return 0;
    }

    std::vector<chatter::Value> values;
    for (auto &argument : arguments) {
        values.push_back(chatter::Value(argument));
    }

    RuntimeConfig runtimeConfig;

#if defined(DEBUG)
    runtimeConfig.tracing = traceRuntime;
#endif    

    // Configure the random number generator.
    std::default_random_engine generator(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> distribution(0.0, 1.0);
    runtimeConfig.random = [&]() { return distribution(generator); };

    Runtime runtime(fileName, runtimeConfig);
    auto object = std::make_shared<Object>(fileName, result);
    runtime.send(RuntimeMessage(messageName, values), object);

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
        {"trace-parse", no_argument, &yydebug, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"message-name", required_argument, 0, 'm'},
        {"pretty-print", no_argument, &prettyPrint, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    std::string messageName = "begin";

    int c, option_index = 0;
    while ((c = getopt_long(argc, argv, "mph", long_options, &option_index)) != -1) {
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

    std::string fileName;

    if (optind < argc) {
        fileName = argv[optind];
    }

    std::vector<std::string> arguments;
    for (int i = optind + 1; i < argc; i++) {
        arguments.push_back(argv[i]);
    }

    return run(fileName, messageName, arguments);
}
