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

SIF_NAMESPACE_BEGIN

Token::Token() : type(Type::EndOfFile) {}
Token::Token(Type type, Location location) : type(type), location(location) {}

bool Token::isWord() const {
    switch (type) {
    case Type::An:
    case Type::And:
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
    case Type::Try:
    case Type::Until:
    case Type::While:
    case Type::Global:
    case Type::Local:
    case Type::Word:
    case Type::EmptyLiteral:
        return true;
    default:
        return false;
    }
}

bool Token::isEndOfStatement() const { return type == Type::NewLine || type == Type::EndOfFile; }

std::string Token::description() const {
    switch (type) {
    case Type::Error:
        return "error";
    case Type::EndOfFile:
        return "end of script";
    case Type::NewLine:
        return "new line";
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
