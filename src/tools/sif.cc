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

#include "Common.h"
#include "ast/PrettyPrinter.h"
#include "compiler/Compiler.h"
#include "compiler/Parser.h"
#include "compiler/Reader.h"
#include "runtime/VirtualMachine.h"
#include "runtime/modules/Core.h"
#include "runtime/objects/Dictionary.h"
#include "runtime/objects/List.h"
#include "runtime/objects/Native.h"
#include "runtime/objects/String.h"
#include "utilities/chunk.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#include <getopt.h>
#include <libgen.h>
#include <unistd.h>

using namespace sif;
Mapping<std::string, Strong<Native>> builtins();

enum { Success = 0, ParseFailure = 1, CompileFailure = 2, RuntimeFailure = 3 };

#if defined(DEBUG)
static int traceParsing = 0;
#endif

#if defined(DEBUG)
static int traceRuntime = 0;
#endif

static bool prettyPrint = false;
static bool printBytecode = false;

VirtualMachine vm;

class FileReader : public Reader {
  public:
    FileReader(const std::string &path) : _path(path) {}

    bool readable() const override { return false; }

    Optional<ReadError> read(int scopeDepth) override {
        std::ifstream file(_path);
        if (!file) {
            return ReadError(Concat("can't open file ", Quoted(_path)));
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        _contents = ss.str();
        return None;
    }

    const std::string &contents() const override { return _contents; }

  private:
    std::string _path;
    std::string _contents;
};

class REPLReader : public Reader {
  public:
    bool readable() const override { return !std::cin.eof(); }

    Optional<ReadError> read(int scopeDepth) override {
        std::cout << std::string(scopeDepth + 1, '>') << " ";
        std::getline(std::cin, _contents);
        return None;
    }

    const std::string &contents() const override { return _contents; }

  private:
    std::string _contents;
};

void report(const std::string &name, Location location, const std::string &source,
            const std::string &message) {
    std::cerr << name << ":" << location << ": " << message << std::endl;
    std::cerr << index_chunk(chunk::line, location.lineNumber - 1, source).get() << std::endl;
    std::cerr << std::string(location.position - 1, ' ') << "^" << std::endl;
}

int evaluate(const std::string &name, Strong<Reader> reader) {
    auto scanner = MakeStrong<Scanner>();

    ParserConfig parserConfig;
#if defined(DEBUG)
    parserConfig.enableTracing = traceParsing;
#endif
    Parser parser(parserConfig, scanner, reader);

    Core core;
    for (const auto &signature : core.signatures()) {
        parser.declare(signature);
    }

    auto statement = parser.statement();
    if (!statement) {
        for (auto error : parser.errors()) {
            report(name, error.token().location, reader->contents(),
                   Concat("parse error, ", error.what()));
        }
        return ParseFailure;
    }

    if (prettyPrint) {
        auto prettyPrinter = PrettyPrinter();
        prettyPrinter.print(*statement);
        std::cout << std::endl;
        return Success;
    }

    Compiler compiler;
    auto bytecode = compiler.compile(*statement);
    if (!bytecode) {
        for (auto error : compiler.errors()) {
            report(name, error.node().location, reader->contents(),
                   Concat("compile error, ", error.what()));
        }
        return CompileFailure;
    }

    if (printBytecode) {
        std::cout << *bytecode;
        return Success;
    }

    vm.execute(bytecode);
    if (auto error = vm.error()) {
        std::cerr << name << ":" << error.value().location().lineNumber << ": "
                  << Concat("runtime error, ", error.value().what()) << std::endl;
        std::cerr << index_chunk(chunk::line, error.value().location().lineNumber - 1,
                                 reader->contents())
                         .get()
                  << std::endl;
        return RuntimeFailure;
    }

    return Success;
}

static int repl(const std::vector<std::string> &arguments) {
    while (!std::cin.eof()) {
        evaluate("<stdin>", MakeStrong<REPLReader>());
    }
    return 0;
}

static int run(const std::vector<std::string> &arguments) {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    return evaluate("<stdin>", MakeStrong<StringReader>(ss.str()));
}

static int run(const std::string &fileName, const std::vector<std::string> &arguments) {
    return evaluate(fileName, MakeStrong<FileReader>(fileName));
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

    Core core;
    for (const auto &pair : core.functions()) {
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
