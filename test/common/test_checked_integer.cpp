#include "catch.hpp"
#include "duckdb/common/checked_integer.hpp"

using namespace duckdb;

using i8_t = CheckedInteger<int8_t>;
using i16_t = CheckedInteger<int16_t>;
using i32_t = CheckedInteger<int32_t>;
using i64_t = CheckedInteger<int64_t>;
using u8_t = CheckedInteger<uint8_t>;
using u16_t = CheckedInteger<uint16_t>;
using u32_t = CheckedInteger<uint32_t>;
using u64_t = CheckedInteger<uint64_t>;

TEST_CASE("CheckedInteger construction", "[checked_integer]") {
	// default construction
	u8_t a;
	REQUIRE(a.GetValue() == 0);
	i32_t b;
	REQUIRE(b.GetValue() == 0);

	// same-type construction
	u8_t c(uint8_t(200));
	REQUIRE(c.GetValue() == 200);
	i16_t d(int16_t(-500));
	REQUIRE(d.GetValue() == -500);

	// cross-type construction (valid)
	u8_t e(42);   // int -> uint8_t
	REQUIRE(e.GetValue() == 42);
	u8_t f(255);  // int -> uint8_t max
	REQUIRE(f.GetValue() == 255);
	u8_t g(0);
	REQUIRE(g.GetValue() == 0);

	// cross-type overflow throws
	REQUIRE_THROWS_AS(u8_t(256), InternalException);
	REQUIRE_THROWS_AS(i8_t(128), InternalException);
	REQUIRE_THROWS_AS(i8_t(-129), InternalException);
	REQUIRE_THROWS_AS(u16_t(100000), InternalException);

	// negative to unsigned throws
	REQUIRE_THROWS_AS(u8_t(-1), InternalException);
	REQUIRE_THROWS_AS(u16_t(-100), InternalException);
	REQUIRE_THROWS_AS(u32_t(-1), InternalException);
	REQUIRE_THROWS_AS(u64_t(int8_t(-1)), InternalException);

	// explicit conversion to T
	u8_t h(42);
	REQUIRE(static_cast<uint8_t>(h) == 42);

	// explicit operator bool
	i32_t zero(0);
	i32_t nonzero(42);
	REQUIRE_FALSE(static_cast<bool>(zero));
	REQUIRE(static_cast<bool>(nonzero));
}

TEST_CASE("CheckedInteger unsigned arithmetic", "[checked_integer]") {
	// addition
	REQUIRE((u8_t(100) + u8_t(55)).GetValue() == 155);
	REQUIRE((u8_t(0) + u8_t(255)).GetValue() == 255);
	REQUIRE_THROWS_AS(u8_t(200) + u8_t(100), InternalException);
	REQUIRE_THROWS_AS(u8_t(255) + u8_t(1), InternalException);

	// subtraction
	REQUIRE((u8_t(100) - u8_t(50)).GetValue() == 50);
	REQUIRE((u8_t(255) - u8_t(255)).GetValue() == 0);
	REQUIRE_THROWS_AS(u8_t(0) - u8_t(1), InternalException);
	REQUIRE_THROWS_AS(u8_t(50) - u8_t(100), InternalException);

	// multiplication
	REQUIRE((u8_t(10) * u8_t(25)).GetValue() == 250);
	REQUIRE((u8_t(0) * u8_t(255)).GetValue() == 0);
	REQUIRE_THROWS_AS(u8_t(16) * u8_t(16), InternalException);
	REQUIRE_THROWS_AS(u8_t(200) * u8_t(2), InternalException);

	// division
	REQUIRE((u8_t(100) / u8_t(10)).GetValue() == 10);
	REQUIRE((u8_t(7) / u8_t(2)).GetValue() == 3);
	REQUIRE_THROWS_AS(u8_t(1) / u8_t(0), InternalException);

	// modulo
	REQUIRE((u8_t(10) % u8_t(3)).GetValue() == 1);
	REQUIRE((u8_t(255) % u8_t(128)).GetValue() == 127);
	REQUIRE_THROWS_AS(u8_t(1) % u8_t(0), InternalException);
}

TEST_CASE("CheckedInteger signed arithmetic", "[checked_integer]") {
	// addition
	REQUIRE((i8_t(50) + i8_t(77)).GetValue() == 127);
	REQUIRE((i8_t(-50) + i8_t(50)).GetValue() == 0);
	REQUIRE((i8_t(-100) + i8_t(-28)).GetValue() == -128);
	REQUIRE_THROWS_AS(i8_t(127) + i8_t(1), InternalException);
	REQUIRE_THROWS_AS(i8_t(-128) + i8_t(-1), InternalException);

	// subtraction
	REQUIRE((i8_t(50) - i8_t(50)).GetValue() == 0);
	REQUIRE((i8_t(-100) - i8_t(28)).GetValue() == -128);
	REQUIRE((i8_t(100) - i8_t(-27)).GetValue() == 127);
	REQUIRE_THROWS_AS(i8_t(100) - i8_t(-100), InternalException);
	REQUIRE_THROWS_AS(i8_t(-128) - i8_t(1), InternalException);

	// multiplication
	REQUIRE((i8_t(10) * i8_t(12)).GetValue() == 120);
	REQUIRE((i8_t(-10) * i8_t(12)).GetValue() == -120);
	REQUIRE((i8_t(-1) * i8_t(-127)).GetValue() == 127);
	REQUIRE((i8_t(-64) * i8_t(2)).GetValue() == -128);
	REQUIRE((i8_t(0) * i8_t(127)).GetValue() == 0);
	REQUIRE_THROWS_AS(i8_t(127) * i8_t(2), InternalException);
	REQUIRE_THROWS_AS(i8_t(-1) * i8_t(-128), InternalException);
	REQUIRE_THROWS_AS(i8_t(-128) * i8_t(2), InternalException);
	REQUIRE_THROWS_AS(i16_t(-200) * i16_t(200), InternalException);

	// division
	REQUIRE((i8_t(100) / i8_t(10)).GetValue() == 10);
	REQUIRE((i8_t(-128) / i8_t(2)).GetValue() == -64);
	REQUIRE_THROWS_AS(i8_t(1) / i8_t(0), InternalException);
	REQUIRE_THROWS_AS(i8_t(-128) / i8_t(-1), InternalException);
	REQUIRE_THROWS_AS(i16_t(-32768) / i16_t(-1), InternalException);

	// modulo
	REQUIRE((i8_t(10) % i8_t(3)).GetValue() == 1);
	REQUIRE((i8_t(-10) % i8_t(3)).GetValue() == -1);
	REQUIRE_THROWS_AS(i8_t(1) % i8_t(0), InternalException);
	REQUIRE((i8_t(-128) % i8_t(-1)).GetValue() == 0);
}

TEST_CASE("CheckedInteger unary operators", "[checked_integer]") {
	// unary plus
	REQUIRE((+i8_t(42)).GetValue() == 42);
	REQUIRE((+u8_t(200)).GetValue() == 200);

	// unary minus (signed)
	REQUIRE((-i8_t(42)).GetValue() == -42);
	REQUIRE((-i8_t(-127)).GetValue() == 127);
	REQUIRE((-i16_t(0)).GetValue() == 0);
	REQUIRE_THROWS_AS(-i8_t(-128), InternalException);
	REQUIRE_THROWS_AS(-i16_t(-32768), InternalException);

	// unary minus (unsigned)
	REQUIRE((-u8_t(0)).GetValue() == 0);
	REQUIRE_THROWS_AS(-u8_t(1), InternalException);
	REQUIRE_THROWS_AS(-u32_t(100), InternalException);

	// bitwise NOT
	REQUIRE((~u8_t(0x0F)).GetValue() == 0xF0);
}

TEST_CASE("CheckedInteger increment decrement", "[checked_integer]") {
	// pre-increment
	u8_t a(10);
	++a;
	REQUIRE(a.GetValue() == 11);

	// post-increment
	u8_t b(10);
	auto old_b = b++;
	REQUIRE(old_b.GetValue() == 10);
	REQUIRE(b.GetValue() == 11);

	// increment at max
	u8_t c(255);
	REQUIRE_THROWS_AS(++c, InternalException);
	i8_t d(127);
	REQUIRE_THROWS_AS(++d, InternalException);
	u8_t e(255);
	REQUIRE_THROWS_AS(e++, InternalException);

	// pre-decrement
	u8_t f(10);
	--f;
	REQUIRE(f.GetValue() == 9);

	// post-decrement
	i8_t g(-5);
	auto old_g = g--;
	REQUIRE(old_g.GetValue() == -5);
	REQUIRE(g.GetValue() == -6);

	// decrement at min
	u8_t h(0);
	REQUIRE_THROWS_AS(--h, InternalException);
	i8_t j(-128);
	REQUIRE_THROWS_AS(--j, InternalException);
}

TEST_CASE("CheckedInteger comparisons", "[checked_integer]") {
	i64_t a(100);
	i64_t b(200);

	REQUIRE(a < b);
	REQUIRE(a <= b);
	REQUIRE(b > a);
	REQUIRE(b >= a);
	REQUIRE(a != b);
	REQUIRE_FALSE(a == b);
	REQUIRE(i16_t(5) == i16_t(5));
	REQUIRE(i16_t(-1) < i16_t(0));

	// comparison with plain int via implicit conversion
	REQUIRE(a == 100);
	REQUIRE(100 == a);
	REQUIRE(a < 200);
	REQUIRE(a > 50);
}

TEST_CASE("CheckedInteger bitwise and shift", "[checked_integer]") {
	REQUIRE((u8_t(0xFF) & u8_t(0x0F)).GetValue() == 0x0F);
	REQUIRE((u8_t(0xF0) | u8_t(0x0F)).GetValue() == 0xFF);
	REQUIRE((u8_t(0xFF) ^ u8_t(0x0F)).GetValue() == 0xF0);

	// left shift
	REQUIRE((u8_t(1) << u8_t(7)).GetValue() == 128);
	REQUIRE((u8_t(0) << u8_t(7)).GetValue() == 0);
	REQUIRE((u8_t(3) << u8_t(2)).GetValue() == 12);
	REQUIRE_THROWS_AS(u8_t(1) << u8_t(8), InternalException);
	REQUIRE_THROWS_AS(u8_t(128) << u8_t(1), InternalException);
	REQUIRE_THROWS_AS(u8_t(2) << u8_t(7), InternalException);

	// right shift
	REQUIRE((u8_t(128) >> u8_t(7)).GetValue() == 1);
	REQUIRE((u8_t(255) >> u8_t(4)).GetValue() == 15);
	REQUIRE_THROWS_AS(u8_t(1) >> u8_t(8), InternalException);

	// signed shift checks
	REQUIRE_THROWS_AS(i8_t(-1) << i8_t(1), InternalException);
	REQUIRE((i8_t(1) << i8_t(6)).GetValue() == 64);
	REQUIRE_THROWS_AS(i8_t(1) << i8_t(7), InternalException);

	// compound bitwise assignment
	u8_t v(0xFF);
	v &= u8_t(0x0F);
	REQUIRE(v.GetValue() == 0x0F);
	v |= u8_t(0xF0);
	REQUIRE(v.GetValue() == 0xFF);
	v ^= u8_t(0x0F);
	REQUIRE(v.GetValue() == 0xF0);

	// compound shift assignment
	u8_t s(1);
	s <<= u8_t(4);
	REQUIRE(s.GetValue() == 16);
	s >>= u8_t(2);
	REQUIRE(s.GetValue() == 4);
}

TEST_CASE("CheckedInteger compound assignment", "[checked_integer]") {
	u32_t u(100);
	u += 50;
	REQUIRE(u == 150u);
	u -= 25;
	REQUIRE(u == 125u);
	u *= 2;
	REQUIRE(u == 250u);
	u /= 5;
	REQUIRE(u == 50u);
	u %= 7;
	REQUIRE(u == 1u);

	// compound overflow
	i64_t a(NumericLimits<int64_t>::Maximum() - 10);
	REQUIRE_THROWS_AS(a += 20, InternalException);
	i64_t b(NumericLimits<int64_t>::Minimum() + 10);
	REQUIRE_THROWS_AS(b -= 20, InternalException);
	i32_t c(100000);
	REQUIRE_THROWS_AS(c *= 100000, InternalException);
	i64_t d(100);
	REQUIRE_THROWS_AS(d /= 0, InternalException);
	i64_t e(NumericLimits<int64_t>::Minimum());
	REQUIRE_THROWS_AS(e /= -1, InternalException);
}

TEST_CASE("CheckedInteger mixed with plain int", "[checked_integer]") {
	// checked + int (implicit conversion for same-type friend operator)
	u8_t a(100);
	auto b = a + 50;
	REQUIRE(b.GetValue() == 150);

	// int + checked (symmetric via friend ADL)
	auto c = 50 + a;
	REQUIRE(c.GetValue() == 150);

	// out-of-range result throws
	REQUIRE_THROWS_AS(a + 300, InternalException);
	// sign-mismatched operand with valid result succeeds
	REQUIRE((a + (-1)).GetValue() == 99);
	REQUIRE_THROWS_AS(a + (-101), InternalException);

	// subtraction underflow via compound assignment
	u16_t x(50);
	REQUIRE_THROWS_AS(x -= 100, InternalException);
}

TEST_CASE("CheckedInteger mixed-type compound assignment", "[checked_integer]") {
	// unsigned += negative signed: 56 + (-6) = 50, valid for uint16_t
	CheckedInteger<uint16_t> x(56);
	x += int32_t {-6};
	REQUIRE(x.GetValue() == 50);

	// unsigned -= negative signed: 50 - (-10) = 60
	x -= int32_t {-10};
	REQUIRE(x.GetValue() == 60);

	// unsigned *= signed
	CheckedInteger<uint8_t> y(10);
	y *= int16_t {12};
	REQUIRE(y.GetValue() == 120);

	// unsigned /= negative signed: 120 / -4 = -30, doesn't fit in uint8_t
	REQUIRE_THROWS_AS(y /= int16_t {-4}, InternalException);

	// unsigned %= signed
	CheckedInteger<uint16_t> z(100);
	z %= int32_t {7};
	REQUIRE(z.GetValue() == 2);

	// result underflow: 10 + (-20) = -10, doesn't fit in uint16_t
	CheckedInteger<uint16_t> w(10);
	REQUIRE_THROWS_AS(w += int32_t {-20}, InternalException);

	// result overflow: 250 + 200 as uint8_t
	CheckedInteger<uint8_t> v(250);
	REQUIRE_THROWS_AS(v += int16_t {200}, InternalException);

	// division by zero with mixed type
	CheckedInteger<uint16_t> d(100);
	REQUIRE_THROWS_AS(d /= int32_t {0}, InternalException);
	REQUIRE_THROWS_AS(d %= int32_t {0}, InternalException);
}

TEST_CASE("CheckedInteger mixed-type binary operators", "[checked_integer]") {
	// unsigned + negative signed yields valid result
	REQUIRE((u16_t(56) + int(-6)).GetValue() == 50);
	REQUIRE((int(-6) + u16_t(56)).GetValue() == 50);

	// unsigned - negative signed
	REQUIRE((u16_t(50) - int(-10)).GetValue() == 60);
	// int - unsigned
	REQUIRE((int(100) - u16_t(30)).GetValue() == 70);
	REQUIRE_THROWS_AS(int(10) - u16_t(20), InternalException);

	// multiplication
	REQUIRE((u8_t(10) * int16_t(12)).GetValue() == 120);
	REQUIRE((int16_t(12) * u8_t(10)).GetValue() == 120);
	REQUIRE_THROWS_AS(u8_t(200) * int16_t(2), InternalException);

	// division
	REQUIRE((u16_t(100) / int32_t(10)).GetValue() == 10);
	REQUIRE((int32_t(100) / u16_t(10)).GetValue() == 10);
	REQUIRE_THROWS_AS(u16_t(100) / int32_t(0), InternalException);
	REQUIRE_THROWS_AS(int32_t(100) / u16_t(0), InternalException);

	// modulo
	REQUIRE((u16_t(100) % int32_t(7)).GetValue() == 2);
	REQUIRE((int32_t(100) % u16_t(7)).GetValue() == 2);
	REQUIRE_THROWS_AS(u16_t(100) % int32_t(0), InternalException);

	// result underflow for unsigned
	REQUIRE_THROWS_AS(u16_t(10) + int32_t(-20), InternalException);
	REQUIRE_THROWS_AS(u8_t(10) - int16_t(20), InternalException);
}

TEST_CASE("CheckedInteger unsigned cannot be negative", "[checked_integer]") {
	REQUIRE_THROWS_AS(u32_t(-1), InternalException);
	REQUIRE_THROWS_AS(u64_t(-100), InternalException);
	REQUIRE_THROWS_AS(u8_t(-1), InternalException);

	REQUIRE_NOTHROW(u32_t(100));
	REQUIRE(u32_t(100).GetValue() == 100u);
}

TEST_CASE("CheckedInteger various types", "[checked_integer]") {
	// uint16_t
	auto r16 = u16_t(60000) + u16_t(5535);
	REQUIRE(r16.GetValue() == 65535);
	REQUIRE_THROWS_AS(u16_t(60000) + u16_t(5536), InternalException);

	// int32_t
	i32_t i32(2000000000);
	REQUIRE_THROWS_AS(i32 + i32_t(2000000000), InternalException);
	REQUIRE((i32 - i32_t(2000000000)).GetValue() == 0);

	// uint64_t
	u64_t u64(NumericLimits<uint64_t>::Maximum());
	REQUIRE_THROWS_AS(u64 + u64_t(1), InternalException);
	REQUIRE((u64 - u64_t(1)).GetValue() == NumericLimits<uint64_t>::Maximum() - 1);

	// int64_t
	i64_t i64max(NumericLimits<int64_t>::Maximum());
	REQUIRE_THROWS_AS(i64max + i64_t(1), InternalException);
	i64_t i64min(NumericLimits<int64_t>::Minimum());
	REQUIRE_THROWS_AS(i64min - i64_t(1), InternalException);
}

TEST_CASE("CheckedInteger edge cases", "[checked_integer]") {
	// multiply by zero
	REQUIRE((i32_t(NumericLimits<int32_t>::Maximum()) * i32_t(0)).GetValue() == 0);
	REQUIRE((u64_t(NumericLimits<uint64_t>::Maximum()) * u64_t(0)).GetValue() == 0);

	// add zero at boundary
	REQUIRE((i8_t(127) + i8_t(0)).GetValue() == 127);
	REQUIRE((i8_t(-128) + i8_t(0)).GetValue() == -128);

	// divide by 1
	REQUIRE((i8_t(-128) / i8_t(1)).GetValue() == -128);

	// chained operations
	auto result = (u16_t(100) + 50) * 2 - 100;
	REQUIRE(result.GetValue() == 200);

	// self-assignment
	i32_t v(10);
	v += v;
	REQUIRE(v.GetValue() == 20);
}
