/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CELSIUS_H
#define CELSIUS_H

#include <cstdint>

template <typename T>
class Celsius {
 public:
  using type = T;

  constexpr Celsius() : value_(0) {}
  constexpr explicit Celsius(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Celsius operator-() const { return Celsius{-value_}; }

  constexpr Celsius& operator+=(const Celsius& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Celsius& operator-=(const Celsius& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Celsius& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Celsius& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Celsius operator+(Celsius lhs, const Celsius& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Celsius operator-(Celsius lhs, const Celsius& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Celsius operator*(Celsius lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Celsius operator*(type scalar, Celsius rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Celsius operator/(Celsius lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Celsius& lhs,
                                    const Celsius& rhs) = default;

 private:
  type value_;
};

inline constexpr Celsius<float> operator""_Cf(unsigned long long value) {
  return Celsius<float>(static_cast<float>(value));
}

inline constexpr Celsius<float> operator""_Cf(long double value) {
  return Celsius<float>(static_cast<float>(value));
}

inline constexpr Celsius<int32_t> operator""_Ci(unsigned long long value) {
  return Celsius<int32_t>(static_cast<int32_t>(value));
}

inline constexpr Celsius<int32_t> operator""_Ci(long double value) {
  return Celsius<int32_t>(static_cast<int32_t>(value));
}

#endif  // CELSIUS_H