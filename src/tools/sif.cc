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
#include <sif/Common.h>
#include <sif/ast/PrettyPrinter.h>
#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Reader.h>
#include <sif/runtime/ModuleLoader.h>
#include <sif/runtime/VirtualMachine.h>
#include <sif/runtime/modules/Core.h>
#include <sif/runtime/modules/System.h>
#include <sif/runtime/objects/Dictionary.h>
#include <sif/runtime/objects/List.h>
#include <sif/runtime/objects/Native.h>
#include <sif/runtime/objects/String.h>

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
static bool printBytecodeClean = false;
static bool noDebugInfo = false;
static const char *codeString = nullptr;
static bool interactive = false;

ModuleLoader loader;
VirtualMachine vm;
Core coreModule;
System systemModule;

Mapping<std::string, Value> globals;
Set<std::string> variables;
std::vector<Signature> signatures;

class REPLReader : public Reader {
  public:
    bool readable() const override { return !std::cin.eof(); }

    Optional<Error> read(int scopeDepth) override {
        std::cout << std::string(scopeDepth + 1, '>') << " ";
        std::getline(std::cin, _contents);
        return None;
    }

    const std::string &contents() const override { return _contents; }

  private:
    std::string _contents;
};

void report(const std::string &name, SourceLocation location, const std::string &source,
            const std::string &message) {
    std::cerr << name << ":" << location << ": " << message << std::endl;
    std::cerr << index_chunk(chunk::line, location.lineNumber, source).get() << std::endl;
    std::cerr << std::string(location.position, ' ') << "^" << std::endl;
}

int evaluate(const std::string &name, Reader &reader) {
    auto scanner = Scanner();
    auto reporter = BasicReporter(name, reader.contents());

    ParserConfig parserConfig{scanner, reader, loader, reporter};
#if defined(DEBUG)
    parserConfig.enableTracing = traceParsing;
#endif
    Parser parser(parserConfig);

    parser.declare(coreModule.signatures());
    parser.declare(systemModule.signatures());
    parser.declare(variables);
    parser.declare(signatures);
    parser.declare(Signature::Make("clear").value());

    vm.addGlobals(globals);

    auto statement = parser.statement();
    if (parser.failed()) {
        return ParseFailure;
    }

    if (prettyPrint) {
        auto prettyPrinter = PrettyPrinter();
        prettyPrinter.print(*statement);
        std::cout << std::endl;
        return Success;
    }

    CompilerConfig compilerConfig{loader, reporter, interactive, !noDebugInfo};
    Compiler compiler(compilerConfig);
    auto bytecode = compiler.compile(*statement);
    if (!bytecode) {
        return CompileFailure;
    }

    if (printBytecode && printBytecodeClean) {
        std::cerr << "Error: Cannot specify both -b and -B options" << std::endl;
        return ParseFailure;
    }

    if (printBytecode) {
        std::cout << *bytecode;
        return Success;
    }

    if (printBytecodeClean) {
        bytecode->printWithoutSourceLocations(std::cout);
        return Success;
    }

    auto result = vm.execute(bytecode);
    if (!result) {
        reporter.report(result.error());
        return RuntimeFailure;
    }

    variables = parser.variables();
    signatures = parser.signatures();
    globals.insert(vm.exports().begin(), vm.exports().end());

    return Success;
}

static int repl(const std::vector<std::string> &arguments) {
    while (!std::cin.eof()) {
        auto reader = REPLReader();
        evaluate("<stdin>", reader);
    }
    return 0;
}

static int run(const std::vector<std::string> &arguments) {
    std::ostringstream ss;
    ss << std::cin.rdbuf();
    auto reader = StringReader(ss.str());
    return evaluate("<stdin>", reader);
}

static int run_source(const char *source, const std::vector<std::string> &arguments) {
    auto reader = StringReader(source);
    return evaluate("<argument>", reader);
}

static int run_file(const std::string &fileName, const std::vector<std::string> &arguments) {
    auto reader = FileReader(fileName);
    std::filesystem::path directoryPath = std::filesystem::path(fileName).parent_path();
    loader.config.searchPaths.push_back(directoryPath);
    return evaluate(fileName, reader);
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
              << " -i, --interactive" << std::endl
              << "\t Run in interactive (REPL) mode." << std::endl
              << " -p, --pretty-print" << std::endl
              << "\t Pretty print the generated abstract syntax tree." << std::endl
              << " -b, --print-bytecode" << std::endl
              << "\t Print generated bytecode." << std::endl
              << " -B, --print-bytecode-clean" << std::endl
              << "\t Print generated bytecode without source locations." << std::endl
              << " -n, --no-debug-info" << std::endl
              << "\t Include argument debug information for enhanced error reporting." << std::endl
              << " -h, --help" << std::endl
              << "\t Print out this help menu." << std::endl;
    return -1;
}

#if !defined(FUZZER)
int main(int argc, char *argv[]) {

    static struct option long_options[] = {
#if defined(DEBUG)
        {"trace-parse", no_argument, &traceParsing, 1},
        {"trace-runtime", no_argument, &traceRuntime, 1},
#endif
        {"execute", required_argument, NULL, 'e'},
        {"interactive", no_argument, NULL, 'i'},
        {"pretty-print", no_argument, NULL, 'p'},
        {"print-bytecode", no_argument, NULL, 'b'},
        {"print-bytecode-clean", no_argument, NULL, 'B'},
        {"no-debug-info", no_argument, NULL, 'n'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c, opt_index = 0;
    while ((c = getopt_long(argc, argv, "pBbnhie:", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'p':
            prettyPrint = true;
            break;
        case 'b':
            printBytecode = true;
            break;
        case 'B':
            printBytecodeClean = true;
            break;
        case 'n':
            noDebugInfo = true;
            break;
        case 'e':
            codeString = optarg;
            break;
        case 'i':
            interactive = true;
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
    vm.config = vmConfig;

    for (const auto &pair : coreModule.values()) {
        vm.addGlobal(pair.first, pair.second);
    }
    for (const auto &pair : systemModule.values()) {
        vm.addGlobal(pair.first, pair.second);
    }

    vm.addGlobal("clear",
                 MakeStrong<Native>([](const NativeCallContext &context) -> Result<Value, Error> {
                     std::cout << ANSI_CLEAR_SCREEN << ANSI_RESET_CURSOR;
                     return Value();
                 }));

    if (codeString) {
        return run_source(codeString, arguments);
    }
    if (fileName.empty()) {
        if (isatty(STDIN_FILENO) || interactive) {
            interactive = true;
            return repl(arguments);
        } else {
            return run(arguments);
        }
    }
    return run_file(fileName, arguments);
}
#endif // !defined(FUZZER)

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    std::string source(reinterpret_cast<const char *>(data), size);
    run_source(source.c_str(), {});
    return 0;
}
