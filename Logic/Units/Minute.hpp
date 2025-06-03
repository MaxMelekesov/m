/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MINUTE_HPP
#define MINUTE_HPP

#include <compare>
#include <type_traits>

template <typename T>
  requires std::is_arithmetic_v<T>
class Min {
 public:
  using type = T;

  constexpr Min() : value_(0) {}
  constexpr explicit Min(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Min operator-() const { return Min{-value_}; }

  constexpr Min& operator+=(const Min& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Min& operator-=(const Min& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Min& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Min& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Min operator+(Min lhs, const Min& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Min operator-(Min lhs, const Min& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Min operator*(Min lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Min operator*(type scalar, Min rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Min operator/(Min lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Min& lhs, const Min& rhs) = default;

  type value_;
};

template <typename T>
concept CMin =
    requires { typename T::type; } && std::is_same_v<T, Min<typename T::type>>;

#endif  // MINUTE_HPP