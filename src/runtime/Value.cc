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

#include "runtime/Value.h"
#include "runtime/Object.h"

#include <iostream>

CH_RUNTIME_NAMESPACE_BEGIN

bool Value::isEmpty() const {
    if (auto v = std::get_if<std::string>(&value)) {
        return v->empty();
    }
    return false;
}

bool Value::isNumber() const { return isInteger() || isFloat(); }

bool Value::isBool() const {
    if (std::holds_alternative<bool>(value)) {
        return true;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        auto lowercased = lowercase(*v);
        if (lowercased == "true" || lowercased == "false") {
            return true;
        }
    }
    return false;
}

bool Value::asBool() const {
    if (auto v = std::get_if<bool>(&value)) {
        return *v;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        auto lowercased = lowercase(*v);
        if (lowercased == "true") {
            return true;
        }
        if (lowercased == "false") {
            return false;
        }
        throw RuntimeError("expected true or false");
    }
    throw RuntimeError("expected boolean type");
}

bool Value::isInteger() const {
    if (std::holds_alternative<int64_t>(value)) {
        return true;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        if (v->empty()) {
            return false;
        }
        char *p;
        strtol(v->c_str(), &p, 10);
        return (*p == 0);
    }
    return false;
}

int64_t Value::asInteger() const { 
    if (auto v = std::get_if<int64_t>(&value)) {
        return *v;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        try {
            return std::stoi(*v);
        } catch (std::invalid_argument &error) {
            throw RuntimeError("expected integer type");
        }
    }
    throw RuntimeError("expected integer type");
}

bool Value::isFloat() const {
    if (std::holds_alternative<double>(value)) {
        return true;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        if (v->empty()) {
            return false;
        }
        char *p;
        strtof(v->c_str(), &p);
        return (*p == 0);
    }
    return false;
}

double Value::asFloat() const { 
    if (auto v = std::get_if<double>(&value)) {
        return *v;
    }
    if (auto v = std::get_if<int64_t>(&value)) {
        return *v;
    }
    if (auto v = std::get_if<std::string>(&value)) {
        try {
            return std::stof(*v);
        } catch (std::invalid_argument &error) {
            throw RuntimeError("expected floating point type");
        }
    }
    throw RuntimeError("expected floating point type");
}

bool Value::isString() const {
    return std::holds_alternative<std::string>(value);
}

std::string Value::asString() const { 
    if (auto v = std::get_if<std::string>(&value)) {
        return *v;
    }
    if (auto v = std::get_if<int64_t>(&value)) {
        std::ostringstream ss;
        ss << *v;
        return ss.str();
    }
    if (auto v = std::get_if<double>(&value)) {
        std::ostringstream ss;
        ss << *v;
        return ss.str();
    }
    if (auto v = std::get_if<bool>(&value)) {
        return *v ? "true" : "false";
    }
    throw RuntimeError("value has unexpected type");
}

bool Value::isObject() const {
    return std::holds_alternative<Strong<Object>>(value);
}

Strong<Object> Value::asObject() const {
    if (auto o = std::get_if<Strong<Object>>(&value)) {
        return *o;
    }
    throw RuntimeError("expected object type");
}

bool Value::operator==(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value == rhs.value;
    }
    return lowercase(asString()) == lowercase(rhs.asString());
}

bool Value::operator!=(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value != rhs.value;
    }
    return lowercase(asString()) != lowercase(rhs.asString());
}

bool Value::operator<(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value < rhs.value;
    }
    return asFloat() < rhs.asFloat();
}

bool Value::operator>(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value > rhs.value;
    }
    return asFloat() > rhs.asFloat();
}

bool Value::operator<=(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value <= rhs.value;
    }
    return asFloat() <= rhs.asFloat();
}

bool Value::operator>=(const Value &rhs) const {
    if (value.index() == rhs.value.index()) {
        return value >= rhs.value;
    }
    return asFloat() >= rhs.asFloat();
}

bool Value::operator&&(const Value &rhs) const {
    return asBool() && rhs.asBool();
}

bool Value::operator||(const Value &rhs) const {
    return asBool() || rhs.asBool();
}

bool Value::contains(const Value &rhs) const {
    return asString().find(rhs.asString()) != std::string::npos;
}

Value Value::operator+(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        if (isInteger() && rhs.isInteger()) {
            return asInteger() + rhs.asInteger();
        }
        return asFloat() + rhs.asFloat();
    } else {
        throw RuntimeError("expected number");
    }
}

Value Value::operator-(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        if (isInteger() && rhs.isInteger()) {
            return asInteger() - rhs.asInteger();
        }
        return asFloat() - rhs.asFloat();
    } else {
        throw RuntimeError("expected number");
    }
}

Value Value::operator*(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        if (isInteger() && rhs.isInteger()) {
            return asInteger() * rhs.asInteger();
        }
        return asFloat() * rhs.asFloat();
    } else {
        throw RuntimeError("expected number");
    }
}

Value Value::operator/(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        // TODO: if needed, throw runtime exception for divide by zero
        if (isInteger() && rhs.isInteger()) {
            return asInteger() / rhs.asInteger();
        }
        return asFloat() / rhs.asFloat();
    } else {
        throw RuntimeError("expected number");
    }
}

Value Value::operator%(const Value &rhs) const {
    if (isInteger() && rhs.isInteger()) {
        return asInteger() % rhs.asInteger();
    } else {
        throw std::runtime_error("expected integer");
    }
}

Value Value::operator^(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        return powf(asFloat(), rhs.asFloat());
    } else {
        throw std::runtime_error("expected number");
    }
}

CH_RUNTIME_NAMESPACE_END
