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
#include "runtime/objects/String.h"

#include <iostream>

CH_NAMESPACE_BEGIN

Value::Value(const std::string &string) : _value(MakeStrong<String>(string)) {}

Value::Type Value::type() const {
    return Value::Type(_value.index());
}

std::string Value::typeName() const {
    switch (type()) {
    case Value::Type::Integer: return "integer";
    case Value::Type::Float: return "float";
    case Value::Type::Bool: return "bool";
    case Value::Type::Object: return asObject()->typeName();
    case Value::Type::Empty: return "empty";
    }
}

bool Value::isEmpty() const { return type() == Value::Type::Empty; }

bool Value::isNumber() const { return isInteger() || isFloat(); }

bool Value::isBool() const {
    if (std::holds_alternative<bool>(_value)) {
        return true;
    }
    return false;
}

bool Value::asBool() const {
    if (auto v = std::get_if<bool>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected bool type");
}

bool Value::isInteger() const {
    if (std::holds_alternative<int64_t>(_value)) {
        return true;
    }
    return false;
}

int64_t Value::asInteger() const {
    if (auto v = std::get_if<int64_t>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected integer type");
}

int64_t Value::castInteger() const {
    if (auto v = std::get_if<int64_t>(&_value)) {
        return *v;
    } else if (auto v = std::get_if<double>(&_value)) {
        return static_cast<int64_t>(*v);
    }
    throw std::runtime_error("expected number type");
}

bool Value::isFloat() const {
    if (std::holds_alternative<double>(_value)) {
        return true;
    }
    return false;
}

double Value::asFloat() const {
    if (auto v = std::get_if<double>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected float type");
}

double Value::castFloat() const {
    if (auto v = std::get_if<double>(&_value)) {
        return *v;
    } else if (auto v = std::get_if<int64_t>(&_value)) {
        return static_cast<int64_t>(*v);
    }
    throw std::runtime_error("expected number type");
}

bool Value::isObject() const {
    if (std::holds_alternative<Strong<Object>>(_value)) {
        return true;
    }
    return false;
}

Strong<Object> Value::asObject() const {
    if (auto v = std::get_if<Strong<Object>>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected object type");
}

std::string Value::description() const {
    std::ostringstream ss;
    ss << "(" << typeName() << ") " << *this;
    return ss.str();
}

bool Value::operator==(const Value &value) const {
    if (isObject() && value.isObject()) {
        return asObject()->equals(value.asObject());
    }
    if (type() != value.type() && isNumber() && value.isNumber()) {
        return castFloat() == value.castFloat();
    }
    return _value == value._value;
}

size_t ValueHasher::operator()(const Value& value) const {
    if (value.isEmpty()) return 0;

    size_t hash;
    std::visit([&](auto arg){ hash = std::hash<decltype(arg)>{}(arg); }, value._value);
    return hash;
}

std::ostream &operator<<(std::ostream &out, const std::monostate &) {
    return out << "empty";
}

std::ostream &operator<<(std::ostream &out, const Value &value) {
    if (value.isBool()) {
        return out << (value.asBool() ? "true" : "false");
    } else if (value.isObject()) {
        return out << value.asObject()->description();
    }

    std::visit([&](auto && arg){ out << arg;}, value._value);
    return out;
}

std::ostream &operator<<(std::ostream &out, const std::vector<Value> &v) {
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

CH_NAMESPACE_END
