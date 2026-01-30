/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_MUTEX_HPP
#define CORO_MUTEX_HPP

#include <CoroScheduler.hpp>

namespace m {

class CoroMutex {
 public:
  class Guard {
   public:
    explicit Guard(CoroMutex& mutex) : mutex_(&mutex) {}
    ~Guard() {
      if (mutex_) {
        mutex_->locked_ = false;
      }
    }

    void unlock() {
      if (mutex_) {
        mutex_->locked_ = false;
        mutex_ = nullptr;
      }
    }

    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&& other) : mutex_(other.mutex_) { other.mutex_ = nullptr; }

   private:
    CoroMutex* mutex_;
  };

  CoroMutex() = default;

  [[nodiscard]] auto lock() {
    struct Awaiter {
      CoroMutex& m;

      bool await_ready() {
        if (!m.locked_) {
          m.locked_ = true;
          return true;
        }
        return false;
      }

      void await_suspend(std::coroutine_handle<> h) {
        CoroScheduler::getInstance().enqueueGlobal(h);
      }

      Guard await_resume() { return Guard{m}; }
    };

    return Awaiter{*this};
  }

 private:
  bool locked_ = false;
};

}  // namespace m

#endif  // CORO_MUTEX_HPP