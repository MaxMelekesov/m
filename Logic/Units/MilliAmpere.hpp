/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MILLI_AMPERE_HPP
#define MILLI_AMPERE_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct mA : public Unit<mA<T>, T> {
 public:
  using Unit<mA<T>, T>::Unit;
};

template <typename T>
mA(T) -> mA<T>;

namespace m::ifc {
template <typename T>
concept CmA = requires { typename T::type; } &&
              std::is_base_of_v<mA<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CmA<mA<uint32_t>>, "mA must satisfy CmA concept");

#endif  // MILLI_AMPERE_HPP