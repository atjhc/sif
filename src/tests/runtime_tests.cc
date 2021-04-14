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

#include "tests/TestSuite.h"

#include "parser/Parser.h"
#include "runtime/Interpreter.h"
#include "runtime/Object.h"
#include "utilities/devnull.h"

#include <string>
#include <filesystem>

using namespace chatter;
using namespace chatter::runtime;

TEST_CASE(Interpreter, All) {
    for (auto pstr : suite.files_in("runtime")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".chatter") {
            continue;
        }

        auto resultPath = path;
        resultPath.replace_extension(".txt");

        auto expectedResult = suite.file_contents(resultPath);
        ASSERT_NEQ(expectedResult, "");

        auto object = Object::Make(path, suite.file_contents(path));
        ASSERT_NOT_NULL(object);

        std::ostringstream ss;
        InterpreterConfig coreConfig(ss, devnull, idevnull);
        coreConfig.random = [&]() { return 0; };
        Interpreter core(coreConfig);

        ASSERT_NO_THROW(core.send(Message("begin"), object)) << path << std::endl;
        ASSERT_EQ(ss.str(), expectedResult) << path << std::endl;
    }
}
