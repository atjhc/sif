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

SIF_NAMESPACE_BEGIN

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
        For,
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
        Local,
        Global,
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
        FloatLiteral,
        EmptyLiteral,
    };

    Type type;
    Location location;
    std::string text;

    Token();
    Token(Type type, Location location);

    bool isWord() const;
    bool isEndOfStatement() const;

    std::string description() const;
    std::string debugDescription() const;
};

SIF_NAMESPACE_END
