/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef COROUTINETASK_HPP
#define COROUTINETASK_HPP

#include <coroutine>
#include <exception>

namespace m {

template <typename Derived, typename ResultType = void>
class CoroutineTask {
 public:
  struct promise_type {
    ResultType result_ = ResultType{};
    Derived get_return_object() {
      return Derived{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_value(ResultType res) { result_ = res; }
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit CoroutineTask(Handle h) : handle_(h) {}

  CoroutineTask(const CoroutineTask&) = delete;
  CoroutineTask& operator=(const CoroutineTask&) = delete;

  CoroutineTask(CoroutineTask&& other) : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  CoroutineTask& operator=(CoroutineTask&& other) {
    if (this != &other) {
      if (handle_) handle_.destroy();
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~CoroutineTask() {
    if (handle_) handle_.destroy();
  }

  bool done() { return handle_ && handle_.done(); }

  bool resume() {
    if (!done()) return false;
    handle_();
    return !handle_.done();
  }

  ResultType result() {
    return done() ? handle_.promise().result_ : ResultType{0};
  }

 protected:
  Handle handle_;
};

template <typename Derived>
class CoroutineTask<Derived, void> {
 public:
  struct promise_type {
    Derived get_return_object() {
      return Derived{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_void() {}
  };

  using Handle = std::coroutine_handle<promise_type>;

  explicit CoroutineTask(Handle h) : handle_(h) {}

  CoroutineTask(const CoroutineTask&) = delete;
  CoroutineTask& operator=(const CoroutineTask&) = delete;

  CoroutineTask(CoroutineTask&& other) : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  CoroutineTask& operator=(CoroutineTask&& other) {
    if (this != &other) {
      if (handle_) handle_.destroy();
      handle_ = other.handle_;
      other.handle_ = nullptr;
    }
    return *this;
  }

  ~CoroutineTask() {
    if (handle_) handle_.destroy();
  }

  bool done() { return handle_ && handle_.done(); }

  bool resume() {
    if (!done()) return false;
    handle_();
    return !handle_.done();
  }

 protected:
  Handle handle_;
};

}  // namespace m

#endif  // COROUTINETASK_HPP