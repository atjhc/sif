//
//  Copyright (c) 2025 James Callender
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

#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Scanner.h>
#include <sif/compiler/Signature.h>
#include <sif/runtime/VirtualMachine.h>
#include <sif/runtime/modules/Core.h>
#include <sif/runtime/ModuleLoader.h>
#include "TestSuite.h"

#include <iostream>
#include <sstream>
#include <chrono>

using namespace sif;

static std::pair<Strong<Bytecode>, std::string> compileWithDebugInfo(const std::string &source, bool enableDebugInfo) {
    std::ostringstream err;
    auto scanner = Scanner();
    auto reader = StringReader(source);
    auto loader = ModuleLoader();
    auto reporter = IOReporter(err);

    ParserConfig config{scanner, reader, loader, reporter};
    Parser parser(config);

    parser.declare(Core().signatures());
    auto statement = parser.statement();

    if (parser.failed()) {
        return {nullptr, err.str()};
    }

    CompilerConfig compilerConfig{loader, reporter, false, enableDebugInfo};
    Compiler compiler(compilerConfig);
    auto bytecode = compiler.compile(*statement);

    return {bytecode, err.str()};
}


TEST_CASE(DebugInfoIntegration, EnabledByDefault) {
    ModuleLoader loader;
    std::ostringstream err;
    IOReporter reporter(err);

    CompilerConfig config{loader, reporter, false};
    ASSERT_TRUE(config.enableDebugInfo);
}

TEST_CASE(DebugInfoIntegration, BytecodeHasArgumentRanges) {
    std::string source = R"(quit with "invalid")";

    auto [bytecodeWithDebug, errorWithDebug] = compileWithDebugInfo(source, true);
    auto [bytecodeWithoutDebug, errorWithoutDebug] = compileWithDebugInfo(source, false);

    ASSERT_TRUE(bytecodeWithDebug);
    ASSERT_TRUE(bytecodeWithoutDebug);

    bool foundRangesWithDebug = false;
    bool foundRangesWithoutDebug = false;

    auto codeWithDebug = bytecodeWithDebug->code();
    auto codeWithoutDebug = bytecodeWithoutDebug->code();

    for (size_t i = 0; i < codeWithDebug.size(); i++) {
        if (codeWithDebug[i] == Opcode::Call) {
            auto ranges = bytecodeWithDebug->argumentRanges(i);
            if (!ranges.empty()) {
                foundRangesWithDebug = true;
                break;
            }
        }
    }

    for (size_t i = 0; i < codeWithoutDebug.size(); i++) {
        if (codeWithoutDebug[i] == Opcode::Call) {
            auto ranges = bytecodeWithoutDebug->argumentRanges(i);
            if (!ranges.empty()) {
                foundRangesWithoutDebug = true;
                break;
            }
        }
    }

    ASSERT_TRUE(foundRangesWithDebug);
    ASSERT_FALSE(foundRangesWithoutDebug);
}

TEST_CASE(DebugInfoIntegration, SpecificArgumentRangeValidation) {
    std::string source = R"(replace all "old" with "new" in 123)";

    auto [bytecode, compileError] = compileWithDebugInfo(source, true);
    ASSERT_TRUE(bytecode);

    auto code = bytecode->code();
    for (size_t i = 0; i < code.size(); i++) {
        if (code[i] == Opcode::Call) {
            auto ranges = bytecode->argumentRanges(i);
            ASSERT_EQ(ranges.size(), 4);

            for (size_t j = 1; j < ranges.size(); j++) {
                ASSERT_LT(ranges[j-1].start.position, ranges[j].start.position);
            }
            break;
        }
    }
}

TEST_CASE(DebugInfoIntegration, ArgumentRangeAccuracy) {
    std::string source = R"(the abs of "invalid")";

    auto [bytecode, compileError] = compileWithDebugInfo(source, true);
    ASSERT_TRUE(bytecode);

    auto code = bytecode->code();
    for (size_t i = 0; i < code.size(); i++) {
        if (code[i] == Opcode::Call) {
            auto ranges = bytecode->argumentRanges(i);
            ASSERT_EQ(ranges.size(), 2);

            auto argRange = ranges[1];
            ASSERT_EQ(argRange.start.position, 11); // Position of opening quote
            ASSERT_EQ(argRange.end.position, 20);   // Position after closing quote
            break;
        }
    }
}
