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

CH_NAMESPACE_BEGIN

Value::Type Value::type() const {
    return Value::Type(value.index());
}

std::string Value::typeName() const {
    switch (type()) {
    case Value::Type::Integer: return "integer";
    case Value::Type::Float: return "float";
    case Value::Type::Bool: return "bool";
    case Value::Type::Object: return asObject()->typeName();
    }
}

bool Value::isNumber() const { return isInteger() || isFloat(); }

bool Value::isBool() const {
    if (std::holds_alternative<bool>(value)) {
        return true;
    }
    return false;
}

bool Value::asBool() const {
    if (auto v = std::get_if<bool>(&value)) {
        return *v;
    }
    throw std::runtime_error("expected bool type");
}

bool Value::isInteger() const {
    if (std::holds_alternative<int64_t>(value)) {
        return true;
    }
    return false;
}

int64_t Value::asInteger() const {
    if (auto v = std::get_if<int64_t>(&value)) {
        return *v;
    }
    throw std::runtime_error("expected integer type");
}

int64_t Value::castInteger() const {
    if (auto v = std::get_if<int64_t>(&value)) {
        return *v;
    } else if (auto v = std::get_if<double>(&value)) {
        return static_cast<int64_t>(*v);
    }
    throw std::runtime_error("expected number type");
}

bool Value::isFloat() const {
    if (std::holds_alternative<double>(value)) {
        return true;
    }
    return false;
}

double Value::asFloat() const {
    if (auto v = std::get_if<double>(&value)) {
        return *v;
    }
    throw std::runtime_error("expected float type");
}

double Value::castFloat() const {
    if (auto v = std::get_if<double>(&value)) {
        return *v;
    } else if (auto v = std::get_if<int64_t>(&value)) {
        return static_cast<int64_t>(*v);
    }
    throw std::runtime_error("expected number type");
}

bool Value::isObject() const {
    if (std::holds_alternative<Strong<Object>>(value)) {
        return true;
    }
    return false;
}

Strong<Object> Value::asObject() const {
    if (auto v = std::get_if<Strong<Object>>(&value)) {
        return *v;
    }
    throw std::runtime_error("expected object type");
}

std::ostream &operator<<(std::ostream &out, const Value &value) {
    if (value.isBool()) {
        return out << (value.asBool() ? "true" : "false");
    } else if (value.isObject()) {
        return out << value.asObject()->description();
    }

    std::visit([&](auto && arg){ out << arg;}, value.value);
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
