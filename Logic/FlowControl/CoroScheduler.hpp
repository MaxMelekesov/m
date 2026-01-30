/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
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
  std::coroutine_handle<> continuation_{nullptr};
  bool scheduled_{false};
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
      head.promise().scheduled_ = false;
      if (!head.done()) {
        head.resume();
      }
      head = next;
    }
  }

  static CoroScheduler& getInstance() {
    static CoroScheduler sched;
    return sched;
  }

  void enqueue(std::coroutine_handle<> h) {
    auto base = std::coroutine_handle<CoroSchedulerPromiseBase>::from_address(
        h.address());
    if (base.promise().scheduled_) {
      return;
    }
    base.promise().scheduled_ = true;
    auto head = ready_list_.load(std::memory_order_relaxed);
    do {
      base.promise().next_ready_ = head;
    } while (!ready_list_.compare_exchange_weak(
        head, base, std::memory_order_release, std::memory_order_relaxed));
  }

 private:
  std::atomic<std::coroutine_handle<CoroSchedulerPromiseBase>> ready_list_{
      nullptr};

  CoroScheduler() = default;
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

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          if (h.promise().continuation_) {
            CoroScheduler::getInstance().enqueue(h.promise().continuation_);
          }
        }
        void await_resume() noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_value(T value) { result_ = value; }
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~Task() {
    if (coro_) coro_.destroy();
  }
  Task(Task&& other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() && {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro; }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        coro.promise().continuation_ = caller;
        CoroScheduler::getInstance().enqueue(coro);
        return true;
      }

      T await_resume() { return coro.promise().result_; }
    };
    return Awaiter{std::exchange(coro_, nullptr)};
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

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }

        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          if (h.promise().continuation_) {
            CoroScheduler::getInstance().enqueue(h.promise().continuation_);
          }
        }

        void await_resume() noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_void() {}
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~Task() {
    if (coro_) coro_.destroy();
  }
  Task(Task&& other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() && {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro; }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        coro.promise().continuation_ = caller;
        CoroScheduler::getInstance().enqueue(coro);
        return true;
      }

      void await_resume() {}
    };
    return Awaiter{std::exchange(coro_, nullptr)};
  }

 private:
  std::coroutine_handle<promise_type> coro_;
};
}  // namespace m

#endif  // CORO_SCHEDULER_HPP