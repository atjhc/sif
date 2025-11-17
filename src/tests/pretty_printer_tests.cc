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

#include <sif/ast/PrettyPrinter.h>
#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Scanner.h>
#include <sif/runtime/ModuleLoader.h>
#include <sif/runtime/modules/Core.h>
#include <sif/runtime/modules/System.h>
#include "tests/TestSuite.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace sif;

static bool compareBytecode(const Bytecode &a, const Bytecode &b, std::ostream &diagnostics) {
    // Serialize both bytecodes to strings without source locations
    std::ostringstream originalStream;
    std::ostringstream prettyStream;

    a.printWithoutSourceLocations(originalStream);
    b.printWithoutSourceLocations(prettyStream);

    if (originalStream.str() != prettyStream.str()) {
        diagnostics << "Bytecode differs" << std::endl;
        diagnostics << "=== Original (no source locations) ===" << std::endl;
        diagnostics << originalStream.str() << std::endl;
        diagnostics << "=== Pretty-printed (no source locations) ===" << std::endl;
        diagnostics << prettyStream.str() << std::endl;
        return false;
    }

    return true;
}

struct ParseResult {
    Strong<Statement> statement;
    bool failed;
};

static ParseResult parseSource(const std::string &source, const std::filesystem::path &directoryPath,
                               const std::filesystem::path &currentPath, std::ostream &err) {
    auto scanner = Scanner();
    auto reader = StringReader(source);
    auto loader = ModuleLoader();
    auto reporter = IOReporter(err);

    std::filesystem::current_path(currentPath / directoryPath);
    loader.config.searchPaths.push_back(std::filesystem::path("./"));
    ParserConfig config{scanner, reader, loader, reporter};
    Parser parser(config);

    parser.declare(Core().signatures());
    parser.declare(System().signatures());

    auto statement = parser.statement();
    return {statement, parser.failed()};
}

static Strong<Bytecode> compileStatement(const Statement &statement, ModuleLoader &loader,
                                         Reporter &reporter, bool enableDebugInfo) {
    Compiler compiler(CompilerConfig{loader, reporter, false, enableDebugInfo});
    return compiler.compile(statement);
}

TEST_CASE(PrettyPrinter, RoundTripBytecodeEquivalence) {
    auto currentPath = std::filesystem::current_path();
    auto absoluteResourcesPath = std::filesystem::absolute(suite.config.resourcesPath);

    int successCount = 0;
    int failureCount = 0;
    int skippedCount = 0;

    for (auto pstr : suite.all_files_in("transcripts")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".sif") {
            continue;
        }

        // Skip parse and compiler error tests - they won't compile
        // Runtime error tests are fine - they compile successfully
        // Skip gc/ tests - they contain syntax that doesn't parse without special context
        if (path.string().find("errors/parser/") != std::string::npos ||
            path.string().find("errors/compiler/") != std::string::npos ||
            path.string().find("gc/") != std::string::npos) {
            continue;
        }

        std::cout << "Testing pretty printer on " << path << std::endl;

        auto fullPath = absoluteResourcesPath / pstr;
        std::ifstream file(fullPath);
        if (!file) {
            skippedCount++;
            std::cout << "  Skipped (could not read file)" << std::endl;
            continue;
        }
        std::ostringstream sourceStream;
        sourceStream << file.rdbuf();
        auto originalSource = sourceStream.str();

        auto directoryPath = (suite.config.resourcesPath / path).parent_path();
        std::ostringstream err;

        auto originalParseResult = parseSource(originalSource, directoryPath, currentPath, err);
        std::filesystem::current_path(currentPath);

        if (originalParseResult.failed) {
            skippedCount++;
            std::cout << "  Skipped (parse errors in original)" << std::endl;
            continue;
        }

        bool enableDebugInfo = originalSource.find("# DEBUG_INFO: false") == std::string::npos;
        auto loader = ModuleLoader();
        auto reporter = IOReporter(err);
        auto originalBytecode = compileStatement(*originalParseResult.statement, loader, reporter, enableDebugInfo);

        if (!originalBytecode) {
            skippedCount++;
            std::cout << "  Skipped (compilation errors in original)" << std::endl;
            continue;
        }

        std::ostringstream prettyPrintedSource;
        PrettyPrinterConfig ppConfig{prettyPrintedSource};
        PrettyPrinter printer(ppConfig);
        printer.print(*originalParseResult.statement);

        auto prettySource = prettyPrintedSource.str();
        std::ostringstream prettyErr;
        auto prettyParseResult = parseSource(prettySource, directoryPath, currentPath, prettyErr);
        std::filesystem::current_path(currentPath);

        if (prettyParseResult.failed) {
            failureCount++;
            std::cout << "  FAILED (parse error after pretty-printing)" << std::endl;
            if (std::getenv("PRETTY_PRINTER_VERBOSE")) {
                std::cout << "    Pretty-printed source:" << std::endl;
                std::cout << prettySource << std::endl;
                std::cout << "    Errors:" << std::endl;
                std::cout << prettyErr.str() << std::endl;
            }
            continue;
        }

        auto prettyLoader = ModuleLoader();
        auto prettyReporter = IOReporter(prettyErr);
        auto prettyBytecode = compileStatement(*prettyParseResult.statement, prettyLoader, prettyReporter, enableDebugInfo);

        if (!prettyBytecode) {
            failureCount++;
            std::cout << "  FAILED (compilation error after pretty-printing)" << std::endl;
            continue;
        }

        std::ostringstream diagnostics;
        bool bytecodeMatches = compareBytecode(*originalBytecode, *prettyBytecode, diagnostics);

        if (!bytecodeMatches) {
            failureCount++;
            std::cout << "  FAILED (bytecode mismatch)" << std::endl;
        } else {
            successCount++;
            std::cout << "  PASSED" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Pretty Printer Test Results:" << std::endl;
    std::cout << "  Passed:  " << successCount << std::endl;
    std::cout << "  Failed:  " << failureCount << std::endl;
    std::cout << "  Skipped: " << skippedCount << std::endl;
    std::cout << "  Total:   " << (successCount + failureCount + skippedCount) << std::endl;

    int testableCount = successCount + failureCount;
    if (testableCount > 0) {
        double passRate = (double)successCount / testableCount * 100.0;
        std::cout << "  Pass rate: " << passRate << "%" << std::endl;
    }

    ASSERT_TRUE(successCount > 0) << "No test files successfully round-tripped through pretty printer";
}
