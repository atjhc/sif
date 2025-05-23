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

#include "sif/compiler/Reader.h"
#include <sif/Common.h>
#include <sif/ast/Node.h>
#include <sif/compiler/Token.h>

#include <string>

SIF_NAMESPACE_BEGIN

class Scanner {
  public:
    Scanner();

    void reset(const std::string &contents);

    Token scan();

    bool ignoreNewLines = false;
    bool interpolating = false;
    char stringTerminal = 0;

  private:
    bool isCharacter(wchar_t);

    bool isAtEnd();
    char advance(int count = 1);
    wchar_t advanceCharacter(int count = 1);
    bool match(const wchar_t);
    void skipWhitespace();
    void skipComments();
    void skipLine();

    Token scanString(char startingQuote, char terminalQuote);
    Token scanNumber();
    Token scanWord();

    Token make(Token::Type);
    Token makeError(const std::string &);
    Token::Type wordType();
    Token::Type checkKeyword(int offset, int length, const char *name, Token::Type type);

    std::string::const_iterator _start, _end, _current;
    SourceLocation _startLocation, _currentLocation;
    Strong<Reader> _reader;
};

SIF_NAMESPACE_END
