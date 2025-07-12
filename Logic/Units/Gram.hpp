/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef GRAM_HPP
#define GRAM_HPP

#include <Unit.hpp>
#include <type_traits>

template <typename T>
struct Gram : public Unit<Gram<T>, T> {
 public:
  using Unit<Gram<T>, T>::Unit;
};

namespace m::ifc {
template <typename T>
concept CGram = requires { typename T::type; } &&
                std::is_base_of_v<Unit<T, typename T::type>, T>;
}  // namespace m::ifc

#endif  // GRAM_HPP