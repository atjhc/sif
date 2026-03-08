#pragma once

#include "extern/bigint.h"
#include "runtime/objects/BigInt.h"

#include <sif/runtime/Value.h>

#include <limits>

SIF_NAMESPACE_BEGIN

inline ::BigInt::bigint ToBigInt(const Value &v) {
    if (v.isInteger()) {
        return ::BigInt::bigint(v.asInteger());
    }
    if (auto big = v.as<BigInt>()) {
        return big->value();
    }
    throw std::runtime_error("expected numeric value");
}

inline Value FromBigInt(const ::BigInt::bigint &result) {
    static const ::BigInt::bigint maxInt(std::numeric_limits<long long>::max());
    static const ::BigInt::bigint minInt(std::numeric_limits<long long>::min());
    if (result >= minInt && result <= maxInt) {
        return Value(static_cast<Integer>(std::stoll(std::string(result))));
    }
    return Value(MakeStrong<BigInt>(result));
}

inline Value CheckedAdd(Integer a, Integer b) {
    Integer result;
    if (__builtin_add_overflow(a, b, &result)) {
        return FromBigInt(::BigInt::bigint(a) + ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedSubtract(Integer a, Integer b) {
    Integer result;
    if (__builtin_sub_overflow(a, b, &result)) {
        return FromBigInt(::BigInt::bigint(a) - ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedMultiply(Integer a, Integer b) {
    Integer result;
    if (__builtin_mul_overflow(a, b, &result)) {
        return FromBigInt(::BigInt::bigint(a) * ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedNegate(Integer a) {
    if (a == std::numeric_limits<Integer>::min()) {
        return FromBigInt(-::BigInt::bigint(a));
    }
    return Value(-a);
}

inline Value BigIntAdd(const Value &lhs, const Value &rhs) {
    return FromBigInt(ToBigInt(lhs) + ToBigInt(rhs));
}

inline Value BigIntSubtract(const Value &lhs, const Value &rhs) {
    return FromBigInt(ToBigInt(lhs) - ToBigInt(rhs));
}

inline Value BigIntMultiply(const Value &lhs, const Value &rhs) {
    return FromBigInt(ToBigInt(lhs) * ToBigInt(rhs));
}

inline Value BigIntDivide(const Value &lhs, const Value &rhs) {
    return FromBigInt(ToBigInt(lhs) / ToBigInt(rhs));
}

inline Value BigIntModulo(const Value &lhs, const Value &rhs) {
    return FromBigInt(ToBigInt(lhs) % ToBigInt(rhs));
}

inline int BigIntCompare(const Value &lhs, const Value &rhs) {
    auto a = ToBigInt(lhs);
    auto b = ToBigInt(rhs);
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

inline Float BigIntCastFloat(const Value &v) {
    if (v.isFloat()) return v.asFloat();
    if (v.isInteger()) return static_cast<Float>(v.asInteger());
    if (auto big = v.as<BigInt>()) return std::stod(std::string(big->value()));
    throw std::runtime_error("can't convert to float");
}

SIF_NAMESPACE_END
