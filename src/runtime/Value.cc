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

#include "sif/runtime/Object.h"
#include <sif/runtime/Value.h>

#include "sif/runtime/objects/String.h"

#include <format>
#include <iostream>

SIF_NAMESPACE_BEGIN

Value::Value(const std::string &string) : _value(MakeStrong<String>(string)) {}

Value::Type Value::type() const { return Value::Type(_value.index()); }

std::string Value::typeName() const {
    switch (type()) {
    case Value::Type::Empty:
        return "empty";
    case Value::Type::Integer:
        return "integer";
    case Value::Type::Float:
        return "float";
    case Value::Type::Bool:
        return "bool";
    case Value::Type::Object:
        return asObject()->typeName();
    }
    // Unreachable, but GCC requires a return statement
    return "unknown";
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
    if (std::holds_alternative<Integer>(_value)) {
        return true;
    }
    return false;
}

Integer Value::asInteger() const {
    if (auto v = std::get_if<Integer>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected integer type");
}

Integer Value::castInteger() const {
    if (auto v = std::get_if<Integer>(&_value)) {
        return *v;
    } else if (auto v = std::get_if<Float>(&_value)) {
        return static_cast<Integer>(*v);
    }
    throw std::runtime_error("can't convert value to number");
}

bool Value::isFloat() const {
    if (std::holds_alternative<Float>(_value)) {
        return true;
    }
    return false;
}

Float Value::asFloat() const {
    if (auto v = std::get_if<Float>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected float type");
}

Float Value::castFloat() const {
    if (auto v = std::get_if<Float>(&_value)) {
        return *v;
    } else if (auto v = std::get_if<Integer>(&_value)) {
        return static_cast<Float>(*v);
    }
    throw std::runtime_error("can't convert value to number");
}

bool Value::isObject() const {
    if (std::holds_alternative<Strong<Object>>(_value)) {
        return true;
    }
    return false;
}

bool Value::isString() const { return isObject() && as<String>() != nullptr; }

Strong<Object> Value::asObject() const {
    if (auto v = std::get_if<Strong<Object>>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected object type");
}

Strong<Object> &Value::reference() {
    if (auto v = std::get_if<Strong<Object>>(&_value)) {
        return *v;
    }
    throw std::runtime_error("expected object type");
}

std::ostream &operator<<(std::ostream &out, const std::monostate &) { return out << "empty"; }

std::string Value::toString() const {
    if (isObject()) {
        return asObject()->toString();
    }
    return description();
}

std::string Value::description() const {
    return std::visit(
        Overload{[](auto &&arg) -> std::string { return std::format("{}", arg); },
                 [](bool boolValue) -> std::string { return boolValue ? "yes" : "no"; },
                 [](std::monostate mono) -> std::string { return "empty"; },
                 [](Strong<Object> object) -> std::string { return object->description(); }},
        _value);
}

std::string Value::debugDescription() const {
    std::ostringstream ss;
    if (isObject()) {
        ss << "(" << typeName() << ") " << asObject()->debugDescription();
    } else {
        ss << "(" << typeName() << ") " << *this;
    }
    return ss.str();
}

bool Value::operator==(const Value &value) const {
    if (auto string = as<String>(); string && string->string().size() == 0 && value.isEmpty()) {
        return true;
    }
    if (auto string = value.as<String>(); string && string->string().size() == 0 && isEmpty()) {
        return true;
    }
    if (isObject() && value.isObject()) {
        auto lhs = asObject();
        auto rhs = value.asObject();
        return lhs->equals(rhs);
    }
    if (type() != value.type() && isNumber() && value.isNumber()) {
        return castFloat() == value.castFloat();
    }
    return _value == value._value;
}

size_t Value::Hash::operator()(const Value &value) const {
    if (value.isEmpty())
        return 0;
    if (value.isObject()) {
        if (auto string = value.as<String>(); string && string->string().empty()) {
            return 0;
        }
        return value.asObject()->hash();
    }
    if (value.isInteger()) {
        return std::hash<Float>{}(static_cast<Float>(value.asInteger()));
    }
    size_t hash;
    std::visit([&](auto arg) { hash = std::hash<decltype(arg)>{}(arg); }, value._value);
    return hash;
}

std::ostream &operator<<(std::ostream &out, const Value &value) { return out << value.toString(); }

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

SIF_NAMESPACE_END
