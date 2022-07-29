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

SIF_NAMESPACE_BEGIN

bool Token::isWord() const {
    switch (type) {
    case Type::An:
    case Type::And:
    case Type::Each:
    case Type::Else:
    case Type::End:
    case Type::Exit:
    case Type::For:
    case Type::Forever:
    case Type::Function:
    case Type::If:
    case Type::In:
    case Type::Is:
    case Type::Next:
    case Type::Not:
    case Type::Or:
    case Type::Repeat:
    case Type::Return:
    case Type::Set:
    case Type::Then:
    case Type::To:
    case Type::Until:
    case Type::While:
    case Type::Global:
    case Type::Local:
    case Type::Word:
        return true;
    default:
        return false;
    }
}

Scanner::Scanner(const char *start, const char *end)
    : _start(start), _end(end), _current(start), _currentLocation{1, 1}, _skipNewlines(0) {}

Token Scanner::scan() {
    skipWhitespace();
    skipComments();

    _start = _current;
    _startLocation = _currentLocation;

    if (isAtEnd()) {
        return make(Token::Type::EndOfFile);
    }

    auto c = advance();

    if (isdigit(c))
        return scanNumber();
    if (isalpha(c) || c == '_')
        return scanWord();

    switch (c) {
    case '\n':
        _currentLocation.lineNumber++;
        _currentLocation.position = 1;
        return make(Token::Type::NewLine);
    case ';':
        return make(Token::Type::NewLine);
    case '(':
        _skipNewlines++;
        return make(Token::Type::LeftParen);
    case ')':
        _skipNewlines--;
        return make(Token::Type::RightParen);
    case '[':
        _skipNewlines++;
        return make(Token::Type::LeftBracket);
    case ']':
        _skipNewlines--;
        return make(Token::Type::RightBracket);
    case '{':
        _skipNewlines++;
        return make(Token::Type::LeftBrace);
    case '}':
        _skipNewlines--;
        return make(Token::Type::RightBrace);
    case '+':
        return make(Token::Type::Plus);
    case '-':
        return make(match('>') ? Token::Type::Arrow : Token::Type::Minus);
    case '*':
        return make(Token::Type::Star);
    case '/':
        return make(Token::Type::Slash);
    case ':':
        return make(Token::Type::Colon);
    case ',':
        return make(Token::Type::Comma);
    case '=':
        return make(Token::Type::Equal);
    case '%':
        return make(Token::Type::Percent);
    case '^':
        return make(Token::Type::Carrot);
    case '!':
        return make(match('=') ? Token::Type::NotEqual : Token::Type::Bang);
    case '<':
        return make(match('=') ? Token::Type::LessThanOrEqual : Token::Type::LessThan);
    case '>':
        return make(match('=') ? Token::Type::GreaterThanOrEqual : Token::Type::GreaterThan);
    case '"':
        return scanString('"');
    case '\'':
        return scanString('\'');
    case '.':
        if (match('.')) {
            if (match('.')) {
                return make(Token::Type::ThreeDots);
            } else if (match('<')) {
                return make(Token::Type::ClosedRange);
            }
        }
    }
    return makeError(Concat("unknown character: ", int(c)));
}

Token Scanner::scanWord() {
    while (!isAtEnd()) {
        char c = _current[0];
        if (!isalpha(c) && !isdigit(c) && c != '_') {
            break;
        }
        advance();
    }
    return make(wordType());
}

Token::Type Scanner::wordType() {
    switch (std::tolower(_start[0])) {
    case 'a':
        if (_current - _start == 1)
            return Token::Type::An;
        else if (_current - _start == 2) {
            switch (std::tolower(_start[1])) {
            case 's':
                return Token::Type::As;
            case 'n':
                return Token::Type::An;
            }
        } else if (_current - _start > 2) {
            return checkKeyword(1, 2, "nd", Token::Type::And);
        }
        break;
    case 'o':
        return checkKeyword(1, 1, "r", Token::Type::Or);
    case 'i':
        if (_current - _start == 2) {
            switch (std::tolower(_start[1])) {
            case 'f':
                return Token::Type::If;
            case 's':
                return Token::Type::Is;
            case 'n':
                return Token::Type::In;
            }
        }
        break;
    case 'e':
        if (_current - _start > 1) {
            switch (std::tolower(_start[1])) {
            case 'a':
                return checkKeyword(2, 2, "ch", Token::Type::Each);
            case 'l':
                return checkKeyword(2, 2, "se", Token::Type::Else);
            case 'n':
                return checkKeyword(2, 1, "d", Token::Type::End);
            case 'x':
                return checkKeyword(2, 2, "it", Token::Type::Exit);
            case 'm':
                return checkKeyword(2, 3, "pty", Token::Type::EmptyLiteral);
            }
        }
        break;
    case 'g':
        return checkKeyword(1, 5, "lobal", Token::Type::Global);
    case 't':
        if (_current - _start == 2 && std::tolower(_start[1]) == 'o') {
            return Token::Type::To;
        } else {
            switch (std::tolower(_start[1])) {
            case 'h':
                return checkKeyword(2, 2, "en", Token::Type::Then);
            case 'r':
                return checkKeyword(2, 2, "ue", Token::Type::BoolLiteral);
            }
        }
        break;
    case 'r':
        if (_current - _start > 1 && std::tolower(_start[1]) == 'e') {
            if (_current - _start > 2) {
                switch (std::tolower(_start[2])) {
                case 't':
                    return checkKeyword(3, 3, "urn", Token::Type::Return);
                case 'p':
                    return checkKeyword(3, 3, "eat", Token::Type::Repeat);
                }
            }
        }
        break;
    case 'b':
        return checkKeyword(1, 4, "reak", Token::Type::Break);
    case 's':
        return checkKeyword(1, 2, "et", Token::Type::Set);
    case 'f':
        if (_current - _start > 1) {
            switch (std::tolower(_start[1])) {
            case 'u':
                return checkKeyword(2, 6, "nction", Token::Type::Function);
            case 'a':
                return checkKeyword(2, 3, "lse", Token::Type::BoolLiteral);
            case 'o':
                if (_current - _start == 3 && _start[2] == 'r') {
                    return Token::Type::For;
                }
                return checkKeyword(2, 5, "rever", Token::Type::Forever);
            }
        }
    case 'l':
        return checkKeyword(1, 4, "ocal", Token::Type::Local);
    case 'n':
        if (_current - _start > 1) {
            switch (std::tolower(_start[1])) {
            case 'e':
                return checkKeyword(2, 2, "xt", Token::Type::Next);
            case 'o':
                return checkKeyword(2, 1, "t", Token::Type::Not);
            }
        }
    case 'w':
        return checkKeyword(1, 4, "hile", Token::Type::While);
    case 'u':
        return checkKeyword(1, 4, "ntil", Token::Type::Until);
    }
    return Token::Type::Word;
}

Token::Type Scanner::checkKeyword(int offset, int length, const char *name, Token::Type type) {
    if (_current - _start != offset + length) {
        return Token::Type::Word;
    }
    for (int i = 0; i < length; i++) {
        if (std::tolower(_start[offset + i]) != name[i]) {
            return Token::Type::Word;
        }
    }
    return type;
}

Token Scanner::scanString(char terminal) {
    while (!isAtEnd()) {
        char c = _current[0];
        if (c == terminal)
            break;
        if (c == '\n') {
            _currentLocation.lineNumber++;
            _currentLocation.position = 1;
        }
        advance();
    }

    if (isAtEnd()) {
        return makeError("unterminated string");
    }

    advance();
    return make(Token::Type::StringLiteral);
}

Token Scanner::scanNumber() {
    while (!isAtEnd() && isdigit(_current[0])) {
        advance();
    }

    if (!isAtEnd() && _current[0] == '.') {
        if (_end - _current > 1 && _current[1] == '.') {
            return make(Token::Type::IntLiteral);
        }
        advance();
        while (!isAtEnd() && isdigit(_current[0])) {
            advance();
        }
        return make(Token::Type::FloatLiteral);
    } else {
        return make(Token::Type::IntLiteral);
    }
}

bool Scanner::isAtEnd() { return _current == _end; }

char Scanner::advance(int count) {
    _current += count;
    _currentLocation.position += count;
    return _current[-1];
}

bool Scanner::match(const char c) {
    if (isAtEnd())
        return false;
    if (_current[0] != c)
        return false;
    _current++;
    _currentLocation.position++;
    return true;
}

char Scanner::peekNext() {
    if (_current + 1 == _end) {
        return '\0';
    }
    return _current[1];
}

Token Scanner::make(Token::Type type) {
    auto token = Token(type, _startLocation);
    token.text = std::string(_start, _current - _start);
    return token;
}

Token Scanner::makeError(const std::string &message) {
    auto token = make(Token::Type::Error);
    token.text = message;
    return token;
}

void Scanner::skipWhitespace() {
    while (!isAtEnd()) {
        char c = _current[0];
        switch (c) {
        case '\n':
            if (_skipNewlines) {
                _currentLocation.lineNumber++;
                _currentLocation.position = 1;
                advance();
                break;
            }
            return;
        case ' ':
        case '\t':
        case '\r':
            advance();
            break;
        case '#':
            skipLine();
            break;
        case '\\':
            if (_current < _end && _current[1] == '\n') {
                advance();
                advance();
            }
            break;
        case '-':
            if (_current < _end && _current[1] == '-') {
                skipLine();
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

void Scanner::skipComments() {
    if (isAtEnd()) return;

    int depth = 0;
    do {
        if (_current + 1 < _end && _current[0] == '(' && _current[1] == '-' && _current[2] == '-') {
            advance(3);
            depth++;
        } else if (depth > 0) {
            if (_current + 1 < _end && _current[0] == '-' && _current[1] == '-' && _current[2] == ')') {
                advance(3);
                depth--;
            } else {
                if (advance() == '\n') {
                    _currentLocation.lineNumber++;
                    _currentLocation.position = 1;
                }
            }
        }
    } while (depth > 0 && !isAtEnd());
}

void Scanner::skipLine() {
    while (!isAtEnd() && _current[0] != '\n') {
        advance();
    }
}

SIF_NAMESPACE_END
