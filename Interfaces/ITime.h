/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ITIME_H
#define ITIME_H
#include <cstdint>

namespace m::ifc {

class Ms {
 private:
  uint32_t value_;

 public:
  using type = uint32_t;

  constexpr Ms() : value_(0) {}
  constexpr explicit Ms(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Ms operator-() const {
    return Ms{static_cast<type>(-static_cast<int32_t>(value_))};
  }

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
};

class Us {
 private:
  uint32_t value_;

 public:
  using type = uint32_t;

  constexpr Us() : value_(0) {}
  constexpr explicit Us(type value) : value_(value) {}
  constexpr auto value() const { return value_; }

  constexpr Us operator-() const {
    return Us{static_cast<type>(-static_cast<int32_t>(value_))};
  }

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
};

class ITime {
 public:
  virtual ~ITime() {}

  virtual void delay(Us us) = 0;
  virtual void delay(Ms us) = 0;
  virtual Us getTickUs() = 0;
  virtual Ms getTickMs() = 0;
  virtual Us getDiff(Us start) = 0;
  virtual Ms getDiff(Ms start) = 0;
};
}  // namespace m::ifc

static inline constexpr m::ifc::Ms operator""_Ms(uint64_t value) {
  return m::ifc::Ms{static_cast<uint32_t>(value)};
}
static inline constexpr m::ifc::Us operator""_Us(uint64_t value) {
  return m::ifc::Us{static_cast<uint32_t>(value)};
}

#endif  // ITIME_H