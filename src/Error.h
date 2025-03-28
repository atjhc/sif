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

#include <format>

SIF_NAMESPACE_BEGIN

struct Error {
    Error(const Value &value) : value(value) {}
    Error(const char *str) : Error(std::string(str)) {}

    template <class... Args>
    Error(std::format_string<Args...> fmt, Args &&...args)
        : Error(std::format(fmt, std::forward<Args>(args)...)) {}

    Error(const SourceRange &range, const Value &value) : range(range), value(value) {}
    Error(const SourceLocation &location, const Value &value)
        : range{location, location}, value(value) {}

    Error(const SourceLocation &location, const char *str)
        : Error(location, Value(std::string(str))) {}
    Error(const SourceRange &range, const char *str) : Error(range, Value(std::string(str))) {}

    template <class... Args>
    Error(const SourceLocation &location, std::format_string<Args...> fmt, Args &&...args)
        : Error(location, std::format(fmt, std::forward<Args>(args)...)) {}

    std::string what() const { return value.toString(); }

    SourceRange range;
    Value value;
};

SIF_NAMESPACE_END
