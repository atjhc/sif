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

#include "compiler/Token.h"
#include "utilities/strings.h"
#include <cassert>

SIF_NAMESPACE_BEGIN

Token::Token() : type(Type::EndOfFile) {}
Token::Token(Type type, Location location) : type(type), location(location) {}

bool Token::isWord() const {
    switch (type) {
    case Type::An:
    case Type::And:
    case Type::Empty:
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
    case Type::To:
    case Type::Try:
    case Type::Until:
    case Type::Use:
    case Type::Using:
    case Type::While:
    case Type::Word:
        return true;
    case Type::Else:
    case Type::Global:
    case Type::Local:
    case Type::Then:
    default:
        return false;
    }
}

bool Token::isPrimary() const {
    if (isWord())
        return true;
    switch (type) {
    case Token::Type::LeftBrace:
    case Token::Type::LeftBracket:
    case Token::Type::LeftParen:
    case Token::Type::IntLiteral:
    case Token::Type::FloatLiteral:
    case Token::Type::BoolLiteral:
    case Token::Type::StringLiteral:
    case Token::Type::Local:
    case Token::Type::Global:
        return true;
    default:
        return false;
    }
}

bool Token::isEndOfStatement() const { return type == Type::NewLine || type == Type::EndOfFile; }

std::string Token::encodedString() const {
    assert(type == Type::StringLiteral);
    return string_from_escaped_string(std::string(text.begin() + 1, text.end() - 1));
}

std::string Token::encodedStringOrWord() const {
    if (type == Type::StringLiteral) {
        return encodedString();
    }
    assert(type == Type::Word);
    return text;
}

std::string Token::description() const {
    switch (type) {
    case Type::Error:
        return "error";
    case Type::EndOfFile:
        return "end of script";
    case Type::NewLine:
        return "new line";
    case Type::StringLiteral:
        return text;
    default:
        return Quoted(text);
    }
}

std::string Token::debugDescription() const {
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

SIF_NAMESPACE_END
