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

class Error : public std::runtime_error {
  public:
    Error(const std::string &what) : std::runtime_error(what) {}
    Error(const Location &location, const std::string &what)
        : std::runtime_error(what), _location(location) {}
    Error(const Location &location, const Value &value, const std::string &what)
        : std::runtime_error(what), _location(location), _value(value) {}

    const Location &location() const { return _location; }
    const Value &value() const { return _value; }

  private:
    Location _location;
    Value _value;
};

SIF_NAMESPACE_END
