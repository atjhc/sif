// Minimal arbitrary-precision integer for sif.
// Base 10^9 limbs in a vector<uint32_t>, least-significant first.

#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class BigInt {
    static constexpr uint32_t BASE = 1'000'000'000;
    static constexpr int DIGITS_PER_LIMB = 9;

    std::vector<uint32_t> _limbs;
    bool _negative = false;

    void normalize() {
        while (_limbs.size() > 1 && _limbs.back() == 0)
            _limbs.pop_back();
        if (_limbs.size() == 1 && _limbs[0] == 0)
            _negative = false;
    }

    bool isZero() const { return _limbs.size() == 1 && _limbs[0] == 0; }

    static int compareMagnitude(const BigInt &a, const BigInt &b) {
        if (a._limbs.size() != b._limbs.size())
            return a._limbs.size() < b._limbs.size() ? -1 : 1;
        for (int i = static_cast<int>(a._limbs.size()) - 1; i >= 0; i--) {
            if (a._limbs[i] != b._limbs[i])
                return a._limbs[i] < b._limbs[i] ? -1 : 1;
        }
        return 0;
    }

    static BigInt addMagnitudes(const BigInt &a, const BigInt &b) {
        BigInt result;
        size_t n = std::max(a._limbs.size(), b._limbs.size());
        result._limbs.resize(n);
        uint64_t carry = 0;
        for (size_t i = 0; i < n; i++) {
            uint64_t sum = carry;
            if (i < a._limbs.size()) sum += a._limbs[i];
            if (i < b._limbs.size()) sum += b._limbs[i];
            result._limbs[i] = static_cast<uint32_t>(sum % BASE);
            carry = sum / BASE;
        }
        if (carry)
            result._limbs.push_back(static_cast<uint32_t>(carry));
        return result;
    }

    // Caller must guarantee |a| >= |b|.
    static BigInt subtractMagnitudes(const BigInt &a, const BigInt &b) {
        BigInt result;
        result._limbs.resize(a._limbs.size());
        int64_t borrow = 0;
        for (size_t i = 0; i < a._limbs.size(); i++) {
            int64_t diff = static_cast<int64_t>(a._limbs[i]) - borrow;
            if (i < b._limbs.size()) diff -= b._limbs[i];
            if (diff < 0) {
                diff += BASE;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result._limbs[i] = static_cast<uint32_t>(diff);
        }
        result.normalize();
        return result;
    }

    // Single-limb divisor fast path.
    static std::pair<BigInt, BigInt> divmodSmall(const BigInt &num, uint32_t den) {
        BigInt quotient;
        quotient._limbs.resize(num._limbs.size());
        uint64_t rem = 0;
        for (int i = static_cast<int>(num._limbs.size()) - 1; i >= 0; i--) {
            uint64_t cur = rem * BASE + num._limbs[i];
            quotient._limbs[i] = static_cast<uint32_t>(cur / den);
            rem = cur % den;
        }
        quotient.normalize();
        BigInt remainder;
        remainder._limbs = {static_cast<uint32_t>(rem)};
        return {quotient, remainder};
    }

    // Multiply magnitude by a single limb (used by division).
    static BigInt mulSmall(const BigInt &a, uint32_t b) {
        BigInt result;
        result._limbs.resize(a._limbs.size());
        uint64_t carry = 0;
        for (size_t i = 0; i < a._limbs.size(); i++) {
            uint64_t prod = static_cast<uint64_t>(a._limbs[i]) * b + carry;
            result._limbs[i] = static_cast<uint32_t>(prod % BASE);
            carry = prod / BASE;
        }
        if (carry)
            result._limbs.push_back(static_cast<uint32_t>(carry));
        result.normalize();
        return result;
    }

    // Multi-limb long division. Both operands must be positive.
    static std::pair<BigInt, BigInt> divmodLarge(const BigInt &num, const BigInt &den) {
        int cmp = compareMagnitude(num, den);
        if (cmp < 0)
            return {BigInt(0LL), num};
        if (cmp == 0)
            return {BigInt(1LL), BigInt(0LL)};

        if (den._limbs.size() == 1)
            return divmodSmall(num, den._limbs[0]);

        // Knuth Algorithm D: normalize so leading limb of divisor >= BASE/2
        uint32_t d = BASE / (den._limbs.back() + 1);
        BigInt u = mulSmall(num, d);
        BigInt v = mulSmall(den, d);

        size_t n = v._limbs.size();
        size_t m = u._limbs.size() - n;

        // Ensure u has m+n+1 limbs
        while (u._limbs.size() <= m + n)
            u._limbs.push_back(0);

        BigInt quotient;
        quotient._limbs.resize(m + 1, 0);

        for (int j = static_cast<int>(m); j >= 0; j--) {
            // Trial quotient from top two limbs of current remainder
            uint64_t top = static_cast<uint64_t>(u._limbs[j + n]) * BASE +
                           u._limbs[j + n - 1];
            uint64_t qhat = top / v._limbs[n - 1];
            uint64_t rhat = top % v._limbs[n - 1];

            // Refine estimate
            while (qhat >= BASE ||
                   (n >= 2 &&
                    qhat * v._limbs[n - 2] >
                        rhat * BASE + u._limbs[j + n - 2])) {
                qhat--;
                rhat += v._limbs[n - 1];
                if (rhat >= BASE)
                    break;
            }

            // Subtract qhat * v from u[j..j+n]
            int64_t borrow = 0;
            for (size_t i = 0; i < n; i++) {
                int64_t prod = static_cast<int64_t>(qhat) *
                               static_cast<int64_t>(v._limbs[i]);
                int64_t diff = static_cast<int64_t>(u._limbs[j + i]) -
                               borrow - (prod % static_cast<int64_t>(BASE));
                borrow = prod / static_cast<int64_t>(BASE);
                if (diff < 0) {
                    diff += BASE;
                    borrow++;
                }
                u._limbs[j + i] = static_cast<uint32_t>(diff);
            }
            int64_t diff = static_cast<int64_t>(u._limbs[j + n]) - borrow;
            u._limbs[j + n] = static_cast<uint32_t>(diff < 0 ? diff + BASE : diff);

            quotient._limbs[j] = static_cast<uint32_t>(qhat);

            // If we subtracted too much, add back
            if (diff < 0) {
                quotient._limbs[j]--;
                uint64_t carry = 0;
                for (size_t i = 0; i < n; i++) {
                    uint64_t sum = static_cast<uint64_t>(u._limbs[j + i]) +
                                   v._limbs[i] + carry;
                    u._limbs[j + i] = static_cast<uint32_t>(sum % BASE);
                    carry = sum / BASE;
                }
                u._limbs[j + n] += static_cast<uint32_t>(carry);
            }
        }

        quotient.normalize();

        // Remainder is u / d (unnormalize)
        auto [remainder, _] = divmodSmall(u, d);
        remainder._limbs.resize(n);
        remainder.normalize();
        return {quotient, remainder};
    }

  public:
    BigInt() : _limbs{0}, _negative(false) {}

    BigInt(long long val) {
        if (val == 0) {
            _limbs = {0};
            _negative = false;
            return;
        }
        _negative = val < 0;
        // Cast to uint64_t before negating to avoid UB on INT64_MIN
        uint64_t abs_val;
        if (val == std::numeric_limits<long long>::min()) {
            abs_val = static_cast<uint64_t>(std::numeric_limits<long long>::max()) + 1;
        } else {
            abs_val = _negative ? static_cast<uint64_t>(-val)
                                : static_cast<uint64_t>(val);
        }
        while (abs_val > 0) {
            _limbs.push_back(static_cast<uint32_t>(abs_val % BASE));
            abs_val /= BASE;
        }
    }

    BigInt(const std::string &s) {
        if (s.empty())
            throw std::invalid_argument("empty string");

        size_t start = 0;
        _negative = false;
        if (s[0] == '-') {
            _negative = true;
            start = 1;
        } else if (s[0] == '+') {
            start = 1;
        }

        if (start >= s.size())
            throw std::invalid_argument("no digits in: " + s);

        // Validate digits
        for (size_t i = start; i < s.size(); i++) {
            if (s[i] < '0' || s[i] > '9')
                throw std::invalid_argument("invalid character in: " + s);
        }

        // Parse 9-digit groups from the right
        _limbs.clear();
        int pos = static_cast<int>(s.size());
        while (pos > static_cast<int>(start)) {
            int begin = std::max(pos - DIGITS_PER_LIMB, static_cast<int>(start));
            uint32_t limb = 0;
            for (int i = begin; i < pos; i++)
                limb = limb * 10 + static_cast<uint32_t>(s[i] - '0');
            _limbs.push_back(limb);
            pos = begin;
        }

        if (_limbs.empty())
            _limbs.push_back(0);

        normalize();
    }

    BigInt operator-() const {
        BigInt result = *this;
        if (!isZero())
            result._negative = !_negative;
        return result;
    }

    BigInt operator+(const BigInt &rhs) const {
        if (_negative == rhs._negative) {
            BigInt result = addMagnitudes(*this, rhs);
            result._negative = _negative;
            result.normalize();
            return result;
        }
        int cmp = compareMagnitude(*this, rhs);
        if (cmp == 0)
            return BigInt(0LL);
        if (cmp > 0) {
            BigInt result = subtractMagnitudes(*this, rhs);
            result._negative = _negative;
            return result;
        }
        BigInt result = subtractMagnitudes(rhs, *this);
        result._negative = rhs._negative;
        return result;
    }

    BigInt operator-(const BigInt &rhs) const { return *this + (-rhs); }

    BigInt operator*(const BigInt &rhs) const {
        if (isZero() || rhs.isZero())
            return BigInt(0LL);

        BigInt result;
        size_t n = _limbs.size(), m = rhs._limbs.size();
        result._limbs.resize(n + m, 0);

        for (size_t i = 0; i < n; i++) {
            uint64_t carry = 0;
            for (size_t j = 0; j < m; j++) {
                uint64_t prod = static_cast<uint64_t>(_limbs[i]) * rhs._limbs[j] +
                                result._limbs[i + j] + carry;
                result._limbs[i + j] = static_cast<uint32_t>(prod % BASE);
                carry = prod / BASE;
            }
            if (carry)
                result._limbs[i + m] += static_cast<uint32_t>(carry);
        }

        result._negative = _negative != rhs._negative;
        result.normalize();
        return result;
    }

    BigInt operator/(const BigInt &rhs) const {
        if (rhs.isZero())
            throw std::logic_error("division by zero");

        // Work with magnitudes, apply sign after
        BigInt absNum = *this;
        absNum._negative = false;
        BigInt absDen = rhs;
        absDen._negative = false;

        auto [q, r] = divmodLarge(absNum, absDen);
        q._negative = (_negative != rhs._negative) && !q.isZero();
        return q;
    }

    BigInt operator%(const BigInt &rhs) const {
        if (rhs.isZero())
            throw std::logic_error("division by zero");

        BigInt absNum = *this;
        absNum._negative = false;
        BigInt absDen = rhs;
        absDen._negative = false;

        auto [q, r] = divmodLarge(absNum, absDen);
        r._negative = _negative && !r.isZero();
        return r;
    }

    bool operator==(const BigInt &rhs) const {
        return _negative == rhs._negative && _limbs == rhs._limbs;
    }

    bool operator!=(const BigInt &rhs) const { return !(*this == rhs); }

    bool operator<(const BigInt &rhs) const {
        if (_negative != rhs._negative)
            return _negative;
        int cmp = compareMagnitude(*this, rhs);
        return _negative ? cmp > 0 : cmp < 0;
    }

    bool operator>(const BigInt &rhs) const { return rhs < *this; }
    bool operator<=(const BigInt &rhs) const { return !(rhs < *this); }
    bool operator>=(const BigInt &rhs) const { return !(*this < rhs); }

    std::string to_string() const {
        std::string result;
        if (_negative)
            result += '-';

        // Most significant limb without leading zeros
        result += std::to_string(_limbs.back());

        // Remaining limbs zero-padded to 9 digits
        for (int i = static_cast<int>(_limbs.size()) - 2; i >= 0; i--) {
            std::string limb = std::to_string(_limbs[i]);
            result.append(DIGITS_PER_LIMB - limb.size(), '0');
            result += limb;
        }
        return result;
    }

    long long to_long_long() const { return std::stoll(to_string()); }
};
