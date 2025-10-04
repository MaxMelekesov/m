/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SECOND_HPP
#define SECOND_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Sec : public Unit<Sec<T>, T> {
 public:
  using Unit<Sec<T>, T>::Unit;
};

namespace m::ifc {
template <typename T>
concept CSec = requires { typename T::type; } &&
               std::is_base_of_v<Sec<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CSec<Sec<uint32_t>>, "Sec must satisfy CSec concept");

#endif  // SECOND_HPP