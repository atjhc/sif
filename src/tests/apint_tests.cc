#include "tests/TestSuite.h"

#include <sif/runtime/APInt.h>

#include <limits>
#include <string>

using sif::APInt;

// ============================================================
// Constructors
// ============================================================

TEST_CASE(APInt, DefaultConstructor) {
    APInt a;
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(APInt, ConstructFromPositiveLongLong) {
    APInt a(42LL);
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(APInt, ConstructFromNegativeLongLong) {
    APInt a(-42LL);
    ASSERT_EQ(a.to_string(), "-42");
}

TEST_CASE(APInt, ConstructFromZeroLongLong) {
    APInt a(0LL);
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(APInt, ConstructFromINT64_MAX) {
    APInt a(std::numeric_limits<long long>::max());
    ASSERT_EQ(a.to_string(), "9223372036854775807");
}

TEST_CASE(APInt, ConstructFromINT64_MIN) {
    APInt a(std::numeric_limits<long long>::min());
    ASSERT_EQ(a.to_string(), "-9223372036854775808");
}

TEST_CASE(APInt, ConstructFromString) {
    APInt a("123456789012345678901234567890");
    ASSERT_EQ(a.to_string(), "123456789012345678901234567890");
}

TEST_CASE(APInt, ConstructFromNegativeString) {
    APInt a("-99999999999999999999");
    ASSERT_EQ(a.to_string(), "-99999999999999999999");
}

TEST_CASE(APInt, ConstructFromStringWithLeadingPlus) {
    APInt a("+42");
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(APInt, ConstructFromStringWithLeadingZeros) {
    APInt a("000042");
    ASSERT_EQ(a.to_string(), "42");
}

TEST_CASE(APInt, ConstructFromStringZero) {
    APInt a("0");
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(APInt, ConstructFromStringNegativeZero) {
    APInt a("-0");
    ASSERT_EQ(a.to_string(), "0");
}

TEST_CASE(APInt, ConstructFromEmptyStringThrows) {
    ASSERT_THROWS(APInt(""));
}

TEST_CASE(APInt, ConstructFromInvalidStringThrows) {
    ASSERT_THROWS(APInt("abc"));
}

TEST_CASE(APInt, ConstructFromPartiallyInvalidStringThrows) {
    ASSERT_THROWS(APInt("123abc"));
}

TEST_CASE(APInt, ConstructFromSignOnlyThrows) {
    ASSERT_THROWS(APInt("-"));
    ASSERT_THROWS(APInt("+"));
}

// ============================================================
// Addition
// ============================================================

TEST_CASE(APInt, AddPositives) {
    ASSERT_EQ((APInt(100) + APInt(200)).to_string(), "300");
}

TEST_CASE(APInt, AddNegatives) {
    ASSERT_EQ((APInt(-100) + APInt(-200)).to_string(), "-300");
}

TEST_CASE(APInt, AddMixedSigns) {
    ASSERT_EQ((APInt(100) + APInt(-30)).to_string(), "70");
    ASSERT_EQ((APInt(-100) + APInt(30)).to_string(), "-70");
}

TEST_CASE(APInt, AddMixedSignsCancels) {
    ASSERT_EQ((APInt(100) + APInt(-100)).to_string(), "0");
}

TEST_CASE(APInt, AddWithZero) {
    ASSERT_EQ((APInt(42) + APInt(0)).to_string(), "42");
    ASSERT_EQ((APInt(0) + APInt(42)).to_string(), "42");
}

TEST_CASE(APInt, AddLargeNumbers) {
    APInt a("99999999999999999999999999999");
    APInt b("1");
    ASSERT_EQ((a + b).to_string(), "100000000000000000000000000000");
}

// ============================================================
// Subtraction
// ============================================================

TEST_CASE(APInt, SubtractBasic) {
    ASSERT_EQ((APInt(100) - APInt(30)).to_string(), "70");
}

TEST_CASE(APInt, SubtractResultsInNegative) {
    ASSERT_EQ((APInt(30) - APInt(100)).to_string(), "-70");
}

TEST_CASE(APInt, SubtractNegatives) {
    ASSERT_EQ((APInt(-100) - APInt(-30)).to_string(), "-70");
    ASSERT_EQ((APInt(-30) - APInt(-100)).to_string(), "70");
}

TEST_CASE(APInt, SubtractFromZero) {
    ASSERT_EQ((APInt(0) - APInt(42)).to_string(), "-42");
}

TEST_CASE(APInt, SubtractToZero) {
    ASSERT_EQ((APInt(42) - APInt(42)).to_string(), "0");
}

// ============================================================
// Multiplication
// ============================================================

TEST_CASE(APInt, MultiplyPositives) {
    ASSERT_EQ((APInt(123) * APInt(456)).to_string(), "56088");
}

TEST_CASE(APInt, MultiplyNegatives) {
    ASSERT_EQ((APInt(-6) * APInt(-7)).to_string(), "42");
}

TEST_CASE(APInt, MultiplyMixedSigns) {
    ASSERT_EQ((APInt(-6) * APInt(7)).to_string(), "-42");
    ASSERT_EQ((APInt(6) * APInt(-7)).to_string(), "-42");
}

TEST_CASE(APInt, MultiplyByZero) {
    ASSERT_EQ((APInt(999999) * APInt(0)).to_string(), "0");
    ASSERT_EQ((APInt(0) * APInt(999999)).to_string(), "0");
}

TEST_CASE(APInt, MultiplyByOne) {
    ASSERT_EQ((APInt(42) * APInt(1)).to_string(), "42");
    ASSERT_EQ((APInt(42) * APInt(-1)).to_string(), "-42");
}

TEST_CASE(APInt, MultiplyLargeNumbers) {
    APInt a("999999999999999999");
    APInt b("999999999999999999");
    ASSERT_EQ((a * b).to_string(), "999999999999999998000000000000000001");
}

// ============================================================
// Division
// ============================================================

TEST_CASE(APInt, DivideBasic) {
    ASSERT_EQ((APInt(100) / APInt(3)).to_string(), "33");
}

TEST_CASE(APInt, DivideExact) {
    ASSERT_EQ((APInt(42) / APInt(6)).to_string(), "7");
}

TEST_CASE(APInt, DivideNegatives) {
    ASSERT_EQ((APInt(-42) / APInt(-6)).to_string(), "7");
}

TEST_CASE(APInt, DivideMixedSigns) {
    ASSERT_EQ((APInt(-42) / APInt(6)).to_string(), "-7");
    ASSERT_EQ((APInt(42) / APInt(-6)).to_string(), "-7");
}

TEST_CASE(APInt, DivideSmallerByLarger) {
    ASSERT_EQ((APInt(3) / APInt(100)).to_string(), "0");
}

TEST_CASE(APInt, DivideByOne) {
    ASSERT_EQ((APInt(42) / APInt(1)).to_string(), "42");
}

TEST_CASE(APInt, DivideBySelf) {
    APInt a("99999999999999999999");
    ASSERT_EQ((a / a).to_string(), "1");
}

TEST_CASE(APInt, DivideSimilarMagnitudes) {
    APInt a("9223372036854775907");
    APInt b("9223372036854775808");
    // a > b, so a/b = 1 with remainder 99, and b/a = 0
    ASSERT_EQ((a / b).to_string(), "1");
    ASSERT_EQ((b / a).to_string(), "0");
}

TEST_CASE(APInt, DivideByZeroThrows) {
    ASSERT_THROWS(APInt(42) / APInt(0));
}

TEST_CASE(APInt, DivideLargeBySmall) {
    APInt a("1000000000000000000000");
    ASSERT_EQ((a / APInt(7)).to_string(), "142857142857142857142");
}

TEST_CASE(APInt, DivideLargeNumbers) {
    APInt a("999999999999999998000000000000000001");
    APInt b("999999999999999999");
    ASSERT_EQ((a / b).to_string(), "999999999999999999");
}

// ============================================================
// Modulo
// ============================================================

TEST_CASE(APInt, ModuloBasic) {
    ASSERT_EQ((APInt(10) % APInt(3)).to_string(), "1");
}

TEST_CASE(APInt, ModuloExact) {
    ASSERT_EQ((APInt(42) % APInt(6)).to_string(), "0");
}

TEST_CASE(APInt, ModuloNegativeDividend) {
    ASSERT_EQ((APInt(-10) % APInt(3)).to_string(), "-1");
}

TEST_CASE(APInt, ModuloNegativeDivisor) {
    ASSERT_EQ((APInt(10) % APInt(-3)).to_string(), "1");
}

TEST_CASE(APInt, ModuloBothNegative) {
    ASSERT_EQ((APInt(-10) % APInt(-3)).to_string(), "-1");
}

TEST_CASE(APInt, ModuloSmallerByLarger) {
    ASSERT_EQ((APInt(3) % APInt(100)).to_string(), "3");
}

TEST_CASE(APInt, ModuloByZeroThrows) {
    ASSERT_THROWS(APInt(42) % APInt(0));
}

TEST_CASE(APInt, ModuloSimilarMagnitudes) {
    APInt a("9223372036854775907");
    APInt b("9223372036854775808");
    ASSERT_EQ((a % b).to_string(), "99");
}

// ============================================================
// Unary negate
// ============================================================

TEST_CASE(APInt, NegatePositive) {
    ASSERT_EQ((-APInt(42)).to_string(), "-42");
}

TEST_CASE(APInt, NegateNegative) {
    ASSERT_EQ((-APInt(-42)).to_string(), "42");
}

TEST_CASE(APInt, NegateZero) {
    ASSERT_EQ((-APInt(0)).to_string(), "0");
}

TEST_CASE(APInt, NegateINT64_MIN) {
    APInt a(std::numeric_limits<long long>::min());
    ASSERT_EQ((-a).to_string(), "9223372036854775808");
}

// ============================================================
// Comparisons
// ============================================================

TEST_CASE(APInt, CompareEqual) {
    ASSERT_TRUE(APInt(42) == APInt(42));
    ASSERT_FALSE(APInt(42) != APInt(42));
}

TEST_CASE(APInt, CompareNotEqual) {
    ASSERT_TRUE(APInt(42) != APInt(43));
    ASSERT_FALSE(APInt(42) == APInt(43));
}

TEST_CASE(APInt, CompareLessThan) {
    ASSERT_TRUE(APInt(41) < APInt(42));
    ASSERT_FALSE(APInt(42) < APInt(42));
    ASSERT_FALSE(APInt(43) < APInt(42));
}

TEST_CASE(APInt, CompareGreaterThan) {
    ASSERT_TRUE(APInt(43) > APInt(42));
    ASSERT_FALSE(APInt(42) > APInt(42));
    ASSERT_FALSE(APInt(41) > APInt(42));
}

TEST_CASE(APInt, CompareLessOrEqual) {
    ASSERT_TRUE(APInt(41) <= APInt(42));
    ASSERT_TRUE(APInt(42) <= APInt(42));
    ASSERT_FALSE(APInt(43) <= APInt(42));
}

TEST_CASE(APInt, CompareGreaterOrEqual) {
    ASSERT_TRUE(APInt(43) >= APInt(42));
    ASSERT_TRUE(APInt(42) >= APInt(42));
    ASSERT_FALSE(APInt(41) >= APInt(42));
}

TEST_CASE(APInt, CompareNegatives) {
    ASSERT_TRUE(APInt(-100) < APInt(-42));
    ASSERT_TRUE(APInt(-42) > APInt(-100));
    ASSERT_TRUE(APInt(-42) == APInt(-42));
}

TEST_CASE(APInt, CompareNegativeVsPositive) {
    ASSERT_TRUE(APInt(-1) < APInt(1));
    ASSERT_TRUE(APInt(1) > APInt(-1));
    ASSERT_FALSE(APInt(-1) > APInt(1));
}

TEST_CASE(APInt, CompareDifferentMagnitudes) {
    ASSERT_TRUE(APInt("99999999999999999999") > APInt("9999999999999999999"));
    ASSERT_TRUE(APInt("-99999999999999999999") < APInt("-9999999999999999999"));
}

// ============================================================
// Conversion
// ============================================================

TEST_CASE(APInt, ToLongLong) {
    ASSERT_EQ(APInt(42).to_long_long(), 42LL);
    ASSERT_EQ(APInt(-42).to_long_long(), -42LL);
    ASSERT_EQ(APInt(0).to_long_long(), 0LL);
}

TEST_CASE(APInt, ToLongLongBounds) {
    ASSERT_EQ(APInt(std::numeric_limits<long long>::max()).to_long_long(),
              std::numeric_limits<long long>::max());
    ASSERT_EQ(APInt(std::numeric_limits<long long>::min()).to_long_long(),
              std::numeric_limits<long long>::min());
}

TEST_CASE(APInt, ToLongLongOverflowThrows) {
    ASSERT_THROWS(APInt("99999999999999999999").to_long_long());
}

TEST_CASE(APInt, ToStringRoundTrip) {
    std::string large = "123456789012345678901234567890";
    ASSERT_EQ(APInt(large).to_string(), large);

    std::string negative = "-987654321098765432109876543210";
    ASSERT_EQ(APInt(negative).to_string(), negative);
}

// ============================================================
// Zero normalization
// ============================================================

TEST_CASE(APInt, ZeroIsNeverNegative) {
    APInt a(0);
    APInt b = -a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_FALSE(b < APInt(0));
    ASSERT_FALSE(APInt(0) < b);
}

TEST_CASE(APInt, SubtractToZeroIsNotNegative) {
    APInt a(42);
    APInt b = a - a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_TRUE(b == APInt(0));
}

TEST_CASE(APInt, NegativeSubtractToZeroIsNotNegative) {
    APInt a(-42);
    APInt b = a - a;
    ASSERT_EQ(b.to_string(), "0");
    ASSERT_TRUE(b == APInt(0));
}
