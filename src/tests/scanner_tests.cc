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

#include <sif/compiler/Scanner.h>
#include <sif/Utilities.h>
#include "tests/TestSuite.h"

#include <filesystem>
#include <iostream>
#include <sstream>

using namespace sif;

static inline std::ostream &operator<<(std::ostream &out, Token::Type tokenType) {
    return out << RawValue(tokenType);
}

TEST_CASE(ScannerTests, All) {
    std::string source = "this is a 100 list of + tokens - if else (then) # this is a comment";

    Scanner scanner;
    scanner.reset(source);

    auto expectedTokens = std::vector<Token::Type>{
        Token::Type::Word,       Token::Type::Is,         Token::Type::An,
        Token::Type::IntLiteral, Token::Type::Word,       Token::Type::Word,
        Token::Type::Plus,       Token::Type::Word,       Token::Type::Minus,
        Token::Type::If,         Token::Type::Else,       Token::Type::LeftParen,
        Token::Type::Then,       Token::Type::RightParen, Token::Type::Comment,
        Token::Type::EndOfFile
    };

    unsigned int i = 0;
    for (auto tokenType : expectedTokens) {
        auto token = scanner.scan();
        ASSERT_EQ(token.type, tokenType)
            << i << ": " << token.type << " != " << tokenType << std::endl;
        i++;
    }
}

TEST_CASE(ScannerTests, InterpolatedString) {
    std::string source = R"(print "Hello, {name}!")";

    Scanner scanner;
    scanner.reset(source);

    // Expected tokens:
    // 1. print (Word)
    // 2. "Hello, { (OpenInterpolation) - includes both delimiters
    // 3. name (Word)
    // 4. }!" (ClosedInterpolation) - includes both delimiters: } and "
    // 5. EOF

    auto token = scanner.scan();
    ASSERT_EQ(token.type, Token::Type::Word);
    ASSERT_EQ(token.text, "print");

    token = scanner.scan();
    ASSERT_EQ(token.type, Token::Type::OpenInterpolation);
    ASSERT_EQ(token.text, "\"Hello, {");

    // Simulate what the Parser does after seeing OpenInterpolation
    scanner.interpolating = true;
    scanner.stringTerminal = '"';

    token = scanner.scan();
    ASSERT_EQ(token.type, Token::Type::Word);
    ASSERT_EQ(token.text, "name");

    // After OpenInterpolation, the } should trigger scanString and return ClosedInterpolation
    // ClosedInterpolation includes both delimiters (} and "), consistent with OpenInterpolation
    token = scanner.scan();
    ASSERT_EQ(token.type, Token::Type::ClosedInterpolation)
        << "Expected ClosedInterpolation but got " << token.type
        << " with text '" << token.text << "'";
    ASSERT_EQ(token.text, "}!\"");

    token = scanner.scan();
    ASSERT_EQ(token.type, Token::Type::EndOfFile);
}
