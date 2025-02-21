/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef KELVIN_H
#define KELVIN_H

#include <cstdint>

template <typename T>
class Kelvin {
 public:
  using type = T;

  constexpr Kelvin() : value_(0) {}
  constexpr explicit Kelvin(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Kelvin operator-() const { return Kelvin{-value_}; }

  constexpr Kelvin& operator+=(const Kelvin& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Kelvin& operator-=(const Kelvin& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Kelvin& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Kelvin& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Kelvin operator+(Kelvin lhs, const Kelvin& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Kelvin operator-(Kelvin lhs, const Kelvin& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Kelvin operator*(Kelvin lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Kelvin operator*(type scalar, Kelvin rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Kelvin operator/(Kelvin lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Kelvin& lhs,
                                    const Kelvin& rhs) = default;

 private:
  type value_;
};

#endif  // KELVIN_H