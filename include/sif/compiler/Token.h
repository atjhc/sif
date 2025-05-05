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

#include <sif/Common.h>
#include <sif/ast/Node.h>

SIF_NAMESPACE_BEGIN

struct Token {
    enum class Type {
        An,
        And,
        Arrow,
        As,
        Bang,
        BoolLiteral,
        Break,
        Carrot,
        ClosedRange,
        Colon,
        Comma,
        Comment,
        Else,
        Empty,
        End,
        EndOfFile,
        Equal,
        Error,
        Exit,
        FloatLiteral,
        For,
        Forever,
        Function,
        Global,
        GreaterThan,
        GreaterThanOrEqual,
        If,
        In,
        IntLiteral,
        OpenInterpolation,
        Interpolation,
        ClosedInterpolation,
        Is,
        LeftBrace,
        LeftBracket,
        LeftParen,
        LessThan,
        LessThanOrEqual,
        Local,
        Minus,
        NewLine,
        Next,
        Not,
        NotEqual,
        Or,
        Percent,
        Plus,
        Repeat,
        Return,
        RightBrace,
        RightBracket,
        RightParen,
        Set,
        Slash,
        Star,
        StringLiteral,
        Then,
        ThreeDots,
        To,
        Try,
        Until,
        Use,
        Using,
        While,
        Word,
    };

    Type type;
    SourceRange range;
    std::string text;

    Token();
    Token(Type type, SourceRange range);

    bool isWord() const;
    bool isPrimary() const;
    bool isEndOfStatement() const;

    std::string encodedString() const;
    std::string encodedStringLiteralOrWord() const;
    char openingStringTerminal() const;

    std::string description() const;
    std::string debugDescription() const;
};

SIF_NAMESPACE_END
