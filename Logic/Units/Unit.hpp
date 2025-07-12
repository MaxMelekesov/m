/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef UNIT_HPP
#define UNIT_HPP

#include <compare>
#include <type_traits>

template <typename Derived, typename Storage>
class Unit {
 public:
  using type = Storage;

  constexpr Unit() : value_(0) {}
  constexpr explicit Unit(type value) : value_(value) {}

  constexpr auto value() const { return value_; }

  constexpr Derived operator-() const
    requires std::is_signed_v<type>
  {
    return Derived(-value_);
  }

  constexpr Derived& operator+=(const Derived& other) {
    value_ += other.value();
    return static_cast<Derived&>(*this);
  }

  constexpr Derived& operator-=(const Derived& other) {
    value_ -= other.value();
    return static_cast<Derived&>(*this);
  }

  constexpr Derived& operator++() {
    ++value_;
    return static_cast<Derived&>(*this);
  }

  constexpr Derived operator++(int) {
    Derived tmp = static_cast<Derived&>(*this);
    ++(*this);
    return tmp;
  }

  constexpr Derived& operator--() {
    --value_;
    return static_cast<Derived&>(*this);
  }

  constexpr Derived operator--(int) {
    Derived tmp = static_cast<Derived&>(*this);
    --(*this);
    return tmp;
  }

  constexpr Derived& operator*=(type scalar) {
    value_ *= scalar;
    return static_cast<Derived&>(*this);
  }

  constexpr Derived& operator/=(type scalar) {
    value_ /= scalar;
    return static_cast<Derived&>(*this);
  }

  friend constexpr Derived operator+(Derived lhs, const Derived& rhs) {
    lhs += rhs;
    return lhs;
  }

  friend constexpr Derived operator-(Derived lhs, const Derived& rhs) {
    lhs -= rhs;
    return lhs;
  }

  friend constexpr Derived operator*(Derived lhs, type scalar) {
    lhs *= scalar;
    return lhs;
  }

  friend constexpr Derived operator*(type scalar, Derived rhs) {
    rhs *= scalar;
    return rhs;
  }

  friend constexpr Derived operator/(Derived lhs, type scalar) {
    lhs /= scalar;
    return lhs;
  }

  constexpr auto operator<=>(const Unit&) const = default;

  type value_;
};

#endif  // UNIT_HPP