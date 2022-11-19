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

class TestReader : public Reader {
  public:
    TestReader(std::vector<std::string>::const_iterator begin,
               std::vector<std::string>::const_iterator end)
        : current(begin - 1), end(end) {}

    bool readable() const override { return current < end; }

    Optional<Error> read(int scopeDepth) override {
        current++;
        return None;
    }

    const std::string &contents() const override { return *current; }

  private:
    std::vector<std::string>::const_iterator current, end;
};

static Strong<Statement> test(const std::vector<std::string> &source) {
    ParserConfig config;
    config.scanner = MakeStrong<Scanner>();
    config.reader = MakeStrong<TestReader>(source.begin(), source.end());
    auto parser = MakeStrong<Parser>(config);
    return parser->statement();
}

TEST_CASE(ReaderTests, If) {
    ASSERT_NOT_NULL(test({
        "if true then",
        "  return",
        "end if",
    }));

    ASSERT_NOT_NULL(test({
        "if true",
        "then return",
    }));

    ASSERT_NOT_NULL(test({
        "if true",
        "then",
        "  return",
        "end if",
    }));

    ASSERT_NOT_NULL(test({
        "if true then",
        "  return",
        "else",
        "  return",
        "end if",
    }));

    ASSERT_NOT_NULL(test({
        "if true then",
        "  return",
        "else print 2",
    }));

    ASSERT_NOT_NULL(test({
        "if true then",
        "  if true then"
        "    return",
        "  end if",
        "end if",
    }));
}

TEST_CASE(ReaderTests, Repeat) {
    ASSERT_NOT_NULL(test({
        "repeat",
        "  return",
        "end repeat",
    }));

    ASSERT_NOT_NULL(test({
        "repeat forever",
        "  return",
        "end repeat",
    }));

    ASSERT_NOT_NULL(test({
        "repeat while true",
        "  return",
        "end repeat",
    }));

    ASSERT_NOT_NULL(test({
        "repeat for i in 1...10",
        "  return",
        "end repeat",
    }));

    ASSERT_NOT_NULL(test({
        "repeat",
        "  if true then",
        "    return",
        "  end if",
        "end repeat",
    }));
}

TEST_CASE(ReaderTests, Function) {
    ASSERT_NOT_NULL(test({
        "function a",
        "  return",
        "end function",
    }));
}

class ErrorReader : public Reader {
    bool readable() const override { return true; }

    Optional<Error> read(int scopeDepth) override { return Error("failed to read"); }

    const std::string &contents() const override {
        static std::string contents = "";
        return contents;
    }
};

TEST_CASE(ReaderTests, Error) {
    ParserConfig config;
    config.scanner = MakeStrong<Scanner>();
    config.reader = MakeStrong<ErrorReader>();
    auto parser = MakeStrong<Parser>(config);
    auto result = parser->statement();

    ASSERT_NULL(result);
    ASSERT_EQ(parser->errors().size(), 1);
    ASSERT_EQ(std::string(parser->errors()[0].what()), "failed to read");
};
