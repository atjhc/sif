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
#include <ostream>
#include <sstream>
#include <string>
#include <variant>
#include <iostream>

CH_RUNTIME_NAMESPACE_BEGIN

// TODO: Add exceptions for improper conversions.
class Value {
  public:
    std::variant<std::string, double, long, bool> value;

    Value() : value(std::string("")) {}

    Value(const Value &v) : value(v.value) {}
    Value(Value &&v) : value(std::move(v.value)) {}
    Value& operator=(const Value &v) {
        value = v.value;
        return *this;
    }
    Value& operator=(Value &&v) {
        value = std::move(v.value);
        return *this;
    }

    template<typename T>
    Value(const T &v) : value(v) {}
    Value(const std::size_t &s) : value((long)s) {}
    
    bool isEmpty() const;

    bool isBool() const;
    bool asBool() const;

    bool isNumber() const;

    bool isInteger() const;
    long asInteger() const;

    bool isFloat() const;
    double asFloat() const;

    std::string asString() const;

    operator int() const { return asInteger(); }
    operator long() const { return asInteger(); }
    operator float() const { return asFloat(); }
    operator double() const { return asFloat(); }
    operator std::string() const { return asString(); }

    bool operator==(const Value &rhs) const;
    bool operator!=(const Value &rhs) const;
    bool operator<(const Value &rhs) const;
    bool operator>(const Value &rhs) const;
    bool operator<=(const Value &rhs) const;
    bool operator>=(const Value &rhs) const;
    bool operator&&(const Value &rhs) const;
    bool operator||(const Value &rhs) const;
    bool contains(const Value &rhs) const;
    Value operator+(const Value &rhs) const;
    Value operator-(const Value &rhs) const;
    Value operator*(const Value &rhs) const;
    Value operator/(const Value &rhs) const;
    Value operator%(const Value &rhs) const;
    Value operator^(const Value &rhs) const;
};

static inline std::ostream &operator<<(std::ostream &out, const Value &v) {
    out << v.asString();
    return out;
}

static inline std::ostream &operator<<(std::ostream &out, const std::vector<Value> &v) {
    auto i = v.begin();
    while (i != v.end()) {
        out << *i;

        i++;
        if (i != v.end()) {
            out << ", ";
        }
    }
    return out;
}

CH_RUNTIME_NAMESPACE_END
