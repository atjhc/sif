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

#include <cassert>
#include <cmath>
#include <limits>

SIF_NAMESPACE_BEGIN

static ::APInt toAPInt(const Value &v) {
    if (v.isInteger())
        return ::APInt(v.asInteger());
    auto big = v.as<BigInt>();
    assert(big && "expected integer or BigInt value");
    return big->value();
}

BigInt::BigInt(const ::APInt &value) : _value(value) {}

BigInt::BigInt(const std::string &value) : _value(value) {}

BigInt::BigInt(const Value &v) {
    if (v.isInteger()) {
        _value = ::APInt(v.asInteger());
    } else if (auto big = v.as<BigInt>()) {
        _value = big->_value;
    } else {
        throw std::runtime_error("expected numeric value");
    }
}

std::string BigInt::typeName() const { return "integer"; }

std::string BigInt::description() const { return _value.to_string(); }

std::string BigInt::toString() const { return _value.to_string(); }

bool BigInt::equals(Strong<Object> object) const {
    if (const auto &other = Cast<BigInt>(object)) {
        return _value == other->_value;
    }
    return false;
}

size_t BigInt::hash() const { return std::hash<std::string>{}(_value.to_string()); }

const ::APInt &BigInt::value() const { return _value; }

Value BigInt::toValue(const ::APInt &result) {
    static const ::APInt maxInt(std::numeric_limits<long long>::max());
    static const ::APInt minInt(std::numeric_limits<long long>::min());
    if (result >= minInt && result <= maxInt) {
        return Value(static_cast<Integer>(result.to_long_long()));
    }
    return Value(MakeStrong<BigInt>(result));
}

double BigInt::toDouble(const ::APInt &val) {
    static const ::APInt maxLL(std::numeric_limits<long long>::max());
    static const ::APInt minLL(std::numeric_limits<long long>::min());
    if (val >= minLL && val <= maxLL) {
        return static_cast<double>(val.to_long_long());
    }
    auto str = val.to_string();
    bool negative = str[0] == '-';
    size_t start = negative ? 1 : 0;
    size_t numDigits = str.size() - start;
    // Parse leading significant digits and scale by remaining magnitude
    double mantissa = std::stod(str.substr(start, 18));
    double result = mantissa * std::pow(10.0, static_cast<double>(numDigits - 18));
    return negative ? -result : result;
}

Float BigInt::toFloat(const Value &v) {
    if (v.isFloat())
        return v.asFloat();
    if (v.isInteger())
        return static_cast<Float>(v.asInteger());
    if (auto big = v.as<BigInt>())
        return toDouble(big->_value);
    throw std::runtime_error("can't convert to float");
}

Result<Value, Error> BigInt::castFloat() const {
    try {
        return Value(toDouble(_value));
    } catch (...) {
        return Fail(Error("value cannot be represented as a float"));
    }
}

Result<Value, Error> BigInt::castInteger() const {
    static const ::APInt maxInt(std::numeric_limits<long long>::max());
    static const ::APInt minInt(std::numeric_limits<long long>::min());
    if (_value < minInt || _value > maxInt) {
        return Fail(Error("value cannot be represented as an integer"));
    }
    return Value(static_cast<Integer>(_value.to_long_long()));
}

Value BigInt::add(const Value &lhs, const Value &rhs) {
    return toValue(toAPInt(lhs) + toAPInt(rhs));
}

Value BigInt::subtract(const Value &lhs, const Value &rhs) {
    return toValue(toAPInt(lhs) - toAPInt(rhs));
}

Value BigInt::multiply(const Value &lhs, const Value &rhs) {
    return toValue(toAPInt(lhs) * toAPInt(rhs));
}

Value BigInt::divide(const Value &lhs, const Value &rhs) {
    return toValue(toAPInt(lhs) / toAPInt(rhs));
}

Value BigInt::modulo(const Value &lhs, const Value &rhs) {
    return toValue(toAPInt(lhs) % toAPInt(rhs));
}

Value BigInt::negate(const Value &v) { return toValue(-toAPInt(v)); }

int BigInt::compare(const Value &lhs, const Value &rhs) {
    auto a = toAPInt(lhs);
    auto b = toAPInt(rhs);
    if (a < b)
        return -1;
    if (a > b)
        return 1;
    return 0;
}

SIF_NAMESPACE_END
