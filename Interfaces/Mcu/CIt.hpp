/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef C_IT_HPP
#define C_IT_HPP

#include <concepts>
#include <functional>
#include <utility>

namespace m::c::mcu {

template <typename T>
concept CIt =
    requires(T it, std::function<void()>&& cb) {
      { it.setCallback(std::move(cb)) };
    } &&
    std::is_same_v<decltype(&T::setCallback),
                   void (T::*)(std::function<void()>&&)> &&
    requires(T it) {
      { it.start() } -> std::same_as<bool>;
      { it.running() } -> std::same_as<bool>;
      { it.stop() } -> std::same_as<bool>;
    };

}  // namespace m::c::mcu

#endif  // C_IT_HPP
