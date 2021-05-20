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

#include "Scanner.h"

#include <iostream>

CH_NAMESPACE_BEGIN

using namespace ast;

Scanner::Scanner(const char *start, const char *end)
    : _start(start), _end(end), _current(start), _currentLocation{1, 1} {}

Token Scanner::scan() {
    _skipWhitespace();
    _start = _current;
    _startLocation = _currentLocation;

    if (_isAtEnd()) {
        return _make(Token::Type::EndOfFile);
    }

    char c = _advance();

    if (isalpha(c) || c == '_') return _scanWord();
    if (isdigit(c)) return _scanNumber();

    switch (c) {
    case '\n': 
        _currentLocation.lineNumber++;
        _currentLocation.position = 1;
        return _make(Token::Type::NewLine); 
    case '(': return _make(Token::Type::LeftParen);
    case ')': return _make(Token::Type::RightParen);
    case '+': return _make(Token::Type::Plus);
    case '-': return _make(Token::Type::Minus);
    case '*': return _make(Token::Type::Star);
    case '/': return _make(Token::Type::Slash);
    case ':': return _make(Token::Type::Colon);
    case ',': return _make(Token::Type::Comma);
    case '=': return _make(Token::Type::Equal);
    case '%': return _make(Token::Type::Percent);
    case '!': return _make(_match('=') ? Token::Type::NotEqual : Token::Type::Bang);
    case '<': return _make(_match('=') ? Token::Type::LessThanOrEqual : Token::Type::LessThan);
    case '>': return _make(_match('=') ? Token::Type::GreaterThanOrEqual : Token::Type::GreaterThan);
    case '"': return _scanString();
    }
    return _makeError(String("unknown character: ", int(c)));
}

Token Scanner::_scanWord() {
    while(!_isAtEnd()) {
        char c = _current[0];
        if (!isalpha(c) && !isdigit(c) && c != '_') {
            break;
        }
        _advance();
    }
    return _make(_wordType());
}

Token::Type Scanner::_wordType() {
    switch (_start[0]) {
    case 'a': 
        if (_current - _start == 1) return Token::Type::An;
        else if (_current - _start == 2) {
            switch (_start[1]) {
            case 's': return Token::Type::As;
            case 'n': return Token::Type::An;
            }
        } else if (_current - _start > 2) {
            return _checkKeyword(1, 2, "nd", Token::Type::And);
        }
        break;
    case 'o': return _checkKeyword(1, 1, "r", Token::Type::Or);
    case 'i': 
        if (_current - _start == 2) {
            switch (_start[1]) {
            case 'f': return Token::Type::If;
            case 's': return Token::Type::Is;
            }
        }
        break;
    case 'e': 
        if (_current - _start > 1) {
            switch (_start[1]) {
            case 'l': return _checkKeyword(2, 2, "se", Token::Type::Else);
            case 'n': return _checkKeyword(2, 1, "d", Token::Type::End);
            case 'x': return _checkKeyword(2, 2, "it", Token::Type::Exit);
            }
        }
        break;
    case 't': 
        if (_current - _start == 2 && _start[1] == 'o') {
            return Token::Type::To;
        } else {
            switch (_start[1]) {
            case 'h': return _checkKeyword(2, 2, "en", Token::Type::Then);
            case 'r': return _checkKeyword(2, 2, "ue", Token::Type::BoolLiteral);
            }
            
        }
        break;
    case 'r': 
        if (_current - _start > 1 && _start[1] == 'e') {
            if (_current - _start > 2) {
                switch (_start[2]) {
                case 't': return _checkKeyword(3, 3, "urn", Token::Type::Return);
                case 'p': return _checkKeyword(3, 3, "eat", Token::Type::Repeat);
                }
            }
        }
        break;
    case 'b': return _checkKeyword(1, 4, "reak", Token::Type::Break);
    case 's': return _checkKeyword(1, 2, "et", Token::Type::Set);
    case 'f': 
        if (_current - _start > 1) {
            switch (_start[1]) {
            case 'u': return _checkKeyword(2, 6, "nction", Token::Type::Function);
            case 'a': return _checkKeyword(2, 3, "lse", Token::Type::BoolLiteral);
            case 'o': return _checkKeyword(2, 5, "rever", Token::Type::Forever);
            }
        }
    case 'n':
        if (_current - _start > 1) {
            switch (_start[1]) {
            case 'e': return _checkKeyword(2, 2, "xt", Token::Type::Next);
            case 'o': return _checkKeyword(2, 1, "t", Token::Type::Not);
            }
        }
    case 'w': return _checkKeyword(1, 4, "hile", Token::Type::While);
    case 'u': return _checkKeyword(1, 4, "ntil", Token::Type::Until);
    }
    return Token::Type::Word;
}

Token::Type Scanner::_checkKeyword(int offset, int length, const char *name, Token::Type type) {
    if (_current - _start == offset + length &&
        memcmp(_start + offset, name, length) == 0) {
    return type;
  }
  return Token::Type::Word;
}

Token Scanner::_scanString() {
    while (!_isAtEnd()) {
        char c = _current[0];
        if (c == '"') break;
        if (c == '\n') {
            _currentLocation.lineNumber++;
            _currentLocation.position = 1;
        }
        _advance();
    }

    if (_isAtEnd()) {
        return _makeError("unterminated string");
    }

    _advance();
    return _make(Token::Type::StringLiteral);
}

Token Scanner::_scanNumber() {
    while (!_isAtEnd() && isdigit(_current[0])) { 
        _advance();
    }

    if (!_isAtEnd() && _current[0] == '.') {
        _advance();
        while (!_isAtEnd() && isdigit(_current[0])) {
            _advance();
        } 
    }

    return _make(Token::Type::FloatLiteral);
}

bool Scanner::_isAtEnd() {
    return _current == _end;
}

char Scanner::_advance() {
    _current++;
    _currentLocation.position++;
    return _current[-1];
}

bool Scanner::_match(const char c) {
    if (_isAtEnd()) return false;
    if (_current[0] != c) return false;
    _current++;
    _currentLocation.position++;
    return true;
}

char Scanner::_peekNext() {
    if (_current + 1 == _end) {
        return '\0';
    }
    return _current[1];
}

Token Scanner::_make(Token::Type type) {
    auto token = Token(type, _startLocation);
    token.text = std::string(_start, _current - _start); 
    return token;
}

Token Scanner::_makeError(const std::string &message) {
    auto token = _make(Token::Type::Error);
    token.text = message;
    return token;
}

void Scanner::_skipWhitespace() {
    while (!_isAtEnd()) {
        char c = _current[0];
        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            _advance();
            break;
        case '#':
            _skipLine();
            break;
        case '-':
            if (_current < _end && _current[1] == '-') {
                _skipLine();
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

void Scanner::_skipLine() {
    while (!_isAtEnd() && _current[0] != '\n') {
        _advance();
    }
}

CH_NAMESPACE_END
