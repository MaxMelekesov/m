/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_YIELD_HPP
#define CORO_YIELD_HPP

#include <CoroScheduler.hpp>

namespace m {

[[nodiscard]] inline auto coroYield() {
  struct Awaiter {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) {
      CoroScheduler::getInstance().enqueue(h);
    }
    void await_resume() {}
  };
  return Awaiter{};
}

}  // namespace m

#endif  // CORO_YIELD_HPP
