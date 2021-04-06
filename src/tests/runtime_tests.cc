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

#include "TestSuite.h"

#include "parser/Parser.h"
#include "runtime/Runtime.h"
#include "runtime/Object.h"
#include "utilities/devnull.h"

#include <string>

using namespace chatter;
using namespace chatter::runtime;

TEST_CASE(Runtime, All) {
    for (auto path : suite.files_in("runtime")) {
        auto pos = path.rfind(".chatter");
        if (pos == std::string::npos) {
            continue;
        }

        auto name = std::string(path.begin(), path.begin() + pos);
        auto expectedResult = suite.file_contents(name + ".txt");

        ASSERT_NEQ(expectedResult, "");

        chatter::ParserConfig config(path);
        chatter::Parser parser(config);
        auto source = suite.file_contents(path);
        auto result = parser.parseScript(source);

        ASSERT_NOT_NULL(result);

        std::ostringstream ss;
        std::istringstream iss;
        RuntimeConfig runtimeConfig(ss, devnull, iss);
        runtimeConfig.random = [&]() { return 0; };
        Runtime runtime(runtimeConfig);
        auto object = MakeStrong<Object>(path, result);
        try {
            runtime.send(RuntimeMessage("begin"), object);
        } catch (RuntimeError &error) {
            ASSERT_FAIL("runtime exception thrown");
        }

        ASSERT_EQ(ss.str(), expectedResult);
    }
}