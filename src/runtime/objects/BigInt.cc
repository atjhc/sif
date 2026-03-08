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

#include <cmath>
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

const ::BigInt::bigint &BigInt::bigint() const { return _value; }

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

double BigInt::toDouble(const ::BigInt::bigint &value) {
    static const ::BigInt::bigint maxLL(std::numeric_limits<long long>::max());
    static const ::BigInt::bigint minLL(std::numeric_limits<long long>::min());
    if (value >= minLL && value <= maxLL) {
        return std::stod(std::string(value));
    }
    std::string s = std::string(value);
    bool negative = s[0] == '-';
    size_t start = negative ? 1 : 0;
    size_t numDigits = s.size() - start;
    // Parse leading significant digits and scale by remaining magnitude
    double mantissa = std::stod(s.substr(start, 18));
    double result = mantissa * std::pow(10.0, static_cast<double>(numDigits - 18));
    return negative ? -result : result;
}

Result<Value, Error> BigInt::castFloat() const {
    try {
        return Value(toDouble(_value));
    } catch (...) {
        return Fail(Error("value cannot be represented as a float"));
    }
}

Result<Value, Error> BigInt::castInteger() const {
    try {
        return Value(static_cast<Integer>(std::stoll(std::string(_value))));
    } catch (...) {
        return Fail(Error("value cannot be represented as an integer"));
    }
}

SIF_NAMESPACE_END
