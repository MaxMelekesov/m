/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MS_H
#define MS_H

#include <cstdint>

template <typename T>
class Ms {
 public:
  using type = T;

  constexpr Ms() : value_(0) {}
  constexpr explicit Ms(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Ms operator-() const { return Ms{-value_}; }

  constexpr Ms& operator+=(const Ms& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Ms& operator-=(const Ms& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Ms& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Ms& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Ms operator+(Ms lhs, const Ms& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Ms operator-(Ms lhs, const Ms& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Ms operator*(Ms lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Ms operator*(type scalar, Ms rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Ms operator/(Ms lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Ms& lhs, const Ms& rhs) = default;

 private:
  type value_;
};

#endif  // MS_H