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

#include <sif/compiler/Compiler.h>
#include <sif/compiler/Parser.h>
#include <sif/compiler/Scanner.h>
#include <sif/runtime/VirtualMachine.h>
#include <sif/runtime/modules/Core.h>
#include <sif/runtime/modules/System.h>
#include <sif/runtime/objects/List.h>
#include <sif/runtime/ModuleLoader.h>
#include <sif/runtime/objects/Native.h>
#include <sif/compiler/Signature.h>
#include "tests/TestSuite.h"
#include "tests/TrackingObject.h"

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

        auto sourceOpt = suite.file_contents(path);
        ASSERT_TRUE(sourceOpt.has_value());
        if (!sourceOpt) {
            continue;
        }
        auto source = sourceOpt.value();

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

        CoreConfig coreConfig{std::mt19937_64()};
        coreConfig.randomInteger = [&coreConfig](Integer max) {
            return coreConfig.engine() % max;
        };
        Core core(coreConfig);
        SystemConfig systemConfig{out, in, err};
        System system(systemConfig);

        parser.declare(Core().signatures());
        parser.declare(System().signatures());
        parser.declare(Signature::Make("tracking object").value());
        parser.declare(Signature::Make("track count").value());
        parser.declare(Signature::Make("collect garbage").value());
        auto statement = parser.statement();

        if (!parser.failed()) {
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

                vm.addGlobal("tracking object", MakeStrong<Native>([&vm](VirtualMachine &, SourceLocation, Value *) -> Result<Value, Error> {
                    auto obj = vm.make<TrackingObject>();
                    return obj;
                }));

                vm.addGlobal("track count", MakeStrong<Native>([](VirtualMachine &, SourceLocation, Value *) -> Result<Value, Error> {
                    return Value(static_cast<Integer>(TrackingObject::count));
                }));

                vm.addGlobal("collect garbage", MakeStrong<Native>([](VirtualMachine &vm, SourceLocation, Value *) -> Result<Value, Error> {
                    vm.collectGarbage();
                    return Value();
                }));

                TrackingObject::count = 0;
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
