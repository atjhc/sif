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

#include "utilities/chunk.h"
#include "ast/PrettyPrinter.h"
#include "parser/Parser.h"
#include "parser/Compiler.h"
#include "parser/VirtualMachine.h"
#include "runtime/objects/Native.h"
#include "runtime/objects/List.h"
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/String.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <getopt.h>
#include <libgen.h>
#include <unistd.h>

using namespace chatter;
Map<std::string, Strong<Native>> builtins();

enum {
    Success = 0,
    ParseFailure = 1,
    CompileFailure = 2,
    RuntimeFailure = 3
};

#if defined(DEBUG)
static int traceParsing = 0;
#endif

#if defined(DEBUG)
static int traceRuntime = 0;
#endif

static bool prettyPrint = false;
static bool printBytecode = false;

VirtualMachine vm;
Map<std::string, Strong<Native>> natives = builtins();

Map<std::string, Strong<Native>> builtins() {
    Map<std::string, Strong<Native>> natives;
    natives["quit (program)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        exit(0);
    });
    natives["print (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        std::cout << values[0];
        return Value();
    });
    natives["print line (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        std::cout << values[0] << std::endl;
        return Value();
    });
    natives["print error (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        std::cerr << values[0] << std::endl;
        return Value();
    });
    natives["read word"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        std::string input;
        std::cin >> input;
        return input;
    });
    natives["read line"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        std::string input;
        std::getline(std::cin, input);
        return input;
    });
    natives["(the) size (of) (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        size_t size = 0;
        if (auto list = values[0].as<List>()) {
            size = list->values().size();
        } else if (auto dictionary = values[0].as<Dictionary>()) {
            size = dictionary->values().size();
        } else if (auto string = values[0].as<String>()) {
            size = string->string().size();
        }
        return static_cast<long>(size);
    });
    natives["item (:) of (:)"] = MakeStrong<Native>(2, [](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto list = values[1].as<List>();
        return list->values()[index];
    });
    natives["(the) type (of) (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        return values[0].typeName();
    });
    natives["sin (of) (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return sin(argument);
    });
    natives["cos (of) (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return cos(argument);
    });
    natives["tan (of) (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto argument = values[0].castFloat();
        return tan(argument);
    });
    natives["(char/character) (:) of (:)"] = MakeStrong<Native>(2, [](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::character, index, text->string()).get();
    });
    natives["(char/character) (:) to (:) of (:)"] = MakeStrong<Native>(3, [](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::character, start, end, text->string()).get();
    });
    natives["(the) (mid/middle) (char/character) of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return middle_chunk(chunk::character, text->string()).get();
    });
    natives["(the) last (char/character) of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::character, text->string()).get();
    });
    natives["(the) number of (chars/characters) in (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return static_cast<long>(count_chunk(chunk::character, text->string()).count);
    });
    natives["word (:) of (:)"] = MakeStrong<Native>(2, [](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::word, index, text->string()).get();
    });
    natives["word (:) to (:) of (:)"] = MakeStrong<Native>(3, [](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::word, start, end, text->string()).get();
    });
    natives["(the) (mid/middle) word of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return middle_chunk(chunk::word, text->string()).get();
    });
    natives["(the) last word of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::word, text->string()).get();
    });
    natives["(the) number of words in (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return static_cast<long>(count_chunk(chunk::word, text->string()).count);
    });
    natives["line (:) of (:)"] = MakeStrong<Native>(2, [](Value *values) -> Value {
        auto index = values[0].asInteger();
        auto text = values[1].as<String>();
        return index_chunk(chunk::line, index, text->string()).get();
    });
    natives["line (:) to (:) of (:)"] = MakeStrong<Native>(3, [](Value *values) -> Value {
        auto start = values[0].asInteger();
        auto end = values[1].asInteger();
        auto text = values[2].as<String>();
        return range_chunk(chunk::line, start, end, text->string()).get();
    });
    natives["(the) (mid/middle) line of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return middle_chunk(chunk::line, text->string()).get();
    });
    natives["(the) last line of (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return last_chunk(chunk::line, text->string()).get();
    });
    natives["(the) number of lines in (:)"] = MakeStrong<Native>(1, [](Value *values) -> Value {
        auto text = values[0].as<String>();
        return static_cast<long>(count_chunk(chunk::line, text->string()).count);
    });

    return natives;
}

void report(const std::string &name, Location location, const std::string &source, const std::string &message) {
    std::cerr << name << ":" << location << ": " << message << std::endl;
    std::cerr << index_chunk(chunk::line, location.lineNumber - 1, source).get() << std::endl;
    std::cerr << std::string(location.position - 1, ' ') << "^" << std::endl;
}

int evaluate(const std::string &name, const std::string &source) {
    Scanner scanner(source.c_str(), source.c_str() + source.length());
    ParserConfig parserConfig;
#if defined(DEBUG)
    parserConfig.enableTracing = traceParsing;
#endif
    Parser parser(parserConfig, scanner);
    for (const auto &pair : natives) {
        parser.declare(FunctionSignature::Make(pair.first));
    }
    auto statement = parser.parse();
    if (!statement) {
        for (auto error : parser.errors()) {
            report(name, error.token().location, source, error.what());
        }
        return ParseFailure;
    }

    if (prettyPrint) {
        auto prettyPrinter = PrettyPrinter();
        prettyPrinter.print(*statement);
        std::cout << std::endl;
        return Success;
    }

    Compiler compiler(std::move(statement));
    for (const auto &pair : natives) {
        compiler.addExtern(pair.first);
    }

    auto bytecode = compiler.compile();
    if (!bytecode) {
        for (auto error : compiler.errors()) {
            report(name, error.node().location, source, error.what());
        }
        return CompileFailure;
    }

    if (printBytecode) {
        std::cout << *bytecode;
        return Success;
    }

    vm.execute(bytecode);
    if (vm.error()) {
        report(name, vm.error().value().location(), source, vm.error().value().what());
        return RuntimeFailure;
    }

    return Success;
}

static int repl(const std::vector<std::string> &arguments) {
    std::string line;

    std::cout << "> ";
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        evaluate("<stdin>", line);
        std::cout << "> ";
    }

    return 0;
}

static int run(const std::vector<std::string> &arguments) {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return evaluate("<stdin>", ss.str());
}

static int run(const std::string &fileName, const std::vector<std::string> &arguments) {
    std::ifstream file(fileName);
    if (!file) {
        std::cerr << "can't open file " << Quoted(fileName) << std::endl;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    return evaluate(fileName, ss.str());
}

int usage(int argc, char *argv[]) {
    std::cout << "Usage: " << basename(argv[0]) << " [options...] [file]" << std::endl
#if defined(DEBUG)
              << "     --trace-parse"
              << "\t Output trace logging for the parser" << std::endl
              << "     --trace-runtime"
              << "\t Output trace logging for the runtime" << std::endl
#endif
              << " -p, --pretty-print"
              << "\t Pretty print the abstract syntax tree" << std::endl
              << " -b, --print-bytecode"
              << "\t Print generated bytecode" << std::endl
              << " -h, --help"
              << "\t Print out this help and exit" << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
#if defined(DEBUG)
        {"trace-parse", no_argument, &traceParsing, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"pretty-print", no_argument, NULL, 'p'},
        {"print-bytecode", no_argument, NULL, 'b'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "pbh", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'p':
            prettyPrint = true;
            break;
        case 'b':
            printBytecode = true;
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

    VirtualMachineConfig vmConfig;
#if defined(DEBUG)
    vmConfig.enableTracing = traceRuntime;
#endif
    vm = VirtualMachine(vmConfig);
    for (const auto &pair : natives) {
        vm.add(pair.first, pair.second);
    }

    if (fileName.empty()) {
        if (isatty(STDIN_FILENO)) {
            return repl(arguments);
        } else {
            return run(arguments);
        }
    } else {
        return run(fileName, arguments);
    }
}
