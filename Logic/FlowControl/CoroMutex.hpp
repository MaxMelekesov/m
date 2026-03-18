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
#include <CoroYield.hpp>

namespace m {

class CoroMutex {
 public:
  class Guard {
   public:
    Guard() : mutex_(nullptr) {}
    explicit Guard(CoroMutex& mutex) : mutex_(&mutex) {}
    ~Guard() { unlock(); }

    void unlock() {
      if (mutex_) {
        mutex_->unlockInternal();
        mutex_ = nullptr;
      }
    }

    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
    Guard(Guard&& other) : mutex_(other.mutex_) { other.mutex_ = nullptr; }
    Guard& operator=(Guard&& other) noexcept {
      if (this != &other) {
        unlock();
        mutex_ = other.mutex_;
        other.mutex_ = nullptr;
      }
      return *this;
    }

   private:
    CoroMutex* mutex_;
  };

  CoroMutex() = default;

  [[nodiscard]] bool isLocked() const { return locked_; }

  [[nodiscard]] auto lock() -> Task<Guard> {
    while (locked_) {
      co_await coroYield();
    }
    locked_ = true;
    co_return Guard{*this};
  }

 private:
  void unlockInternal() {
    if (!locked_) {
      return;
    }
    locked_ = false;
  }

  bool locked_ = false;
};

}  // namespace m

#endif  // CORO_MUTEX_HPP