/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CTIME_HPP
#define CTIME_HPP
#include <concepts>

namespace m::c {

template <typename T, typename TimeUnit>
concept CTime = requires(T t, TimeUnit value) {
  { t.delay(value) } -> std::same_as<void>;
  { t.getTick() } -> std::same_as<TimeUnit>;
  { t.getDiff(value) } -> std::same_as<TimeUnit>;
};

}  // namespace m::c

#endif  // CTIME_HPP