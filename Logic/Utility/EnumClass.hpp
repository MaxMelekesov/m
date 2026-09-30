/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ENUM_CLASS_HPP
#define ENUM_CLASS_HPP

#include <type_traits>

/**
 * @brief Concepts for scoped enums that describe their own extent.
 *
 * `EnumClass` is stricter than `std::is_enum_v`: the type must be an `enum
 * class`, so a plain integer can never be passed where a code is expected.
 *
 * `EnumClassWithSize` additionally requires the enum to carry its own `Size`
 * sentinel — the same idea as the `UnusedField<N>` of a register: the number of
 * codes lives in the enum, so everything that has to be sized from it (bit
 * widths, arrays, static assertions) is derived at compile time.
 *
 * Usage:
 *
 * ```cpp
 * enum class Error : uint16_t { None = 0, Timeout = 1, Size };
 *
 * static_assert(m::EnumClassWithSize<Error>);
 * // 2 bits are enough for every code: std::bit_width(Error::Size)
 * ```
 */

namespace m {

template <typename T>
concept EnumClass =
    std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

template <typename T>
concept EnumClassWithSize = EnumClass<T> && requires {
  T::Size;
  requires std::is_same_v<decltype(T::Size), T>;
};

}  // namespace m

#endif  // ENUM_CLASS_HPP
