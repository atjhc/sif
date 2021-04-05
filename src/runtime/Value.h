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
#include <ostream>
#include <sstream>
#include <string>

#include <iostream>

CH_RUNTIME_NAMESPACE_BEGIN

// TODO: Add exceptions for improper conversions.
class Value {
  public:
    std::string value;

    Value() : value("") {}
    Value(const Value &v) : value(v.value) {}

    Value(const std::string &s) : value(s) {}
    Value(const char *s) : Value(std::string(s)) {}

    Value(bool v) : value((v ? "true" : "false")) {}

    Value(double v) {
        std::ostringstream ss;
        ss << v;
        value = ss.str();
    }
    Value(float f) : Value(double(f)) {}

    template<typename T>
    Value(T v) : value(std::to_string(v)) {}

    bool isEmpty() const {
        return value.empty();
    }

    bool isBool() const {
        auto lowercased = lowercase(value);
        if (lowercased == "true" || lowercased == "false") {
            return true;
        }
        return false;
    }

    bool asBool() const { return lowercase(value) == "true"; }

    bool isNumber() const { return isInteger() || isFloat(); }

    bool isInteger() const {
        char *p;
        strtol(value.c_str(), &p, 10);
        return (*p == 0);
    }

    int asInteger() const { return std::stoi(value); }

    bool isFloat() const {
        char *p;
        strtof(value.c_str(), &p);
        return (*p == 0);
    }

    float asFloat() const { return std::stof(value); }

    const std::string &asString() const { return value; }

    operator float() const { return asFloat(); }
    operator double() const { return asFloat(); }

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
    out << v.value;
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
