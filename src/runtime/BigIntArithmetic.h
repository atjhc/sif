#pragma once

#include "extern/bigint.h"
#include "runtime/objects/BigInt.h"

#include <sif/runtime/Value.h>

#include <limits>

SIF_NAMESPACE_BEGIN

inline Value CheckedAdd(Integer a, Integer b) {
    Integer result;
    if (__builtin_add_overflow(a, b, &result)) {
        return BigInt::toValue(::BigInt::bigint(a) + ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedSubtract(Integer a, Integer b) {
    Integer result;
    if (__builtin_sub_overflow(a, b, &result)) {
        return BigInt::toValue(::BigInt::bigint(a) - ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedMultiply(Integer a, Integer b) {
    Integer result;
    if (__builtin_mul_overflow(a, b, &result)) {
        return BigInt::toValue(::BigInt::bigint(a) * ::BigInt::bigint(b));
    }
    return Value(result);
}

inline Value CheckedNegate(Integer a) {
    if (a == std::numeric_limits<Integer>::min()) {
        return BigInt::toValue(-::BigInt::bigint(a));
    }
    return Value(-a);
}

inline Value BigIntAdd(const Value &lhs, const Value &rhs) {
    return BigInt::toValue(BigInt(lhs).bigint() + BigInt(rhs).bigint());
}

inline Value BigIntSubtract(const Value &lhs, const Value &rhs) {
    return BigInt::toValue(BigInt(lhs).bigint() - BigInt(rhs).bigint());
}

inline Value BigIntMultiply(const Value &lhs, const Value &rhs) {
    return BigInt::toValue(BigInt(lhs).bigint() * BigInt(rhs).bigint());
}

inline Value BigIntDivide(const Value &lhs, const Value &rhs) {
    return BigInt::toValue(BigInt(lhs).bigint() / BigInt(rhs).bigint());
}

inline Value BigIntModulo(const Value &lhs, const Value &rhs) {
    return BigInt::toValue(BigInt(lhs).bigint() % BigInt(rhs).bigint());
}

inline int BigIntCompare(const Value &lhs, const Value &rhs) {
    auto a = BigInt(lhs).bigint();
    auto b = BigInt(rhs).bigint();
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

inline Float BigIntCastFloat(const Value &v) {
    if (v.isFloat()) return v.asFloat();
    if (v.isInteger()) return static_cast<Float>(v.asInteger());
    if (auto big = v.as<BigInt>()) return std::stod(std::string(big->bigint()));
    throw std::runtime_error("can't convert to float");
}

SIF_NAMESPACE_END
