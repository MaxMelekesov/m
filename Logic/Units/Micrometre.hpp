/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MICRO_METRE_HPP
#define MICRO_METRE_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct uM : public Unit<uM<T>, T> {
 public:
  using Unit<uM<T>, T>::Unit;
};

template <typename T>
uM(T) -> uM<T>;

namespace m::ifc {
template <typename T>
concept CuM = requires { typename T::type; } &&
              std::is_base_of_v<uM<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CuM<uM<uint32_t>>, "uM must satisfy CuM concept");

#endif  // MICRO_METRE_HPP