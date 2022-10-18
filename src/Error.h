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
#include "compiler/Token.h"
#include "runtime/Value.h"

SIF_NAMESPACE_BEGIN

class ReadError : public std::runtime_error {
  public:
    ReadError(const std::string &what) : std::runtime_error(what) {}
};

class ParseError : public std::runtime_error {
  public:
    ParseError(const Token &token, const std::string &what)
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
    RuntimeError(const Location &location, const std::string &what, Value value)
        : std::runtime_error(what), _location(location), _value(value) {}

    RuntimeError(const Location &location, const std::string &what)
        : std::runtime_error(what), _location(location), _value(what) {}

    const Location &location() const { return _location; }

    const Value &value() const { return _value; }
  private:
    Location _location;
    Value _value;
};

SIF_NAMESPACE_END
