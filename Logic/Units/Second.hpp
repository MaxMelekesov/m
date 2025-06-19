/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SECOND_HPP
#define SECOND_HPP

#include <compare>
#include <type_traits>

template <typename T>
  requires std::is_arithmetic_v<T>
class Sec {
 public:
  using type = T;

  constexpr Sec() : value_(0) {}
  constexpr explicit Sec(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Sec operator-() const { return Sec{-value_}; }

  constexpr Sec& operator+=(const Sec& other) {
    value_ += other.value_;
    return *this;
  }

  constexpr Sec& operator-=(const Sec& other) {
    value_ -= other.value_;
    return *this;
  }

  constexpr Sec& operator*=(type scalar) {
    value_ *= scalar;
    return *this;
  }

  constexpr Sec& operator/=(type scalar) {
    value_ /= scalar;
    return *this;
  }

  friend constexpr Sec operator+(Sec lhs, const Sec& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Sec operator-(Sec lhs, const Sec& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Sec operator*(Sec lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Sec operator*(type scalar, Sec rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Sec operator/(Sec lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  friend constexpr auto operator<=>(const Sec& lhs, const Sec& rhs) = default;

  type value_;
};

namespace m::ifc {
template <typename T>
concept CSec =
    requires { typename T::type; } && std::is_same_v<T, Sec<typename T::type>>;
}  // namespace m::ifc

#endif  // SECOND_HPP