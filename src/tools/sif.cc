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
#include "runtime/modules/System.h"
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
#include <sys/utsname.h>
#include <unistd.h>

extern char **environ;

using namespace sif;

enum { Success = 0, ParseFailure = 1, CompileFailure = 2, RuntimeFailure = 3 };

static const std::string ANSI_CLEAR_SCREEN = "\033[2J";
static const std::string ANSI_RESET_CURSOR = "\033[0;0H";
static const std::string ANSI_UNDERLINE_FORMAT = "\033[4m";
static const std::string ANSI_RESET_FORMAT = "\033[0m";

#define ANSI_UNDERLINE(STR) ANSI_UNDERLINE_FORMAT << STR << ANSI_RESET_FORMAT

#if defined(DEBUG)
static int traceParsing = 0;
#endif

#if defined(DEBUG)
static int traceRuntime = 0;
#endif

static bool prettyPrint = false;
static bool printBytecode = false;
static const char *codeString = nullptr;

VirtualMachine vm;
Core coreModule;
System systemModule;

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
    parserConfig.scanner = scanner;
    parserConfig.reader = reader;
#if defined(DEBUG)
    parserConfig.enableTracing = traceParsing;
#endif
    Parser parser(parserConfig);

    for (const auto &signature : coreModule.signatures()) {
        parser.declare(signature);
    }
    for (const auto &signature : systemModule.signatures()) {
        parser.declare(signature);
    }

    parser.declare(Signature::Make("clear").value());

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

static int run_source(const char *source, const std::vector<std::string> &arguments) {
    return evaluate("<argument>", MakeStrong<StringReader>(source));
}

static int run_file(const std::string &fileName, const std::vector<std::string> &arguments) {
    return evaluate(fileName, MakeStrong<FileReader>(fileName));
}

int usage(int argc, char *argv[]) {
    std::cout << "Usage: " << basename(argv[0]) << " [options...] [file]" << std::endl
#if defined(DEBUG)
              << "     --trace-parse" << std::endl
              << "\t Output trace logging during parsing." << std::endl
              << "     --trace-runtime" << std::endl
              << "\t Output trace logging during runtime execution." << std::endl
#endif
              << " -e " << ANSI_UNDERLINE("code") << ", --execute=" << ANSI_UNDERLINE("code")
              << std::endl
              << "\t Execute " << ANSI_UNDERLINE("code") << " and exit." << std::endl
              << " -p, --pretty-print" << std::endl
              << "\t Pretty print the generated abstract syntax tree." << std::endl
              << " -b, --print-bytecode" << std::endl
              << "\t Print generated bytecode." << std::endl
              << " -h, --help" << std::endl
              << "\t Print out this help menu." << std::endl;
    return -1;
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
#if defined(DEBUG)
        {"trace-parse", no_argument, &traceParsing, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"execute", required_argument, NULL, 'e'},
        {"pretty-print", no_argument, NULL, 'p'},
        {"print-bytecode", no_argument, NULL, 'b'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "pbhe:", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'p':
            prettyPrint = true;
            break;
        case 'b':
            printBytecode = true;
            break;
        case 'e':
            codeString = optarg;
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

    struct utsname buffer;
    errno = 0;
    if (uname(&buffer) < 0) {
        perror("uname");
        return errno;
    }
    systemModule.setSystemName(buffer.sysname);
    systemModule.setSystemVersion(buffer.release);
    systemModule.setArguments(argv);
    systemModule.setEnvironment(environ);

    std::vector<std::string> arguments;
    for (int i = 1; i < argc; i++) {
        arguments.push_back(argv[i]);
    }

    VirtualMachineConfig vmConfig;
#if defined(DEBUG)
    vmConfig.enableTracing = traceRuntime;
#endif
    vm = VirtualMachine(vmConfig);

    for (const auto &pair : coreModule.functions()) {
        vm.add(pair.first, pair.second);
    }
    for (const auto &pair : systemModule.functions()) {
        vm.add(pair.first, pair.second);
    }

    vm.add("clear", MakeStrong<Native>([](CallFrame &frame, Location location,
                                          Value *values) -> Result<Value, RuntimeError> {
               std::cout << ANSI_CLEAR_SCREEN << ANSI_RESET_CURSOR;
               return Value();
           }));

    if (codeString) {
        return run_source(codeString, arguments);
    }
    if (fileName.empty()) {
        if (isatty(STDIN_FILENO)) {
            return repl(arguments);
        } else {
            return run(arguments);
        }
    }
    return run_file(fileName, arguments);
}
