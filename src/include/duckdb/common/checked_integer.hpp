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
#include "duckdb/common/operator/add.hpp"
#include "duckdb/common/operator/multiply.hpp"
#include "duckdb/common/operator/subtract.hpp"

#include <type_traits>

namespace duckdb {

//! checked_integer is a templated wrapper around integer types (signed or unsigned)
//! that throws an InternalException on overflow or underflow for arithmetic operations
//! (++, --, +=, -=, *=, /=, +, -, *, /) similar to Rust's integer overflow behavior in debug mode.
template <class T>
class checked_integer {
	static_assert(std::is_integral<T>::value, "checked_integer only supports integral types");

private:
	T value;

public:
	using value_type = T;

	checked_integer() : value(0) {}
	checked_integer(T v) : value(v) {}  // implicit for seamless use as normal integer

	// implicit conversion to underlying type for seamless use as normal integer
	operator T() const {
		return value;
	}

	T GetValue() const {
		return value;
	}

	// increment / decrement
	checked_integer &operator++() {
		T result;
		if (!TryAddOperator::Operation(value, T(1), result)) {
			throw InternalException("Overflow in increment of checked_integer");
		}
		value = result;
		return *this;
	}

	checked_integer operator++(int) {
		checked_integer tmp(*this);
		++(*this);
		return tmp;
	}

	checked_integer &operator--() {
		T result;
		if (!TrySubtractOperator::Operation(value, T(1), result)) {
			throw InternalException("Underflow in decrement of checked_integer");
		}
		value = result;
		return *this;
	}

	checked_integer operator--(int) {
		checked_integer tmp(*this);
		--(*this);
		return tmp;
	}

	// compound assignment
	checked_integer &operator+=(checked_integer rhs) {
		return operator+=(rhs.value);
	}
	checked_integer &operator+=(T rhs) {
		T result;
		if (!TryAddOperator::Operation(value, rhs, result)) {
			throw InternalException("Overflow in addition for checked_integer");
		}
		value = result;
		return *this;
	}

	checked_integer &operator-=(checked_integer rhs) {
		return operator-=(rhs.value);
	}
	checked_integer &operator-=(T rhs) {
		T result;
		if (!TrySubtractOperator::Operation(value, rhs, result)) {
			throw InternalException("Underflow in subtraction for checked_integer");
		}
		value = result;
		return *this;
	}

	checked_integer &operator*=(checked_integer rhs) {
		return operator*=(rhs.value);
	}
	checked_integer &operator*=(T rhs) {
		T result;
		if (!TryMultiplyOperator::Operation(value, rhs, result)) {
			throw InternalException("Overflow in multiplication for checked_integer");
		}
		value = result;
		return *this;
	}

	checked_integer &operator/=(checked_integer rhs) {
		return operator/=(rhs.value);
	}
	checked_integer &operator/=(T rhs) {
		if (rhs == 0) {
			throw InternalException("Division by zero in checked_integer");
		}
		if (NumericLimits<T>::IsSigned() && value == NumericLimits<T>::Minimum() && rhs == T(-1)) {
			throw InternalException("Overflow in division for checked_integer");
		}
		value /= rhs;
		return *this;
	}

	// binary arithmetic (return new checked_integer)
	checked_integer operator+(checked_integer rhs) const {
		return operator+(rhs.value);
	}
	checked_integer operator+(T rhs) const {
		T result;
		if (!TryAddOperator::Operation(value, rhs, result)) {
			throw InternalException("Overflow in addition for checked_integer");
		}
		return checked_integer(result);
	}

	checked_integer operator-(checked_integer rhs) const {
		return operator-(rhs.value);
	}
	checked_integer operator-(T rhs) const {
		T result;
		if (!TrySubtractOperator::Operation(value, rhs, result)) {
			throw InternalException("Underflow in subtraction for checked_integer");
		}
		return checked_integer(result);
	}

	checked_integer operator*(checked_integer rhs) const {
		return operator*(rhs.value);
	}
	checked_integer operator*(T rhs) const {
		T result;
		if (!TryMultiplyOperator::Operation(value, rhs, result)) {
			throw InternalException("Overflow in multiplication for checked_integer");
		}
		return checked_integer(result);
	}

	checked_integer operator/(checked_integer rhs) const {
		return operator/(rhs.value);
	}
	checked_integer operator/(T rhs) const {
		if (rhs == 0) {
			throw InternalException("Division by zero in checked_integer");
		}
		if (NumericLimits<T>::IsSigned() && value == NumericLimits<T>::Minimum() && rhs == T(-1)) {
			throw InternalException("Overflow in division for checked_integer");
		}
		return checked_integer(value / rhs);
	}

	// comparisons to support usage in conditions/loops
	bool operator==(const checked_integer &other) const {
		return value == other.value;
	}
	bool operator!=(const checked_integer &other) const {
		return value != other.value;
	}
	bool operator<(const checked_integer &other) const {
		return value < other.value;
	}
	bool operator>(const checked_integer &other) const {
		return value > other.value;
	}
	bool operator<=(const checked_integer &other) const {
		return value <= other.value;
	}
	bool operator>=(const checked_integer &other) const {
		return value >= other.value;
	}

	bool operator==(T other) const {
		return value == other;
	}
	bool operator!=(T other) const {
		return value != other;
	}
	bool operator<(T other) const {
		return value < other;
	}
	bool operator>(T other) const {
		return value > other;
	}
	bool operator<=(T other) const {
		return value <= other;
	}
	bool operator>=(T other) const {
		return value >= other;
	}

	// for symmetry, also allow T op checked via these? but for now member covers checked op T
};

// non-member operators for symmetry (T op checked)
template <class TL, class TR>
checked_integer<TR> operator+(TL lhs, const checked_integer<TR> &rhs) {
	return checked_integer<TR>(lhs) + rhs.GetValue();
}

template <class TL, class TR>
checked_integer<TR> operator-(TL lhs, const checked_integer<TR> &rhs) {
	return checked_integer<TR>(lhs) - rhs.GetValue();
}

template <class TL, class TR>
checked_integer<TR> operator*(TL lhs, const checked_integer<TR> &rhs) {
	return checked_integer<TR>(lhs) * rhs.GetValue();
}

template <class TL, class TR>
checked_integer<TR> operator/(TL lhs, const checked_integer<TR> &rhs) {
	return checked_integer<TR>(lhs) / rhs.GetValue();
}



// type aliases for templated versions (Rust-inspired short names)
using i8_t = checked_integer<int8_t>;
using i16_t = checked_integer<int16_t>;
using i32_t = checked_integer<int32_t>;
using i64_t = checked_integer<int64_t>;
using u8_t = checked_integer<uint8_t>;
using u16_t = checked_integer<uint16_t>;
using u32_t = checked_integer<uint32_t>;
using u64_t = checked_integer<uint64_t>;

// longer aliases for clarity
using checked_i8_t = i8_t;
using checked_i16_t = i16_t;
using checked_i32_t = i32_t;
using checked_i64_t = i64_t;
using checked_u8_t = u8_t;
using checked_u16_t = u16_t;
using checked_u32_t = u32_t;
using checked_u64_t = u64_t;

} // namespace duckdb
