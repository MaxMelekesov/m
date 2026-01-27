/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_SCHEDULER_HPP
#define CORO_SCHEDULER_HPP

#include <atomic>
#include <coroutine>
#include <type_traits>
#include <utility>

namespace m {
struct CoroSchedulerPromiseBase {
  std::coroutine_handle<CoroSchedulerPromiseBase> next_ready_{nullptr};
};

template <typename T>
class Task {
 public:
  struct promise_type : CoroSchedulerPromiseBase {
    T result_{};

    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(T value) { result_ = value; }
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~Task() {
    if (coro_) coro_.destroy();
  }
  Task(Task&& other) : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() {
    struct Awaiter {
      Task& task;

      bool await_ready() { return false; }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
        return task.coro_;
      }
      T await_resume() {
        T result = task.coro_.promise().result_;
        // task.coro_.destroy();
        // task.coro_ = nullptr;
        return result;
      }
    };
    return Awaiter{*this};
  }

 private:
  std::coroutine_handle<promise_type> coro_;
};

template <>
class Task<void> {
 public:
  struct promise_type : CoroSchedulerPromiseBase {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~Task() {
    if (coro_) coro_.destroy();
  }
  Task(Task&& other) : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() {
    struct Awaiter {
      Task& task;

      bool await_ready() { return false; }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
        return task.coro_;
      }
      void await_resume() {
        // task.coro_.destroy();
        // task.coro_ = nullptr;
      }
    };
    return Awaiter{*this};
  }

 private:
  std::coroutine_handle<promise_type> coro_;
};

class CoroScheduler {
 public:
  [[nodiscard]] static auto yield() {
    struct Awaiter {
      bool await_ready() { return false; }
      void await_suspend(std::coroutine_handle<> h) {
        getInstance().enqueue(h);
      }
      void await_resume() {}
    };
    return Awaiter{};
  }

  template <typename Predicate>
  [[nodiscard]] static auto until(Predicate&& pred) {
    struct Awaiter {
      std::decay_t<Predicate> pred_;
      bool await_ready() { return !pred_(); }
      void await_suspend(std::coroutine_handle<> h) {
        getInstance().enqueue(h);
      }
      void await_resume() {}
    };
    return Awaiter{std::forward<Predicate>(pred)};
  }

  static void resumeFromIsr(std::coroutine_handle<> h) {
    if (h) getInstance().enqueue(h);
  }

  void handle() {
    auto head = ready_list_.exchange(nullptr, std::memory_order_acquire);
    while (head) {
      auto next = head.promise().next_ready_;
      head.resume();
      head = next;
    }
  }

  static CoroScheduler& getInstance() {
    static CoroScheduler sched;
    return sched;
  }

 private:
  std::atomic<std::coroutine_handle<CoroSchedulerPromiseBase>> ready_list_{
      nullptr};

  CoroScheduler() = default;

  void enqueue(std::coroutine_handle<> h) {
    auto base = std::coroutine_handle<CoroSchedulerPromiseBase>::from_address(
        h.address());
    auto head = ready_list_.load(std::memory_order_relaxed);
    do {
      base.promise().next_ready_ = head;
    } while (!ready_list_.compare_exchange_weak(
        head, base, std::memory_order_release, std::memory_order_relaxed));
  }
};
}  // namespace m

#endif  // CORO_SCHEDULER_HPP