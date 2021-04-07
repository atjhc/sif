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

TEST_CASE(ParserTests, BasicTests) {
    for (auto path : suite.files_in("parser")) {
        chatter::ParserConfig config(path);
        chatter::Parser parser(config);

        auto source = suite.file_contents(path);
        ASSERT_NOT_NULL(parser.parseScript(source));
    }
}
