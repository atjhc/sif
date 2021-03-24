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

#include <iostream>

CH_NAMESPACE_BEGIN

bool Value::operator==(const Value &rhs) const { return lowercase(value) == lowercase(rhs.value); }

bool Value::operator!=(const Value &rhs) const { return lowercase(value) != lowercase(rhs.value); }

bool Value::operator<(const Value &rhs) const { return asFloat() < rhs.asFloat(); }

bool Value::operator>(const Value &rhs) const { return asFloat() > rhs.asFloat(); }

bool Value::operator<=(const Value &rhs) const { return asFloat() <= rhs.asFloat(); }

bool Value::operator>=(const Value &rhs) const { return asFloat() >= rhs.asFloat(); }

bool Value::operator&&(const Value &rhs) const { return asBool() && rhs.asBool(); }

bool Value::operator||(const Value &rhs) const { return asBool() || rhs.asBool(); }

bool Value::contains(const Value &rhs) const { return value.find(rhs.value) != std::string::npos; }

Value Value::operator+(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        return asFloat() + rhs.asFloat();
    } else {
        throw std::runtime_error("expected number");
    }
}

Value Value::operator-(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        return asFloat() - rhs.asFloat();
    } else {
        throw std::runtime_error("expected number");
    }
}

Value Value::operator*(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        return asFloat() * rhs.asFloat();
    } else {
        throw std::runtime_error("expected number");
    }
}

Value Value::operator/(const Value &rhs) const {
    if (isNumber() && rhs.isNumber()) {
        return asFloat() / rhs.asFloat();
    } else {
        throw std::runtime_error("expected number");
    }
}

Value Value::operator%(const Value &rhs) const {
    if (isInteger() && rhs.isInteger()) {
        return asInteger() % rhs.asInteger();
    } else {
        throw std::runtime_error("expected number");
    }
}

CH_NAMESPACE_END
