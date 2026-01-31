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

#include <coroutine>
#include <type_traits>
#include <utility>

namespace m {
namespace detail {
struct PromiseBase {
  std::coroutine_handle<PromiseBase> next_ready_{nullptr};
  std::coroutine_handle<> continuation_{nullptr};
  bool scheduled_{false};
};

class LifoQueue {
 public:
  using Handle = std::coroutine_handle<PromiseBase>;

  bool empty() const { return !head_; }

  void push(Handle h) {
    h.promise().next_ready_ = head_;
    head_ = h;
  }

  Handle pop() {
    auto head = head_;
    if (head) {
      head_ = head.promise().next_ready_;
    }
    return head;
  }

 private:
  Handle head_{nullptr};
};

class FifoQueue {
 public:
  using Handle = std::coroutine_handle<PromiseBase>;

  bool empty() const { return !head_; }

  void push(Handle h) {
    h.promise().next_ready_ = nullptr;
    if (tail_) {
      tail_.promise().next_ready_ = h;
    } else {
      head_ = h;
    }
    tail_ = h;
  }

  Handle pop() {
    auto head = head_;
    if (head) {
      head_ = head.promise().next_ready_;
      if (!head_) {
        tail_ = nullptr;
      }
    }
    return head;
  }

 private:
  Handle head_{nullptr};
  Handle tail_{nullptr};
};
}  // namespace detail

class CoroScheduler {
 public:
  void handle() {
    while (!lifo_queue_.empty() || !fifo_queue_.empty()) {
      std::coroutine_handle<detail::PromiseBase> head{nullptr};
      const bool lifo_empty = lifo_queue_.empty();
      const bool fifo_empty = fifo_queue_.empty();
      if (!lifo_empty && !fifo_empty) {
        prefer_lifo_ = !prefer_lifo_;
      }
      if (prefer_lifo_) {
        head = lifo_empty ? fifo_queue_.pop() : lifo_queue_.pop();
      } else {
        head = fifo_empty ? lifo_queue_.pop() : fifo_queue_.pop();
      }
      head.promise().scheduled_ = false;
      if (!head.done()) {
        head.resume();
      }
    }
  }

  static CoroScheduler& getInstance() {
    static CoroScheduler sched;
    return sched;
  }

  void enqueueGlobal(std::coroutine_handle<> h) {
    auto base =
        std::coroutine_handle<detail::PromiseBase>::from_address(h.address());
    if (base.promise().scheduled_) {
      return;
    }
    base.promise().scheduled_ = true;
    fifo_queue_.push(base);
  }

  void enqueueLocal(std::coroutine_handle<> h) {
    auto base =
        std::coroutine_handle<detail::PromiseBase>::from_address(h.address());
    if (base.promise().scheduled_) {
      return;
    }
    base.promise().scheduled_ = true;
    lifo_queue_.push(base);
  }

 private:
  detail::LifoQueue lifo_queue_;
  detail::FifoQueue fifo_queue_;
  bool prefer_lifo_{true};

  CoroScheduler() = default;
};

template <typename T>
class Task {
 public:
  struct promise_type : detail::PromiseBase {
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
            CoroScheduler::getInstance().enqueueLocal(
                h.promise().continuation_);
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
        CoroScheduler::getInstance().enqueueLocal(coro);
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
  struct promise_type : detail::PromiseBase {
    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_never initial_suspend() { return {}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }

        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          if (h.promise().continuation_) {
            CoroScheduler::getInstance().enqueueLocal(
                h.promise().continuation_);
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
        CoroScheduler::getInstance().enqueueLocal(coro);
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