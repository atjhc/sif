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

#include <sif/Common.h>
#include <sif/runtime/Object.h>
#include <sif/runtime/Value.h>

#include <sif/runtime/protocols/Castable.h>

#include "extern/bigint.h"

#include <limits>

SIF_NAMESPACE_BEGIN

class BigInt : public Object, public NumberCastable {
  public:
    BigInt(const ::BigInt::bigint &value);
    BigInt(const std::string &value);
    BigInt(const Value &value);

    const ::BigInt::bigint &bigint() const;

    static Value toValue(const ::BigInt::bigint &result);
    static double toDouble(const ::BigInt::bigint &value);

    static inline Value checkedAdd(Integer a, Integer b) {
        Integer result;
        if (__builtin_add_overflow(a, b, &result)) {
            return toValue(::BigInt::bigint(a) + ::BigInt::bigint(b));
        }
        return Value(result);
    }

    static inline Value checkedSubtract(Integer a, Integer b) {
        Integer result;
        if (__builtin_sub_overflow(a, b, &result)) {
            return toValue(::BigInt::bigint(a) - ::BigInt::bigint(b));
        }
        return Value(result);
    }

    static inline Value checkedMultiply(Integer a, Integer b) {
        Integer result;
        if (__builtin_mul_overflow(a, b, &result)) {
            return toValue(::BigInt::bigint(a) * ::BigInt::bigint(b));
        }
        return Value(result);
    }

    static inline Value checkedNegate(Integer a) {
        if (a == std::numeric_limits<Integer>::min()) {
            return toValue(-::BigInt::bigint(a));
        }
        return Value(-a);
    }

    static inline Value add(const Value &lhs, const Value &rhs) {
        return toValue(BigInt(lhs)._value + BigInt(rhs)._value);
    }

    static inline Value subtract(const Value &lhs, const Value &rhs) {
        return toValue(BigInt(lhs)._value - BigInt(rhs)._value);
    }

    static inline Value multiply(const Value &lhs, const Value &rhs) {
        return toValue(BigInt(lhs)._value * BigInt(rhs)._value);
    }

    static inline Value divide(const Value &lhs, const Value &rhs) {
        return toValue(BigInt(lhs)._value / BigInt(rhs)._value);
    }

    static inline Value modulo(const Value &lhs, const Value &rhs) {
        return toValue(BigInt(lhs)._value % BigInt(rhs)._value);
    }

    static inline int compare(const Value &lhs, const Value &rhs) {
        auto a = BigInt(lhs)._value;
        auto b = BigInt(rhs)._value;
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    static inline Float toFloat(const Value &v) {
        if (v.isFloat()) return v.asFloat();
        if (v.isInteger()) return static_cast<Float>(v.asInteger());
        if (auto big = v.as<BigInt>()) return toDouble(big->_value);
        throw std::runtime_error("can't convert to float");
    }

    std::string typeName() const override;
    std::string description() const override;
    std::string toString() const override;
    bool equals(Strong<Object>) const override;
    size_t hash() const override;

    Result<Value, Error> castFloat() const override;
    Result<Value, Error> castInteger() const override;

  private:
    ::BigInt::bigint _value;
};

SIF_NAMESPACE_END
