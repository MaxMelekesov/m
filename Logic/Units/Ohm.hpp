/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef OHM_H
#define OHM_H

#include <cstdint>

template <typename T>
class Ohm {
 public:
  using type = T;

  constexpr Ohm() : value_(0) {}
  constexpr explicit Ohm(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Ohm operator-() const { return Ohm{-value_}; }

  constexpr Ohm& operator+=(const Ohm& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Ohm& operator-=(const Ohm& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Ohm& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Ohm& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Ohm operator+(Ohm lhs, const Ohm& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Ohm operator-(Ohm lhs, const Ohm& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Ohm operator*(Ohm lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Ohm operator*(type scalar, Ohm rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Ohm operator/(Ohm lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Ohm& lhs, const Ohm& rhs) = default;

  friend constexpr type operator/(const Ohm& lhs, const Ohm& rhs) {
    return lhs.value_ / rhs.value_;
  }

 private:
  type value_;
};

#endif  // OHM_H