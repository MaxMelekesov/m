/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef HOUR_HPP
#define HOUR_HPP

#include <Unit.hpp>
#include <cstdint>
#include <type_traits>

template <typename T>
struct Hour : public Unit<Hour<T>, T> {
 public:
  using Unit<Hour<T>, T>::Unit;
};

template <typename T>
Hour(T) -> Hour<T>;

namespace m::ifc {
template <typename T>
concept CHour = requires { typename T::type; } &&
                std::is_base_of_v<Hour<typename T::type>, T>;
}  // namespace m::ifc

static_assert(m::ifc::CHour<Hour<uint32_t>>, "Hour must satisfy CHour concept");

#endif  // HOUR_HPP