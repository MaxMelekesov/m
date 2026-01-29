/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef BPS_HPP
#define BPS_HPP

#include <Unit.hpp>
#include <type_traits>

template <typename T>
struct Bps : public Unit<Bps<T>, T> {
 public:
  using Unit<Bps<T>, T>::Unit;
};

template <typename T>
Bps(T) -> Bps<T>;

namespace m::ifc {
template <typename T>
concept CBps = requires { typename T::type; } &&
               std::is_base_of_v<Unit<T, typename T::type>, T>;
}  // namespace m::ifc

#endif  // BPS_HPP