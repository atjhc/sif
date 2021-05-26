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
#include "Utilities.h"
#include "runtime/Error.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

CH_NAMESPACE_BEGIN

class Object;

class Value {
  public:
    enum class Type : size_t {
        Bool,
        Integer,
        Float,
        Object
    };

    Value() = default;

    Value(const Value &v) = default;
    Value(Value &&v) = default;

    Value &operator=(const Value &v) {
        value = v.value;
        return *this;
    }
    Value &operator=(Value &&v) {
        value = std::move(v.value);
        return *this;
    }

    template <typename T> Value(const T &v) : value(v) {}

    Type type() const;
    std::string typeName() const;

    bool isNumber() const;

    bool isBool() const;
    bool asBool() const;

    bool isInteger() const;
    int64_t asInteger() const;
    int64_t castInteger() const;

    bool isFloat() const;
    double asFloat() const;
    double castFloat() const;

    bool isObject() const;
    Strong<Object> asObject() const;

    template <typename T>
    Strong<T> as() const {
        return isObject() ? std::dynamic_pointer_cast<T>(asObject()) : nullptr;
    }

    friend std::ostream &operator<<(std::ostream &out, const Value &value);

  private:
    std::variant<bool, int64_t, double, Strong<Object>> value;
};

std::ostream &operator<<(std::ostream &out, const std::vector<Value> &v);

CH_NAMESPACE_END
