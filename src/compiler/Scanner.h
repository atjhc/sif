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

#pragma once

#include "Common.h"
#include "ast/Node.h"

#include <iostream>

CH_NAMESPACE_BEGIN

struct Token {
    enum class Type {
        Error,
        EndOfFile,
        NewLine,
        Word,
        End,
        Break,
        Next,
        Return,
        If,
        Then,
        Else,
        Function,
        Repeat,
        Forever,
        Not,
        Is,
        An,
        As,
        In,
        Comma,
        Colon,
        LeftParen,
        RightParen,
        LeftBracket,
        RightBracket,
        LeftBrace,
        RightBrace,
        Plus,
        Minus,
        Star,
        Slash,
        Equal,
        NotEqual,
        Bang,
        Set,
        To,
        While,
        Until,
        Exit,
        LessThan,
        GreaterThan,
        LessThanOrEqual,
        GreaterThanOrEqual,
        And,
        Or,
        Carrot,
        Percent,
        Arrow,
        ThreeDots,
        ClosedRange,
        StringLiteral,
        BoolLiteral,
        IntLiteral,
        FloatLiteral
    };

    Type type;
    Location location;
    std::string text;

    Token() : type(Type::EndOfFile) {}
    Token(Type type, Location location) : type(type), location(location) {}

    bool isWord() const;

    std::string description() const {
        switch (type) {
        case Type::Error:
            return "$error";
        case Type::EndOfFile:
            return "$end";
        case Type::NewLine:
            return "$nl";
        default:
            return text;
        }
    }
};

class Scanner {
  public:
    Scanner(const char *start, const char *end);

    Token scan();

  private:
    bool isAtEnd();
    char advance();
    char peekNext();
    bool match(const char);
    void skipWhitespace();
    void skipLine();

    Token scanString(char terminal);
    Token scanNumber();
    Token scanWord();

    Token make(Token::Type);
    Token makeError(const std::string &);
    Token::Type wordType();
    Token::Type checkKeyword(int offset, int length, const char *name, Token::Type type);

    const char *_start, *_end, *_current;
    Location _startLocation, _currentLocation;
    int _skipNewlines;
};

CH_NAMESPACE_END
