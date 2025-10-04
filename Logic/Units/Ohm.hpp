/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef OHM_HPP
#define OHM_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Ohm : public Unit<Ohm<T>, T> {
 public:
  using Unit<Ohm<T>, T>::Unit;
};

namespace m::ifc {
template <typename T>
concept COhm = requires { typename T::type; } &&
               std::is_base_of_v<Ohm<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::COhm<Ohm<uint32_t>>, "Ohm must satisfy COhm concept");

#endif  // OHM_HPP