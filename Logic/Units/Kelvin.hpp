/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef KELVIN_HPP
#define KELVIN_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Kelvin : public Unit<Kelvin<T>, T> {
 public:
  using Unit<Kelvin<T>, T>::Unit;
};

template <typename T>
Kelvin(T) -> Kelvin<T>;

namespace m::ifc {
template <typename T>
concept CKelvin = requires { typename T::type; } &&
                  std::is_base_of_v<Kelvin<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CKelvin<Kelvin<uint32_t>>,
              "Kelvin must satisfy CKelvin concept");

#endif  // KELVIN_HPP