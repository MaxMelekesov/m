/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MILLI_VOLT_HPP
#define MILLI_VOLT_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct mV : public Unit<mV<T>, T> {
 public:
  using Unit<mV<T>, T>::Unit;
};

template <typename T>
mV(T) -> mV<T>;

namespace m::ifc {
template <typename T>
concept CmV = requires { typename T::type; } &&
              std::is_base_of_v<mV<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CmV<mV<uint32_t>>, "mV must satisfy CmV concept");

#endif  // MILLI_VOLT_HPP