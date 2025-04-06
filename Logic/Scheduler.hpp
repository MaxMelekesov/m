/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <array>
#include <coroutine>
#include <utility>

namespace m {

// Класс Task для корутин
struct Task {
  struct promise_type {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}

    bool suspend_requested = false;
  };

  std::coroutine_handle<promise_type> coro;

  explicit Task(std::coroutine_handle<promise_type> h) : coro(h) {}

  ~Task() {
    if (coro) coro.destroy();
  }

  bool resume() {
    if (!coro.done()) {
      coro.resume();
      return true;
    }
    return false;
  }

  void suspend() {
    if (coro && !coro.done()) {
      coro.promise().suspend_requested = true;
    }
  }

  bool is_suspended() const { return coro.promise().suspend_requested; }
};

// Awaitable для добровольной приостановки
struct YieldAwaiter {
  bool await_ready() const { return false; }

  template <typename PromiseType>
  void await_suspend(std::coroutine_handle<PromiseType> handle) {
    auto& promise = handle.promise();
    promise.suspend_requested = true;
  }

  void await_resume() const {}
};

YieldAwaiter yield_task() { return {}; }

template <std::size_t Max_Tasks>
class Scheduler {
 public:
  Scheduler(std::array<m::Task, Max_Tasks>&& tasks)
      : tasks_(std::move(tasks)) {}

  void handle() {
    if (task_count_ == 0) return;

    if (!current_task_valid() || current_task().is_suspended() ||
        current_task().coro.done()) {
      runNext();
    }
  }

  void switchCoroutine() {
    if (current_task_valid()) {
      auto& current = current_task();
      if (!current.coro.done()) {
        current.suspend();
      }
    }
  }

 private:
  std::array<Task, Max_Tasks> tasks_;
  std::size_t task_count_;
  std::size_t current_index_ = 0;

  bool current_task_valid() const {
    return task_count_ > 0 && current_index_ < task_count_;
  }

  Task& current_task() { return tasks_[current_index_]; }

  void runNext() {
    current_index_ = (current_index_ + 1) % task_count_;
    tasks_[current_index_].resume();
  }
};

}  // namespace m
#endif  // SCHEDULER_H