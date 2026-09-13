/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef HZ_HPP
#define HZ_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Hz : public Unit<Hz<T>, T> {
 public:
  using Unit<Hz<T>, T>::Unit;
};

template <typename T>
Hz(T) -> Hz<T>;

namespace m::ifc {
template <typename T>
concept CHz = requires { typename T::type; } &&
              std::is_base_of_v<Hz<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CHz<Hz<uint32_t>>, "Hz must satisfy CHz concept");

#endif  // HZ_HPP
