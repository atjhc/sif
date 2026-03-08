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

#include <cassert>

#include "extern/bigint.h"

#include <limits>

SIF_NAMESPACE_BEGIN

class BigInt : public Object, public NumberCastable {
  public:
    BigInt(const ::BigInt::bigint &value);
    BigInt(const std::string &value);
    BigInt(const Value &value);

    const ::BigInt::bigint &bigint() const;

    static inline Value toValue(const ::BigInt::bigint &result) {
        static const ::BigInt::bigint maxInt(std::numeric_limits<long long>::max());
        static const ::BigInt::bigint minInt(std::numeric_limits<long long>::min());
        if (result >= minInt && result <= maxInt) {
            return Value(static_cast<Integer>(static_cast<long long>(result)));
        }
        return Value(MakeStrong<BigInt>(result));
    }

    static double toDouble(const ::BigInt::bigint &value);

    static inline Value add(const Value &lhs, const Value &rhs) {
        if (lhs.isInteger() && rhs.isInteger()) {
            Integer result;
            if (!__builtin_add_overflow(lhs.asInteger(), rhs.asInteger(), &result)) {
                return Value(result);
            }
        }
        return toValue(_toBigInt(lhs) + _toBigInt(rhs));
    }

    static inline Value subtract(const Value &lhs, const Value &rhs) {
        if (lhs.isInteger() && rhs.isInteger()) {
            Integer result;
            if (!__builtin_sub_overflow(lhs.asInteger(), rhs.asInteger(), &result)) {
                return Value(result);
            }
        }
        return toValue(_toBigInt(lhs) - _toBigInt(rhs));
    }

    static inline Value multiply(const Value &lhs, const Value &rhs) {
        if (lhs.isInteger() && rhs.isInteger()) {
            Integer result;
            if (!__builtin_mul_overflow(lhs.asInteger(), rhs.asInteger(), &result)) {
                return Value(result);
            }
        }
        return toValue(_toBigInt(lhs) * _toBigInt(rhs));
    }

    static inline Value divide(const Value &lhs, const Value &rhs) {
        return toValue(_toBigInt(lhs) / _toBigInt(rhs));
    }

    static inline Value modulo(const Value &lhs, const Value &rhs) {
        return toValue(_toBigInt(lhs) % _toBigInt(rhs));
    }

    static inline Value negate(const Value &v) {
        if (v.isInteger()) {
            auto a = v.asInteger();
            if (a != std::numeric_limits<Integer>::min()) {
                return Value(-a);
            }
        }
        return toValue(-_toBigInt(v));
    }

    static inline Value increment(const Value &v) {
        if (v.isInteger()) {
            Integer result;
            if (!__builtin_add_overflow(v.asInteger(), Integer(1), &result)) {
                return Value(result);
            }
        }
        return toValue(_toBigInt(v) + ::BigInt::bigint(1));
    }

    static inline int compare(const Value &lhs, const Value &rhs) {
        auto a = _toBigInt(lhs);
        auto b = _toBigInt(rhs);
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
    // Returns by value: constructs a new bigint from Integer, or copies from
    // BigInt. Only called on the overflow/BigInt path, never the integer fast path.
    static inline ::BigInt::bigint _toBigInt(const Value &v) {
        if (v.isInteger()) return ::BigInt::bigint(v.asInteger());
        auto big = v.as<BigInt>();
        assert(big && "expected integer or BigInt value");
        return big->_value;
    }

    ::BigInt::bigint _value;
};

SIF_NAMESPACE_END
