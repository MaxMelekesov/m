/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef HOUR_HPP
#define HOUR_HPP

#include <compare>
#include <type_traits>

template <typename T>
  requires std::is_arithmetic_v<T>
class Hour {
 public:
  using type = T;

  constexpr Hour() : value_(0) {}
  constexpr explicit Hour(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Hour operator-() const { return Hour{-value_}; }

  constexpr Hour& operator+=(const Hour& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Hour& operator-=(const Hour& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Hour& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Hour& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Hour operator+(Hour lhs, const Hour& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Hour operator-(Hour lhs, const Hour& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Hour operator*(Hour lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Hour operator*(type scalar, Hour rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Hour operator/(Hour lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Hour& lhs, const Hour& rhs) = default;

  type value_;
};

namespace m::c {
template <typename T>
concept CHour =
    requires { typename T::type; } && std::is_same_v<T, Hour<typename T::type>>;
}  // namespace m::c

#endif  // HOURS_HPP