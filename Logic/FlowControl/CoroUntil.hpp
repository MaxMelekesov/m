/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_UNTIL_HPP
#define CORO_UNTIL_HPP

#include <CoroScheduler.hpp>
#include <CoroYield.hpp>
#include <coroutine>

namespace m {

// Check predicate until it returns true.
template <typename Predicate>
[[nodiscard]] inline auto coroUntil(Predicate&& pred) -> Task<void> {
  while (!pred()) {
    co_await m::coroYield();
  }
}

// Wait while predicate returns true.
template <typename Predicate>
[[nodiscard]] inline auto coroWhile(Predicate&& pred) -> Task<void> {
  while (pred()) {
    co_await m::coroYield();
  }
}

}  // namespace m

#endif  // CORO_UNTIL_HPP
