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
#include "parser/Scanner.h"

CH_RUNTIME_NAMESPACE_BEGIN

class SyntaxError : public std::runtime_error {
  public:
    
    SyntaxError(const Token &token, const std::string &what)
        : std::runtime_error(what), _token(token) {}

    const Token &token() const { return _token; }

  private:
    Token _token;
};

struct RuntimeError : std::runtime_error {
    ast::Location where;

    RuntimeError(const std::string &what) : std::runtime_error(what) {}

    RuntimeError(const std::string &what, const ast::Location &where)
        : std::runtime_error(what), where(where) {}
};

struct ArgumentsError : RuntimeError {
    ArgumentsError(const std::string &what) : RuntimeError(what) {}
};

struct InvalidArgumentError : ArgumentsError {
    unsigned int argumentIndex;

    InvalidArgumentError(const std::string &what, unsigned int index)
        : ArgumentsError(what), argumentIndex(index) {}
};

CH_RUNTIME_NAMESPACE_END
