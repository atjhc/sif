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

#include "compiler/Scanner.h"
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
        Token::Type::Then,       Token::Type::RightParen, Token::Type::EndOfFile,
    };

    unsigned int i = 0;
    for (auto tokenType : expectedTokens) {
        auto token = scanner.scan();
        ASSERT_EQ(token.type, tokenType)
            << i << ": " << token.type << " != " << tokenType << std::endl;
        i++;
    }
}
