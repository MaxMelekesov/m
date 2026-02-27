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
#include <utility>

namespace m {
namespace detail {
struct PromiseBase {
  std::coroutine_handle<PromiseBase> next_ready_{nullptr};
  std::coroutine_handle<> continuation_{nullptr};
  bool scheduled_{false};
  bool detached_{false};
  bool waiting_for_nested_{false};
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
    while (!fifo_queue_.empty()) {
      auto head = fifo_queue_.pop();
      if (head) {
        head.promise().scheduled_ = false;
        if (!head.done()) {
          head.resume();
        }
        if (head.promise().detached_ && head.done()) {
          head.destroy();
        }
      }
    }
  }

  static CoroScheduler& getInstance() {
    static CoroScheduler sched;
    return sched;
  }

  void enqueue(std::coroutine_handle<> h) {
    if (!h) return;
    auto base =
        std::coroutine_handle<detail::PromiseBase>::from_address(h.address());

    if (base.promise().scheduled_ || base.promise().waiting_for_nested_) {
      return;
    }
    base.promise().scheduled_ = true;
    fifo_queue_.push(base);
  }

 private:
  detail::FifoQueue fifo_queue_;

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
            auto cont = h.promise().continuation_;
            h.promise().continuation_ = nullptr;

            auto cont_base =
                std::coroutine_handle<detail::PromiseBase>::from_address(
                    cont.address());
            cont_base.promise().waiting_for_nested_ = false;
            CoroScheduler::getInstance().enqueue(cont);
          }
        }
        void await_resume() noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_value(T value) { result_ = std::move(value); }
    void unhandled_exception() {}
  };

  explicit Task(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~Task() {
    if (coro_) {
      if (!coro_.done()) {
        coro_.promise().detached_ = true;
      } else if (!coro_.promise().detached_) {
        coro_.destroy();
      }
    }
  }
  Task(Task&& other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() & {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<detail::PromiseBase>::from_address(
                caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          CoroScheduler::getInstance().enqueue(coro);
        }
        return true;
      }

      T await_resume() {
        if (!coro) {
          return T{};
        }
        return std::move(coro.promise().result_);
      }

      ~Awaiter() = default;
    };
    return Awaiter{coro_};
  }

  [[nodiscard]] auto operator co_await() && {
    auto coro = std::exchange(coro_, nullptr);
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<detail::PromiseBase>::from_address(
                caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          CoroScheduler::getInstance().enqueue(coro);
        }
        return true;
      }

      T await_resume() {
        if (!coro) {
          return T{};
        }
        T result = std::move(coro.promise().result_);
        coro.destroy();
        return result;
      }

      ~Awaiter() = default;
    };
    return Awaiter{coro};
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
            auto cont = h.promise().continuation_;
            h.promise().continuation_ = nullptr;
            auto cont_base =
                std::coroutine_handle<detail::PromiseBase>::from_address(
                    cont.address());
            cont_base.promise().waiting_for_nested_ = false;
            CoroScheduler::getInstance().enqueue(cont);
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
    if (coro_) {
      if (!coro_.done()) {
        coro_.promise().detached_ = true;
      } else if (!coro_.promise().detached_) {
        coro_.destroy();
      }
    }
  }
  Task(Task&& other) noexcept : coro_(std::exchange(other.coro_, nullptr)) {}
  Task& operator=(Task&&) = delete;

  [[nodiscard]] auto operator co_await() & {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<detail::PromiseBase>::from_address(
                caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          CoroScheduler::getInstance().enqueue(coro);
        }
        return true;
      }

      void await_resume() {}

      ~Awaiter() = default;
    };
    return Awaiter{coro_};
  }

  [[nodiscard]] auto operator co_await() && {
    auto coro = std::exchange(coro_, nullptr);
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<detail::PromiseBase>::from_address(
                caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          CoroScheduler::getInstance().enqueue(coro);
        }
        return true;
      }

      void await_resume() {
        if (coro) {
          coro.destroy();
        }
      }

      ~Awaiter() = default;
    };
    return Awaiter{coro};
  }

 private:
  std::coroutine_handle<promise_type> coro_;
};
}  // namespace m

#endif  // CORO_SCHEDULER_HPP