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

namespace m {

template <typename ResultType = void>
class [[deprecated("use CoroScheduler instead")]] CoroutineTask {
 public:
  struct promise_type {
    ResultType result_ = ResultType{};
    CoroutineTask get_return_object() {
      return CoroutineTask{
          std::coroutine_handle<promise_type>::from_promise(*this)};
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

  bool done() {
    if (!handle_) return true;
    return handle_.done();
  }

  bool resume() {
    if (!handle_) return false;
    if (!handle_.done()) {
      handle_();
      return true;
    }
    return false;
  }

  ResultType result() {
    return (handle_ && handle_.done()) ? handle_.promise().result_
                                       : ResultType{0};
  }

 protected:
  Handle handle_;
};

template <>
class [[deprecated("use CoroScheduler instead")]] CoroutineTask<void> {
 public:
  struct promise_type {
    CoroutineTask get_return_object() {
      return CoroutineTask{
          std::coroutine_handle<promise_type>::from_promise(*this)};
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

  bool done() {
    if (!handle_) return true;
    return handle_.done();
  }

  bool resume() {
    if (!handle_) return false;
    if (!handle_.done()) {
      handle_();
      return true;
    }
    return false;
  }

 protected:
  Handle handle_;
};

}  // namespace m

#endif  // COROUTINETASK_HPP