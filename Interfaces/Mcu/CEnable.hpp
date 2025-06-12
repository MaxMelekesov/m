/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef C_ENABLE_HPP
#define C_ENABLE_HPP

#include <concepts>

namespace m::c::mcu {

template <typename T>
concept CEnable = requires(T t) {
  { t.enable() } -> std::same_as<bool>;
  { t.isEnabled() } -> std::same_as<bool>;
  { t.disable() } -> std::same_as<bool>;
};

}  // namespace m::c::mcu

#endif  // C_ENABLE_HPP
