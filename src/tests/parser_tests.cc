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
#include "ast/PrettyPrinter.h"

#include <sstream>
#include <filesystem>
#include <iostream>

TEST_CASE(ParserTests, All) {
    for (auto pstr : suite.files_in("parser")) {
        auto path = std::filesystem::path(pstr);
        if (path.extension() != ".chatter") {
            continue;
        }

        auto resultPath = path;
        resultPath.replace_extension(".txt");

        auto expectedResult = suite.file_contents(resultPath);
        ASSERT_FALSE(expectedResult.empty());

        auto parserConfig = chatter::ParserConfig(path);
        auto parser = chatter::Parser(parserConfig);

        auto source = suite.file_contents(path);
        auto script = parser.parseScript(source);

        std::ostringstream ss;
        auto printerConfig = chatter::ast::PrettyPrinterConfig{ss, 2};
        auto printer = chatter::ast::PrettyPrinter(printerConfig);
        printer.print(*script);

        ASSERT_EQ(ss.str(), expectedResult) << "Failed: " << path << std::endl;
    }
}
