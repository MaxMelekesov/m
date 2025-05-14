/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef BPS_HPP
#define BPS_HPP

#include <compare>
#include <type_traits>

template <typename T>
  requires std::is_arithmetic_v<T>
// Bytes per second
class Bps {
 public:
  using type = T;

  constexpr Bps() : value_(0) {}
  constexpr explicit Bps(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Bps operator-() const { return Bps{-value_}; }

  constexpr Bps& operator+=(const Bps& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Bps& operator-=(const Bps& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Bps& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Bps& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Bps operator+(Bps lhs, const Bps& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Bps operator-(Bps lhs, const Bps& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Bps operator*(Bps lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Bps operator*(type scalar, Bps rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Bps operator/(Bps lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Bps& lhs, const Bps& rhs) = default;

  type value_;
};

#endif  // BPS_HPP