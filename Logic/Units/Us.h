/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef US_H
#define US_H

#include <cstdint>

template <typename T>
class Us {
 public:
  using type = T;

  constexpr Us() : value_(0) {}
  constexpr explicit Us(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Us operator-() const { return Us{-value_}; }

  constexpr Us& operator+=(const Us& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Us& operator-=(const Us& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Us& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Us& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Us operator+(Us lhs, const Us& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Us operator-(Us lhs, const Us& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Us operator*(Us lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Us operator*(type scalar, Us rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Us operator/(Us lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Us& lhs, const Us& rhs) = default;

 private:
  type value_;
};

#endif  // US_H