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
#include "runtime/objects/List.h"
#include "tests/TestSuite.h"

#include <filesystem>
#include <iostream>
#include <sstream>

SIF_NAMESPACE_BEGIN

static inline std::ostream &operator<<(std::ostream &out, const Result<Value, Error> &result) {
    return out << (result.has_value() ? result.value().toString() : result.error().what());
}

static inline std::ostream &operator<<(std::ostream &out, const Error &error) {
    if (error.location().position > 0) {
        return out << error.location() << ": " << error.what();
    } else {
        return out << error.what();
    }
}

SIF_NAMESPACE_END

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
    for (auto pstr : suite.all_files_in("transcripts")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".sif") {
            continue;
        }

        std::cout << "Executing " << path << std::endl;

        auto source = suite.file_contents(path);
        ASSERT_NEQ(source, "");

        auto expectedResult = gather(source, "expect");
        auto input = gather(source, "input");

        ParserConfig config;
        config.scanner = MakeStrong<Scanner>();
        config.reader = MakeStrong<StringReader>(source);
        Parser parser(config);

        for (const auto &signature : Core().signatures()) {
            parser.declare(signature);
        }
        auto statement = parser.statement();
        ASSERT_TRUE(statement) << path << " failed to parse: " << std::endl
                               << Join(parser.errors(), "\n") << std::endl;
        if (!statement)
            continue;

        Compiler compiler;
        auto bytecode = compiler.compile(*statement);
        ASSERT_TRUE(bytecode) << path << " failed to compile" << std::endl
                              << Join(compiler.errors(), "\n") << std::endl;
        if (!bytecode)
            continue;

        VirtualMachine vm;

        std::ostringstream out, err;
        std::istringstream in(input);

        CoreConfig coreConfig{out, in, err, std::mt19937_64()};
        coreConfig.randomInteger = [&coreConfig](Integer max) {
            return coreConfig.engine() % max;
        };
        Core core(coreConfig);
        for (const auto &function : core.values()) {
            vm.addGlobal(function.first, function.second);
        }

        auto result = vm.execute(bytecode);
        ASSERT_TRUE(result.has_value()) << path << " failed: " << result;
        ASSERT_EQ(expectedResult, out.str()) << path << " failed:" << std::endl
                                            << "Expected: " << std::endl
                                            << expectedResult << std::endl
                                            << "Got: " << std::endl
                                            << out.str() << std::endl;
    }
}
