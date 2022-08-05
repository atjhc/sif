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
#include "runtime/objects/List.h"
#include "runtime/modules/Core.h"
#include "tests/TestSuite.h"

#include <filesystem>
#include <iostream>
#include <sstream>

SIF_NAMESPACE_BEGIN

static inline std::ostream &operator<<(std::ostream &out, const Optional<RuntimeError> &error) {
    return out << (error.has_value() ? error.value().what() : "(none)");
}

static inline std::ostream &operator<<(std::ostream &out, const SyntaxError &error) {
    return out << error.token().location << ": " << error.what();
}

static inline std::ostream &operator<<(std::ostream &out, const CompileError &error) {
    return out << error.node().location << ": " << error.what();
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
    for (auto pstr : suite.files_in("transcripts")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".sif") {
            continue;
        }

        auto source = suite.file_contents(path);
        ASSERT_NEQ(source, "");

        auto expectedResult = gather(source, "expect");
        auto input = gather(source, "input");

        Scanner scanner(source.c_str(), source.c_str() + source.length());
        Parser parser(ParserConfig(), scanner);
        Core core;
        for (const auto &signature : core.signatures()) {
            parser.declare(signature);
        }
        auto statement = parser.parse();
        ASSERT_TRUE(statement) << path << " failed to parse: " << std::endl
                               << Join(parser.errors(), "\n");
        if (!statement)
            continue;

        Compiler compiler;
        auto bytecode = compiler.compile(*statement);
        ASSERT_TRUE(bytecode) << path << " failed to compile" << std::endl
                              << Join(compiler.errors(), "\n");
        if (!bytecode)
            continue;

        VirtualMachine vm;
        for (const auto &function : core.functions()) {
            vm.add(function.first, function.second);
        }

        std::ostringstream ss;
        vm.add("print {}", MakeStrong<Native>([&](Value *values) -> Value {
                   if (const auto &list = values[0].as<List>()) {
                       ss << Join(list->values(), " ");
                   } else {
                       ss << values[0];
                   }
                   ss << std::endl;
                   return Value();
               }));
        std::istringstream iss(input);
        vm.add("read (a) line", MakeStrong<Native>([&](Value *values) -> Value {
                   std::string input;
                   std::getline(iss, input);
                   return input;
               }));

        vm.execute(bytecode);
        ASSERT_FALSE(vm.error().has_value()) << path << " failed: " << vm.error();
        ASSERT_EQ(expectedResult, ss.str()) << path << " failed:" << std::endl
                                            << "Expected: " << std::endl
                                            << expectedResult << std::endl
                                            << "Got: " << std::endl
                                            << ss.str() << std::endl;
    }
}
