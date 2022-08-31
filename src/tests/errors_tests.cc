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

#include "compiler/Parser.h"
#include "compiler/Reader.h"
#include "compiler/Scanner.h"
#include "tests/TestSuite.h"

#include <string>
#include <vector>

using namespace sif;

static std::vector<ParseError> errors(const std::string &source) {
    auto parser =
        MakeStrong<Parser>(ParserConfig(), MakeStrong<Scanner>(), MakeStrong<StringReader>(source));
    parser->statement();
    return parser->errors();
}

TEST_CASE(ErrorsTests, ErrorRecoveryForBlockStatements) {
    ASSERT_EQ(2, errors(
        "function a ...\n"
        "  exit repeat\n"
        "end function\n"
    ).size());

    ASSERT_EQ(1, errors(
        "if true print 1\n"
    ).size());

    ASSERT_EQ(1, errors(
        "if true true then set a to 1\n"
    ).size());

    ASSERT_EQ(2, errors(
        "if true true then\n"
        "  set a to\n"
        "end if\n"
    ).size());

    ASSERT_EQ(1, errors(
        "if true true then set a to 1\n"
        "else set a to 1\n"
    ).size());

    ASSERT_EQ(2, errors(
        "if true true then set a to 1\n"
        "else set a to\n"
    ).size());

    ASSERT_EQ(3, errors(
        "if true true then set a to\n"
        "else set a to\n"
    ).size());

    ASSERT_EQ(1, errors(
        "repeat a\n"
        "  set a to 1\n"
        "end repeat\n"
    ).size());

    ASSERT_EQ(1, errors(
        "repeat while\n"
        " set a to 1\n"
        "end repeat"
    ).size());

    ASSERT_EQ(1, errors(
        "repeat until\n"
        " set a to 1\n"
        "end repeat\n"
    ).size());

    ASSERT_EQ(1, errors(
        "repeat for\n"
        " set a to 1\n"
        "end repeat\n"
    ).size());
}

TEST_CASE(ErrorsTests, NextRepeatEmbeddedFunction) {
    ASSERT_EQ(1, errors(
        "repeat while false\n"
        "  function a\n"
        "    next repeat\n"
        "  end function\n"
        "end repeat\n"
    ).size());

    ASSERT_EQ(1, errors(
        "repeat while false\n"
        "  function a\n"
        "    exit repeat\n"
        "  end function\n"
        "end repeat\n"
    ).size());

    ASSERT_EQ(0, errors(
        "repeat while false\n"
        "  function a\n"
        "    repeat while false\n"
        "      next repeat\n"
        "    end repeat\n"
        "  end function\n"
        "end repeat\n"
    ).size());

    ASSERT_EQ(0, errors(
        "repeat while false\n"
        "  function a\n"
        "    repeat while false\n"
        "      exit repeat\n"
        "    end repeat\n"
        "  end function\n"
        "end repeat\n"
    ).size());
}

TEST_CASE(ErrorsTests, DuplicateFunctionArgumentNames) {
    ASSERT_EQ(1, errors(
        "function a {b} {b}\n"
        "end function\n"
    ).size());
}
