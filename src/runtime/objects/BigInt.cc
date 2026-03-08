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

#include "runtime/objects/BigInt.h"

#include <limits>

SIF_NAMESPACE_BEGIN

BigInt::BigInt(const ::BigInt::bigint &value) : _value(value) {}

BigInt::BigInt(const std::string &value) : _value(value) {}

BigInt::BigInt(const Value &v) {
    if (v.isInteger()) {
        _value = ::BigInt::bigint(v.asInteger());
    } else if (auto big = v.as<BigInt>()) {
        _value = big->_value;
    } else {
        throw std::runtime_error("expected numeric value");
    }
}

const ::BigInt::bigint &BigInt::value() const { return _value; }

Value BigInt::toValue(const ::BigInt::bigint &result) {
    static const ::BigInt::bigint maxInt(std::numeric_limits<long long>::max());
    static const ::BigInt::bigint minInt(std::numeric_limits<long long>::min());
    if (result >= minInt && result <= maxInt) {
        return Value(static_cast<Integer>(std::stoll(std::string(result))));
    }
    return Value(MakeStrong<BigInt>(result));
}

std::string BigInt::typeName() const { return "integer"; }

std::string BigInt::description() const { return std::string(_value); }

std::string BigInt::toString() const { return std::string(_value); }

bool BigInt::equals(Strong<Object> object) const {
    if (const auto &other = Cast<BigInt>(object)) {
        return _value == other->_value;
    }
    return false;
}

size_t BigInt::hash() const {
    return std::hash<::BigInt::bigint>{}(_value);
}

Result<Value, Error> BigInt::castFloat() const {
    try {
        return Value(std::stod(std::string(_value)));
    } catch (...) {
        return Fail(Error("value cannot be represented as a float"));
    }
}

Result<Value, Error> BigInt::castInteger() const {
    try {
        auto val = std::stoll(std::string(_value));
        return Value(static_cast<Integer>(val));
    } catch (...) {
        return Fail(Error("value cannot be represented as an integer"));
    }
}

SIF_NAMESPACE_END
