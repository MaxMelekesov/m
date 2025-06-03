/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIME_UNITS_HPP
#define TIME_UNITS_HPP

#include <Hour.hpp>
#include <Minute.hpp>
#include <Ms.hpp>
#include <Second.hpp>
#include <Us.hpp>

// toHour
template <typename T>
constexpr Hour<T> toHour(const Min<T>& min) {
  return Hour<T>(min.value() / static_cast<T>(60));
}
template <typename T>
constexpr Hour<T> toHour(const Sec<T>& sec) {
  return Hour<T>(sec.value() / static_cast<T>(3'600));
}
template <typename T>
constexpr Hour<T> toHour(const Ms<T>& ms) {
  return Hour<T>(ms.value() / static_cast<T>(3'600'000));
}
template <typename T>
constexpr Hour<T> toHour(const Us<T>& us) {
  return Hour<T>(us.value() / static_cast<T>(3'600'000'000));
}

// toMin
template <typename T>
constexpr Min<T> toMin(const Hour<T>& hour) {
  return Min<T>(hour.value() * static_cast<T>(60));
}
template <typename T>
constexpr Min<T> toMin(const Sec<T>& sec) {
  return Min<T>(sec.value() / static_cast<T>(60));
}
template <typename T>
constexpr Min<T> toMin(const Ms<T>& ms) {
  return Min<T>(ms.value() / static_cast<T>(60'000));
}
template <typename T>
constexpr Min<T> toMin(const Us<T>& us) {
  return Min<T>(us.value() / static_cast<T>(60'000'000));
}

// toSec
template <typename T>
constexpr Sec<T> toSec(const Hour<T>& hour) {
  return Sec<T>(hour.value() * static_cast<T>(3'600));
}
template <typename T>
constexpr Sec<T> toSec(const Min<T>& min) {
  return Sec<T>(min.value() * static_cast<T>(60));
}
template <typename T>
constexpr Sec<T> toSec(const Ms<T>& ms) {
  return Sec<T>(ms.value() / static_cast<T>(1'000));
}
template <typename T>
constexpr Sec<T> toSec(const Us<T>& us) {
  return Sec<T>(us.value() / static_cast<T>(1'000'000));
}

// toMs
template <typename T>
constexpr Ms<T> toMs(const Hour<T>& hour) {
  return Ms<T>(hour.value() * static_cast<T>(3'600'000));
}
template <typename T>
constexpr Ms<T> toMs(const Min<T>& min) {
  return Ms<T>(min.value() * static_cast<T>(60'000));
}
template <typename T>
constexpr Ms<T> toMs(const Sec<T>& sec) {
  return Ms<T>(sec.value() * static_cast<T>(1'000));
}
template <typename T>
constexpr Ms<T> toMs(const Us<T>& us) {
  return Ms<T>(us.value() / static_cast<T>(1'000));
}

// toUs
template <typename T>
constexpr Us<T> toUs(const Hour<T>& hour) {
  return Us<T>(hour.value() * static_cast<T>(3'600'000'000));
}
template <typename T>
constexpr Us<T> toUs(const Min<T>& min) {
  return Us<T>(min.value() * static_cast<T>(60'000'000));
}
template <typename T>
constexpr Us<T> toUs(const Sec<T>& sec) {
  return Us<T>(sec.value() * static_cast<T>(1'000'000));
}
template <typename T>
constexpr Us<T> toUs(const Ms<T>& ms) {
  return Us<T>(ms.value() * static_cast<T>(1'000));
}

#endif  // TIME_UNITS_HPP