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

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <variant>

SIF_NAMESPACE_BEGIN

class Object;

class Value {
  public:
    enum class Type : size_t { Empty, Bool, Integer, Float, Object };

    Value() = default;

    Value(const Value &v) = default;
    Value(Value &&v) = default;

    template <typename T> Value(const T &v) : _value(v) {}
    Value(const std::string &string);

    Value &operator=(const Value &v) {
        _value = v._value;
        return *this;
    }

    Value &operator=(Value &&v) {
        _value = std::move(v._value);
        return *this;
    }

    Type type() const;
    std::string typeName() const;

    bool isEmpty() const;
    bool isBool() const;
    bool isInteger() const;
    bool isNumber() const;
    bool isFloat() const;
    bool isObject() const;
    bool isString() const;

    Bool asBool() const;
    Integer asInteger() const;
    Float asFloat() const;
    Strong<Object> asObject() const;

    Integer castInteger() const;
    Float castFloat() const;

    template <typename T> Strong<T> as() const {
        return isObject() ? Cast<T>(asObject()) : nullptr;
    }

    std::string toString() const;
    std::string description() const;
    std::string debugDescription() const;

    bool operator==(const Value &value) const;

    struct Hash {
        size_t operator()(const Value &) const;
    };

  private:
    std::variant<std::monostate, Bool, Integer, Float, Strong<Object>> _value;
};

std::ostream &operator<<(std::ostream &out, const Value &value);
std::ostream &operator<<(std::ostream &out, const std::vector<Value> &values);

using ValueMap = std::unordered_map<Value, Value, Value::Hash>;

SIF_NAMESPACE_END
