/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MINUTE_HPP
#define MINUTE_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Min : public Unit<Min<T>, T> {
 public:
  using Unit<Min<T>, T>::Unit;
};

template <typename T>
Min(T) -> Min<T>;

namespace m::ifc {
template <typename T>
concept CMin = requires { typename T::type; } &&
               std::is_base_of_v<Min<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CMin<Min<uint32_t>>, "Min must satisfy CMin concept");

#endif  // MINUTE_HPP