#include "tests/TestSuite.h"

#include <sif/runtime/ArbitraryInt.h>

#include <limits>
#include <string>

// ============================================================
// Constructors
// ============================================================

TEST_CASE(ArbitraryInt, DefaultConstructor) {
    ArbitraryInt a;
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(ArbitraryInt, ConstructFromPositiveLongLong) {
    ArbitraryInt a(42LL);
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(ArbitraryInt, ConstructFromNegativeLongLong) {
    ArbitraryInt a(-42LL);
    ASSERT_EQ(a.to_string(), "-42");
}

TEST_CASE(ArbitraryInt, ConstructFromZeroLongLong) {
    ArbitraryInt a(0LL);
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(ArbitraryInt, ConstructFromINT64_MAX) {
    ArbitraryInt a(std::numeric_limits<long long>::max());
    ASSERT_EQ(a.to_string(), "9223372036854775807");
}

TEST_CASE(ArbitraryInt, ConstructFromINT64_MIN) {
    ArbitraryInt a(std::numeric_limits<long long>::min());
    ASSERT_EQ(a.to_string(), "-9223372036854775808");
}

TEST_CASE(ArbitraryInt, ConstructFromString) {
    ArbitraryInt a("123456789012345678901234567890");
    ASSERT_EQ(a.to_string(), "123456789012345678901234567890");
}

TEST_CASE(ArbitraryInt, ConstructFromNegativeString) {
    ArbitraryInt a("-99999999999999999999");
    ASSERT_EQ(a.to_string(), "-99999999999999999999");
}

TEST_CASE(ArbitraryInt, ConstructFromStringWithLeadingPlus) {
    ArbitraryInt a("+42");
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(ArbitraryInt, ConstructFromStringWithLeadingZeros) {
    ArbitraryInt a("000042");
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(ArbitraryInt, ConstructFromStringZero) {
    ArbitraryInt a("0");
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(ArbitraryInt, ConstructFromStringNegativeZero) {
    ArbitraryInt a("-0");
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(ArbitraryInt, ConstructFromEmptyStringThrows) {
    ASSERT_THROWS(ArbitraryInt(""));
}

TEST_CASE(ArbitraryInt, ConstructFromInvalidStringThrows) {
    ASSERT_THROWS(ArbitraryInt("abc"));
}

TEST_CASE(ArbitraryInt, ConstructFromPartiallyInvalidStringThrows) {
    ASSERT_THROWS(ArbitraryInt("123abc"));
}

TEST_CASE(ArbitraryInt, ConstructFromSignOnlyThrows) {
    ASSERT_THROWS(ArbitraryInt("-"));
    ASSERT_THROWS(ArbitraryInt("+"));
}

// ============================================================
// Addition
// ============================================================

TEST_CASE(ArbitraryInt, AddPositives) {
    ASSERT_EQ((ArbitraryInt(100) + ArbitraryInt(200)).to_string(), "300");
}

TEST_CASE(ArbitraryInt, AddNegatives) {
    ASSERT_EQ((ArbitraryInt(-100) + ArbitraryInt(-200)).to_string(), "-300");
}

TEST_CASE(ArbitraryInt, AddMixedSigns) {
    ASSERT_EQ((ArbitraryInt(100) + ArbitraryInt(-30)).to_string(), "70");
    ASSERT_EQ((ArbitraryInt(-100) + ArbitraryInt(30)).to_string(), "-70");
}

TEST_CASE(ArbitraryInt, AddMixedSignsCancels) {
    ASSERT_EQ((ArbitraryInt(100) + ArbitraryInt(-100)).to_string(), "0");
}

TEST_CASE(ArbitraryInt, AddWithZero) {
    ASSERT_EQ((ArbitraryInt(42) + ArbitraryInt(0)).to_string(), "42");
    ASSERT_EQ((ArbitraryInt(0) + ArbitraryInt(42)).to_string(), "42");
}

TEST_CASE(ArbitraryInt, AddLargeNumbers) {
    ArbitraryInt a("99999999999999999999999999999");
    ArbitraryInt b("1");
    ASSERT_EQ((a + b).to_string(), "100000000000000000000000000000");
}

// ============================================================
// Subtraction
// ============================================================

TEST_CASE(ArbitraryInt, SubtractBasic) {
    ASSERT_EQ((ArbitraryInt(100) - ArbitraryInt(30)).to_string(), "70");
}

TEST_CASE(ArbitraryInt, SubtractResultsInNegative) {
    ASSERT_EQ((ArbitraryInt(30) - ArbitraryInt(100)).to_string(), "-70");
}

TEST_CASE(ArbitraryInt, SubtractNegatives) {
    ASSERT_EQ((ArbitraryInt(-100) - ArbitraryInt(-30)).to_string(), "-70");
    ASSERT_EQ((ArbitraryInt(-30) - ArbitraryInt(-100)).to_string(), "70");
}

TEST_CASE(ArbitraryInt, SubtractFromZero) {
    ASSERT_EQ((ArbitraryInt(0) - ArbitraryInt(42)).to_string(), "-42");
}

TEST_CASE(ArbitraryInt, SubtractToZero) {
    ASSERT_EQ((ArbitraryInt(42) - ArbitraryInt(42)).to_string(), "0");
}

// ============================================================
// Multiplication
// ============================================================

TEST_CASE(ArbitraryInt, MultiplyPositives) {
    ASSERT_EQ((ArbitraryInt(123) * ArbitraryInt(456)).to_string(), "56088");
}

TEST_CASE(ArbitraryInt, MultiplyNegatives) {
    ASSERT_EQ((ArbitraryInt(-6) * ArbitraryInt(-7)).to_string(), "42");
}

TEST_CASE(ArbitraryInt, MultiplyMixedSigns) {
    ASSERT_EQ((ArbitraryInt(-6) * ArbitraryInt(7)).to_string(), "-42");
    ASSERT_EQ((ArbitraryInt(6) * ArbitraryInt(-7)).to_string(), "-42");
}

TEST_CASE(ArbitraryInt, MultiplyByZero) {
    ASSERT_EQ((ArbitraryInt(999999) * ArbitraryInt(0)).to_string(), "0");
    ASSERT_EQ((ArbitraryInt(0) * ArbitraryInt(999999)).to_string(), "0");
}

TEST_CASE(ArbitraryInt, MultiplyByOne) {
    ASSERT_EQ((ArbitraryInt(42) * ArbitraryInt(1)).to_string(), "42");
    ASSERT_EQ((ArbitraryInt(42) * ArbitraryInt(-1)).to_string(), "-42");
}

TEST_CASE(ArbitraryInt, MultiplyLargeNumbers) {
    ArbitraryInt a("999999999999999999");
    ArbitraryInt b("999999999999999999");
    ASSERT_EQ((a * b).to_string(), "999999999999999998000000000000000001");
}

// ============================================================
// Division
// ============================================================

TEST_CASE(ArbitraryInt, DivideBasic) {
    ASSERT_EQ((ArbitraryInt(100) / ArbitraryInt(3)).to_string(), "33");
}

TEST_CASE(ArbitraryInt, DivideExact) {
    ASSERT_EQ((ArbitraryInt(42) / ArbitraryInt(6)).to_string(), "7");
}

TEST_CASE(ArbitraryInt, DivideNegatives) {
    ASSERT_EQ((ArbitraryInt(-42) / ArbitraryInt(-6)).to_string(), "7");
}

TEST_CASE(ArbitraryInt, DivideMixedSigns) {
    ASSERT_EQ((ArbitraryInt(-42) / ArbitraryInt(6)).to_string(), "-7");
    ASSERT_EQ((ArbitraryInt(42) / ArbitraryInt(-6)).to_string(), "-7");
}

TEST_CASE(ArbitraryInt, DivideSmallerByLarger) {
    ASSERT_EQ((ArbitraryInt(3) / ArbitraryInt(100)).to_string(), "0");
}

TEST_CASE(ArbitraryInt, DivideByOne) {
    ASSERT_EQ((ArbitraryInt(42) / ArbitraryInt(1)).to_string(), "42");
}

TEST_CASE(ArbitraryInt, DivideBySelf) {
    ArbitraryInt a("99999999999999999999");
    ASSERT_EQ((a / a).to_string(), "1");
}

TEST_CASE(ArbitraryInt, DivideSimilarMagnitudes) {
    ArbitraryInt a("9223372036854775907");
    ArbitraryInt b("9223372036854775808");
    // a > b, so a/b = 1 with remainder 99, and b/a = 0
    ASSERT_EQ((a / b).to_string(), "1");
    ASSERT_EQ((b / a).to_string(), "0");
}

TEST_CASE(ArbitraryInt, DivideByZeroThrows) {
    ASSERT_THROWS(ArbitraryInt(42) / ArbitraryInt(0));
}

TEST_CASE(ArbitraryInt, DivideLargeBySmall) {
    ArbitraryInt a("1000000000000000000000");
    ASSERT_EQ((a / ArbitraryInt(7)).to_string(), "142857142857142857142");
}

TEST_CASE(ArbitraryInt, DivideLargeNumbers) {
    ArbitraryInt a("999999999999999998000000000000000001");
    ArbitraryInt b("999999999999999999");
    ASSERT_EQ((a / b).to_string(), "999999999999999999");
}

// ============================================================
// Modulo
// ============================================================

TEST_CASE(ArbitraryInt, ModuloBasic) {
    ASSERT_EQ((ArbitraryInt(10) % ArbitraryInt(3)).to_string(), "1");
}

TEST_CASE(ArbitraryInt, ModuloExact) {
    ASSERT_EQ((ArbitraryInt(42) % ArbitraryInt(6)).to_string(), "0");
}

TEST_CASE(ArbitraryInt, ModuloNegativeDividend) {
    ASSERT_EQ((ArbitraryInt(-10) % ArbitraryInt(3)).to_string(), "-1");
}

TEST_CASE(ArbitraryInt, ModuloNegativeDivisor) {
    ASSERT_EQ((ArbitraryInt(10) % ArbitraryInt(-3)).to_string(), "1");
}

TEST_CASE(ArbitraryInt, ModuloBothNegative) {
    ASSERT_EQ((ArbitraryInt(-10) % ArbitraryInt(-3)).to_string(), "-1");
}

TEST_CASE(ArbitraryInt, ModuloSmallerByLarger) {
    ASSERT_EQ((ArbitraryInt(3) % ArbitraryInt(100)).to_string(), "3");
}

TEST_CASE(ArbitraryInt, ModuloByZeroThrows) {
    ASSERT_THROWS(ArbitraryInt(42) % ArbitraryInt(0));
}

TEST_CASE(ArbitraryInt, ModuloSimilarMagnitudes) {
    ArbitraryInt a("9223372036854775907");
    ArbitraryInt b("9223372036854775808");
    ASSERT_EQ((a % b).to_string(), "99");
}

// ============================================================
// Unary negate
// ============================================================

TEST_CASE(ArbitraryInt, NegatePositive) {
    ASSERT_EQ((-ArbitraryInt(42)).to_string(), "-42");
}

TEST_CASE(ArbitraryInt, NegateNegative) {
    ASSERT_EQ((-ArbitraryInt(-42)).to_string(), "42");
}

TEST_CASE(ArbitraryInt, NegateZero) {
    ASSERT_EQ((-ArbitraryInt(0)).to_string(), "0");
}

TEST_CASE(ArbitraryInt, NegateINT64_MIN) {
    ArbitraryInt a(std::numeric_limits<long long>::min());
    ASSERT_EQ((-a).to_string(), "9223372036854775808");
}

// ============================================================
// Comparisons
// ============================================================

TEST_CASE(ArbitraryInt, CompareEqual) {
    ASSERT_TRUE(ArbitraryInt(42) == ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(42) != ArbitraryInt(42));
}

TEST_CASE(ArbitraryInt, CompareNotEqual) {
    ASSERT_TRUE(ArbitraryInt(42) != ArbitraryInt(43));
    ASSERT_FALSE(ArbitraryInt(42) == ArbitraryInt(43));
}

TEST_CASE(ArbitraryInt, CompareLessThan) {
    ASSERT_TRUE(ArbitraryInt(41) < ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(42) < ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(43) < ArbitraryInt(42));
}

TEST_CASE(ArbitraryInt, CompareGreaterThan) {
    ASSERT_TRUE(ArbitraryInt(43) > ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(42) > ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(41) > ArbitraryInt(42));
}

TEST_CASE(ArbitraryInt, CompareLessOrEqual) {
    ASSERT_TRUE(ArbitraryInt(41) <= ArbitraryInt(42));
    ASSERT_TRUE(ArbitraryInt(42) <= ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(43) <= ArbitraryInt(42));
}

TEST_CASE(ArbitraryInt, CompareGreaterOrEqual) {
    ASSERT_TRUE(ArbitraryInt(43) >= ArbitraryInt(42));
    ASSERT_TRUE(ArbitraryInt(42) >= ArbitraryInt(42));
    ASSERT_FALSE(ArbitraryInt(41) >= ArbitraryInt(42));
}

TEST_CASE(ArbitraryInt, CompareNegatives) {
    ASSERT_TRUE(ArbitraryInt(-100) < ArbitraryInt(-42));
    ASSERT_TRUE(ArbitraryInt(-42) > ArbitraryInt(-100));
    ASSERT_TRUE(ArbitraryInt(-42) == ArbitraryInt(-42));
}

TEST_CASE(ArbitraryInt, CompareNegativeVsPositive) {
    ASSERT_TRUE(ArbitraryInt(-1) < ArbitraryInt(1));
    ASSERT_TRUE(ArbitraryInt(1) > ArbitraryInt(-1));
    ASSERT_FALSE(ArbitraryInt(-1) > ArbitraryInt(1));
}

TEST_CASE(ArbitraryInt, CompareDifferentMagnitudes) {
    ASSERT_TRUE(ArbitraryInt("99999999999999999999") > ArbitraryInt("9999999999999999999"));
    ASSERT_TRUE(ArbitraryInt("-99999999999999999999") < ArbitraryInt("-9999999999999999999"));
}

// ============================================================
// Conversion
// ============================================================

TEST_CASE(ArbitraryInt, ToLongLong) {
    ASSERT_EQ(ArbitraryInt(42).to_long_long(), 42LL);
    ASSERT_EQ(ArbitraryInt(-42).to_long_long(), -42LL);
    ASSERT_EQ(ArbitraryInt(0).to_long_long(), 0LL);
}

TEST_CASE(ArbitraryInt, ToLongLongBounds) {
    ASSERT_EQ(ArbitraryInt(std::numeric_limits<long long>::max()).to_long_long(),
              std::numeric_limits<long long>::max());
    ASSERT_EQ(ArbitraryInt(std::numeric_limits<long long>::min()).to_long_long(),
              std::numeric_limits<long long>::min());
}

TEST_CASE(ArbitraryInt, ToLongLongOverflowThrows) {
    ASSERT_THROWS(ArbitraryInt("99999999999999999999").to_long_long());
}

TEST_CASE(ArbitraryInt, ToStringRoundTrip) {
    std::string large = "123456789012345678901234567890";
    ASSERT_EQ(ArbitraryInt(large).to_string(), large);

    std::string negative = "-987654321098765432109876543210";
    ASSERT_EQ(ArbitraryInt(negative).to_string(), negative);
}

// ============================================================
// Zero normalization
// ============================================================

TEST_CASE(ArbitraryInt, ZeroIsNeverNegative) {
    ArbitraryInt a(0);
    ArbitraryInt b = -a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_FALSE(b < ArbitraryInt(0));
    ASSERT_FALSE(ArbitraryInt(0) < b);
}

TEST_CASE(ArbitraryInt, SubtractToZeroIsNotNegative) {
    ArbitraryInt a(42);
    ArbitraryInt b = a - a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_TRUE(b == ArbitraryInt(0));
}

TEST_CASE(ArbitraryInt, NegativeSubtractToZeroIsNotNegative) {
    ArbitraryInt a(-42);
    ArbitraryInt b = a - a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_TRUE(b == ArbitraryInt(0));
}
