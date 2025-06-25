/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef STATIC_MAP_HPP
#define STATIC_MAP_HPP

#include <concepts>
#include <type_traits>

namespace m {

/* Usage example:

struct RegA {};
struct RegB {};
struct RegC {};

using MyMap = m::StaticMap<uint8_t, m::Pair<RegA, 0x10>, m::Pair<RegB, 0x20>,
                         m::Pair<RegC, 0x30>>;
// using MyMap = m::StaticMap<uint8_t, m::Pair<RegA,
// 0x10>, m::Pair<RegB, 0x20>, m::Pair<RegC, 0xFF'FF>>; // Compile error

static_assert(MyMap::value<RegA>() == 0x10, "RegA address must be 0x10");
static_assert(MyMap::value<RegB>() == 0x20, "RegB address must be 0x20");
static_assert(MyMap::value<RegC>() == 0x30, "RegC address must be 0x30");

*/

template <typename T, auto Value>
struct Pair {
  using Key = T;
  static constexpr auto value = Value;
};

template <typename T>
concept CPairs = requires {
  T::value;
  typename T::Key;
};

template <typename T, auto V>
constexpr bool is_narrowing_convertible() {
  return requires { T{V}; };
}

template <typename ValueType, CPairs... Pairs>
  requires(is_narrowing_convertible<ValueType, Pairs::value>() && ...)
struct StaticMap {
  using StorageType = ValueType;

  template <typename Key>
    requires((std::is_same_v<Key, typename Pairs::Key> || ...))
  static constexpr ValueType value() {
    return value_impl<Key, Pairs...>();
  }

 private:
  template <typename Key>
  static constexpr ValueType value_impl() {
    static_assert(sizeof...(Pairs) != 0, "Key not found in StaticMap");
    return ValueType{};
  }

  template <typename Key, typename First, typename... Rest>
  static constexpr ValueType value_impl() {
    if constexpr (std::is_same_v<Key, typename First::Key>)
      return static_cast<ValueType>(First::value);
    else
      return value_impl<Key, Rest...>();
  }
};

}  // namespace m

#endif  // STATIC_MAP_HPP