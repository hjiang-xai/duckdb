//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/checked_integer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/exception.hpp"
#include "duckdb/common/limits.hpp"
#include "duckdb/common/numeric_utils.hpp"

#include <cstdint>
#include <type_traits>

namespace duckdb {

//! CheckedInteger is a templated wrapper around integer types that throws
//! InternalException on overflow, underflow, or out-of-range construction.
template <class T>
class CheckedInteger {
	static_assert(std::is_integral_v<T>, "CheckedInteger only supports integral types");
	static_assert(!std::is_same_v<T, bool>, "CheckedInteger does not support bool");

	T value;

	static constexpr T MAX_VAL = std::numeric_limits<T>::max();
	static constexpr T MIN_VAL = std::numeric_limits<T>::min();
	static constexpr int BITS = std::numeric_limits<T>::digits + (std::is_signed_v<T> ? 1 : 0);

public:
	using value_type = T;

	constexpr CheckedInteger() noexcept : value(0) {
	}

	constexpr CheckedInteger(T v) noexcept : value(v) { // NOLINT
	}

	template <class U, std::enable_if_t<!std::is_same_v<std::decay_t<U>, T> && std::is_integral_v<std::decay_t<U>> &&
	                                        !std::is_same_v<std::decay_t<U>, bool>,
	                                    int> = 0>
	CheckedInteger(U v) : value(NumericCast<T>(v)) { // NOLINT
	}

	constexpr T GetValue() const noexcept {
		return value;
	}

	constexpr explicit operator T() const noexcept {
		return value;
	}

	constexpr explicit operator bool() const noexcept {
		return value != 0;
	}

	constexpr CheckedInteger operator+() const noexcept {
		return *this;
	}

	constexpr CheckedInteger operator-() const {
		if constexpr (std::is_unsigned_v<T>) {
			if (value != 0) {
				throw InternalException("CheckedInteger: negation of non-zero unsigned value");
			}
			return CheckedInteger {T(0)};
		} else {
			if (value == MIN_VAL) {
				throw InternalException("CheckedInteger: negation overflow");
			}
			return CheckedInteger {static_cast<T>(-value)};
		}
	}

	constexpr CheckedInteger operator~() const noexcept {
		return CheckedInteger {static_cast<T>(~value)};
	}

	constexpr CheckedInteger &operator++() {
		if (value == MAX_VAL) {
			throw InternalException("CheckedInteger: increment overflow");
		}
		++value;
		return *this;
	}

	constexpr CheckedInteger operator++(int) {
		CheckedInteger prev {value};
		++(*this);
		return prev;
	}

	constexpr CheckedInteger &operator--() {
		if (value == MIN_VAL) {
			throw InternalException("CheckedInteger: decrement underflow");
		}
		--value;
		return *this;
	}

	constexpr CheckedInteger operator--(int) {
		CheckedInteger prev {value};
		--(*this);
		return prev;
	}

	// SFINAE helper for mixed-type overloads: U is a different integral type, not bool.
	template <class U>
	using EnableMixed = std::enable_if_t<!std::is_same_v<std::decay_t<U>, T> &&
	                                     std::is_integral_v<std::decay_t<U>> &&
	                                     !std::is_same_v<std::decay_t<U>, bool>, int>;

	// Mixed-type compound assignment: performs arithmetic in int64_t when both
	// operands are <= 32-bit, so that e.g. CheckedInteger<uint16_t>(56) += int(-6)
	// correctly yields 50 instead of rejecting -6 during operand conversion.
	template <class U, EnableMixed<U> = 0>
	CheckedInteger &operator+=(U rhs) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			value = NumericCast<T>(static_cast<int64_t>(value) + static_cast<int64_t>(rhs));
		} else {
			*this += CheckedInteger(rhs);
		}
		return *this;
	}

	template <class U, EnableMixed<U> = 0>
	CheckedInteger &operator-=(U rhs) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			value = NumericCast<T>(static_cast<int64_t>(value) - static_cast<int64_t>(rhs));
		} else {
			*this -= CheckedInteger(rhs);
		}
		return *this;
	}

	template <class U, EnableMixed<U> = 0>
	CheckedInteger &operator*=(U rhs) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			value = NumericCast<T>(static_cast<int64_t>(value) * static_cast<int64_t>(rhs));
		} else {
			*this *= CheckedInteger(rhs);
		}
		return *this;
	}

	template <class U, EnableMixed<U> = 0>
	CheckedInteger &operator/=(U rhs) {
		if (rhs == 0) {
			throw InternalException("CheckedInteger: division by zero");
		}
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			value = NumericCast<T>(static_cast<int64_t>(value) / static_cast<int64_t>(rhs));
		} else {
			*this /= CheckedInteger(rhs);
		}
		return *this;
	}

	template <class U, EnableMixed<U> = 0>
	CheckedInteger &operator%=(U rhs) {
		if (rhs == 0) {
			throw InternalException("CheckedInteger: modulo by zero");
		}
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			value = NumericCast<T>(static_cast<int64_t>(value) % static_cast<int64_t>(rhs));
		} else {
			*this %= CheckedInteger(rhs);
		}
		return *this;
	}

	constexpr CheckedInteger &operator+=(CheckedInteger rhs) {
		if constexpr (std::is_unsigned_v<T>) {
			if (rhs.value > MAX_VAL - value) {
				throw InternalException("CheckedInteger: addition overflow");
			}
		} else {
			if (rhs.value > 0 && value > MAX_VAL - rhs.value) {
				throw InternalException("CheckedInteger: addition overflow");
			}
			if (rhs.value < 0 && value < MIN_VAL - rhs.value) {
				throw InternalException("CheckedInteger: addition underflow");
			}
		}
		value += rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator-=(CheckedInteger rhs) {
		if constexpr (std::is_unsigned_v<T>) {
			if (value < rhs.value) {
				throw InternalException("CheckedInteger: subtraction underflow");
			}
		} else {
			if (rhs.value > 0 && value < MIN_VAL + rhs.value) {
				throw InternalException("CheckedInteger: subtraction underflow");
			}
			if (rhs.value < 0 && value > MAX_VAL + rhs.value) {
				throw InternalException("CheckedInteger: subtraction overflow");
			}
		}
		value -= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator*=(CheckedInteger rhs) {
		T a = value, b = rhs.value;
		if (a == 0 || b == 0) {
			value = 0;
			return *this;
		}
		if constexpr (std::is_unsigned_v<T>) {
			if (a > MAX_VAL / b) {
				throw InternalException("CheckedInteger: multiplication overflow");
			}
		} else {
			if (a > 0) {
				if (b > 0) {
					if (a > MAX_VAL / b) {
						throw InternalException("CheckedInteger: multiplication overflow");
					}
				} else {
					if (b < MIN_VAL / a) {
						throw InternalException("CheckedInteger: multiplication underflow");
					}
				}
			} else {
				if (b > 0) {
					if (a < MIN_VAL / b) {
						throw InternalException("CheckedInteger: multiplication underflow");
					}
				} else {
					if (b < MAX_VAL / a) {
						throw InternalException("CheckedInteger: multiplication overflow");
					}
				}
			}
		}
		value = static_cast<T>(a * b);
		return *this;
	}

	constexpr CheckedInteger &operator/=(CheckedInteger rhs) {
		if (rhs.value == 0) {
			throw InternalException("CheckedInteger: division by zero");
		}
		if constexpr (std::is_signed_v<T>) {
			if (value == MIN_VAL && rhs.value == T(-1)) {
				throw InternalException("CheckedInteger: division overflow");
			}
		}
		value /= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator%=(CheckedInteger rhs) {
		if (rhs.value == 0) {
			throw InternalException("CheckedInteger: modulo by zero");
		}
		if constexpr (std::is_signed_v<T>) {
			if (value == MIN_VAL && rhs.value == T(-1)) {
				value = 0;
				return *this;
			}
		}
		value %= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator&=(CheckedInteger rhs) noexcept {
		value &= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator|=(CheckedInteger rhs) noexcept {
		value |= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator^=(CheckedInteger rhs) noexcept {
		value ^= rhs.value;
		return *this;
	}

	constexpr CheckedInteger &operator<<=(CheckedInteger rhs) {
		auto n = rhs.value;
		if constexpr (std::is_signed_v<T>) {
			if (n < 0) {
				throw InternalException("CheckedInteger: negative shift amount");
			}
		}
		if (n >= static_cast<T>(BITS)) {
			throw InternalException("CheckedInteger: shift amount out of range");
		}
		if constexpr (std::is_signed_v<T>) {
			if (value < 0) {
				throw InternalException("CheckedInteger: left shift of negative value");
			}
		}
		if (value > (MAX_VAL >> n)) {
			throw InternalException("CheckedInteger: left shift overflow");
		}
		value = static_cast<T>(value << n);
		return *this;
	}

	constexpr CheckedInteger &operator>>=(CheckedInteger rhs) {
		auto n = rhs.value;
		if constexpr (std::is_signed_v<T>) {
			if (n < 0) {
				throw InternalException("CheckedInteger: negative shift amount");
			}
		}
		if (n >= static_cast<T>(BITS)) {
			throw InternalException("CheckedInteger: shift amount out of range");
		}
		value >>= n;
		return *this;
	}

	friend constexpr CheckedInteger operator+(CheckedInteger a, CheckedInteger b) {
		return a += b;
	}
	friend constexpr CheckedInteger operator-(CheckedInteger a, CheckedInteger b) {
		return a -= b;
	}
	friend constexpr CheckedInteger operator*(CheckedInteger a, CheckedInteger b) {
		return a *= b;
	}
	friend constexpr CheckedInteger operator/(CheckedInteger a, CheckedInteger b) {
		return a /= b;
	}
	friend constexpr CheckedInteger operator%(CheckedInteger a, CheckedInteger b) {
		return a %= b;
	}

	friend constexpr CheckedInteger operator&(CheckedInteger a, CheckedInteger b) {
		return a &= b;
	}
	friend constexpr CheckedInteger operator|(CheckedInteger a, CheckedInteger b) {
		return a |= b;
	}
	friend constexpr CheckedInteger operator^(CheckedInteger a, CheckedInteger b) {
		return a ^= b;
	}
	friend constexpr CheckedInteger operator<<(CheckedInteger a, CheckedInteger b) {
		return a <<= b;
	}
	friend constexpr CheckedInteger operator>>(CheckedInteger a, CheckedInteger b) {
		return a >>= b;
	}

	// Mixed-type binary operators: delegate to the mixed-type compound
	// assignments so that e.g. CheckedInteger<uint16_t>(56) + int(-6) works.
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator+(CheckedInteger a, U b) {
		a += b;
		return a;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator-(CheckedInteger a, U b) {
		a -= b;
		return a;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator*(CheckedInteger a, U b) {
		a *= b;
		return a;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator/(CheckedInteger a, U b) {
		a /= b;
		return a;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator%(CheckedInteger a, U b) {
		a %= b;
		return a;
	}

	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator+(U a, CheckedInteger b) {
		b += a;
		return b;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator*(U a, CheckedInteger b) {
		b *= a;
		return b;
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator-(U a, CheckedInteger b) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			return CheckedInteger {NumericCast<T>(static_cast<int64_t>(a) - static_cast<int64_t>(b.value))};
		} else {
			return CheckedInteger(a) - b;
		}
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator/(U a, CheckedInteger b) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			if (b.value == 0) {
				throw InternalException("CheckedInteger: division by zero");
			}
			return CheckedInteger {NumericCast<T>(static_cast<int64_t>(a) / static_cast<int64_t>(b.value))};
		} else {
			return CheckedInteger(a) / b;
		}
	}
	template <class U, EnableMixed<U> = 0>
	friend CheckedInteger operator%(U a, CheckedInteger b) {
		if constexpr (sizeof(T) <= 4 && sizeof(U) <= 4) {
			if (b.value == 0) {
				throw InternalException("CheckedInteger: modulo by zero");
			}
			return CheckedInteger {NumericCast<T>(static_cast<int64_t>(a) % static_cast<int64_t>(b.value))};
		} else {
			return CheckedInteger(a) % b;
		}
	}

	friend constexpr bool operator==(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value == b.value;
	}
	friend constexpr bool operator!=(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value != b.value;
	}
	friend constexpr bool operator<(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value < b.value;
	}
	friend constexpr bool operator<=(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value <= b.value;
	}
	friend constexpr bool operator>(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value > b.value;
	}
	friend constexpr bool operator>=(CheckedInteger a, CheckedInteger b) noexcept {
		return a.value >= b.value;
	}
};

} // namespace duckdb
