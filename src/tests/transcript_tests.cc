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

#include "compiler/Compiler.h"
#include "compiler/Parser.h"
#include "compiler/Scanner.h"
#include "runtime/VirtualMachine.h"
#include "runtime/modules/Core.h"
#include "runtime/modules/System.h"
#include "runtime/objects/List.h"
#include "runtime/ModuleLoader.h"
#include "tests/TestSuite.h"

#include <filesystem>
#include <iostream>
#include <sstream>

using namespace sif;

static std::string gather(const std::string &source, const std::string &context) {
    const std::string search = "(-- " + context + "\n";
    std::ostringstream ss;
    size_t offset = 0;
    while (true) {
        auto start = source.find(search, offset);
        if (start == source.npos) {
            break;
        }
        start += search.size();
        offset = start;
        auto end = source.find("--)", offset);
        offset = end;
        ss << source.substr(start, end - start);
    }
    return ss.str();
}

TEST_CASE(TranscriptTests, All) {
    auto currentPath = std::filesystem::current_path();
    for (auto pstr : suite.all_files_in("transcripts")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".sif") {
            continue;
        }

        std::cout << "Executing " << path << std::endl;

        auto source = suite.file_contents(path);
        ASSERT_NEQ(source, "");

        auto expectedOutput = gather(source, "expect");
        auto expectedErrors = gather(source, "error");
        auto input = gather(source, "input");

        std::ostringstream out, err;
        std::istringstream in(input);

        auto scanner = Scanner();
        auto reader = StringReader(source);
        auto loader = ModuleLoader();
        auto reporter = IOReporter(err);

        auto directoryPath = (suite.config.resourcesPath / path).parent_path();
        std::filesystem::current_path(currentPath / directoryPath);
        loader.config.searchPaths.push_back(std::filesystem::path("./"));
        ParserConfig config{scanner, reader, loader, reporter};
        Parser parser(config);

        CoreConfig coreConfig{out, in, err, std::mt19937_64()};
        coreConfig.randomInteger = [&coreConfig](Integer max) {
            return coreConfig.engine() % max;
        };
        Core core(coreConfig);
        System system;

        for (const auto &signature : Core().signatures()) {
            parser.declare(signature);
        }
        for (const auto &signature : System().signatures()) {
            parser.declare(signature);
        }
        auto statement = parser.statement();

        if (statement) {
            Compiler compiler(CompilerConfig{loader, reporter, false});
            auto bytecode = compiler.compile(*statement);
            if (bytecode) {
                VirtualMachine vm;
                for (const auto &function : core.values()) {
                    vm.addGlobal(function.first, function.second);
                }
                for (const auto &function : system.values()) {
                    vm.addGlobal(function.first, function.second);
                }
                auto result = vm.execute(bytecode);
                if (!result) {
                    err << result.error().what() << std::endl;
                }
            }
        }

        ASSERT_EQ(expectedOutput, out.str()) << path << " failed the output check:" << std::endl
                                            << "Expected: " << std::endl
                                            << expectedOutput << std::endl
                                            << "Got: " << std::endl
                                            << out.str() << std::endl;
        ASSERT_EQ(expectedErrors, err.str()) << path << " failed the error check:" << std::endl
                                            << "Expected: " << std::endl
                                            << expectedErrors << std::endl
                                            << "Got: " << std::endl
                                            << err.str();
        std::filesystem::current_path(currentPath);
    }
}
