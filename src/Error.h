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
#include "compiler/Scanner.h"

SIF_NAMESPACE_BEGIN

class SyntaxError : public std::runtime_error {
  public:
    SyntaxError(const Token &token, const std::string &what)
        : std::runtime_error(what), _token(token) {}

    const Token &token() const { return _token; }

  private:
    Token _token;
};

class CompileError : public std::runtime_error {
  public:
    CompileError(const Node &node, const std::string &what)
        : std::runtime_error(what), _node(node) {}

    const Node &node() const { return _node; }

  private:
    Node _node;
};

class RuntimeError : public std::runtime_error {
  public:
    RuntimeError(const Location &location, const std::string &what)
        : std::runtime_error(what), _location(location) {}

    const Location &location() const { return _location; }

  private:
    Location _location;
};

SIF_NAMESPACE_END