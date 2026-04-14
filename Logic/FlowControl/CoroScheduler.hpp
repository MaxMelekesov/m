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

#include <array>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

#ifndef M_CORO_POOL_ENABLE_STATS
#define M_CORO_POOL_ENABLE_STATS 1
#endif

namespace m {

// Configuration traits — specialize before first use to override globally:
//   namespace m { template<> struct CoroTraits<> { ... }; }
template <typename Tag = void>
struct CoroTraits {
  static constexpr std::size_t slot_size = 384;
  static constexpr std::size_t capacity = 16;
};

namespace detail {
template <std::size_t SlotSize, std::size_t Capacity>
class CoroFramePool {
  static_assert(SlotSize > 0, "SlotSize must be > 0");
  static_assert(Capacity > 0, "Capacity must be > 0");
  static_assert(Capacity <= std::numeric_limits<std::uint16_t>::max() - 1,
                "Capacity too large for uint16 slot map");

 public:
  struct Stats {
    std::size_t used_slots{0};
    std::size_t peak_used_bytes{0};
    std::size_t peak_used_slots{0};
    std::size_t peak_requested_size{0};
    std::size_t peak_requested_slots{0};
  };

  using OomCallback = void (*)(std::size_t requested_size,
                               std::size_t requested_slots,
                               std::size_t used_slots,
                               std::size_t capacity_slots);

  static CoroFramePool& getInstance() { return instance_; }

  void setOomCallback(OomCallback callback) { oom_callback_ = callback; }

  [[nodiscard]] void* allocate(std::size_t requested_size) noexcept {
    if constexpr (Compile_Stats_Enabled) {
      if (requested_size > stats_.peak_requested_size) {
        stats_.peak_requested_size = requested_size;
      }
    }

    if (requested_size == 0) {
      notifyOom(requested_size, 0);
      return nullptr;
    }

    const std::size_t requested_slots =
        (requested_size + SlotSize - 1) / SlotSize;
    if constexpr (Compile_Stats_Enabled) {
      if (requested_slots > stats_.peak_requested_slots) {
        stats_.peak_requested_slots = requested_slots;
      }
    }

    if (requested_slots > Capacity) {
      notifyOom(requested_size, requested_slots);
      return nullptr;
    }

    std::size_t run_length = 0;
    std::size_t run_start = 0;
    for (std::size_t i = free_hint_; i < Capacity; ++i) {
      if (slot_map_[i] == Free_Slot) {
        if (run_length == 0) {
          run_start = i;
        }
        ++run_length;
        if (run_length >= requested_slots) {
          slot_map_[run_start] = static_cast<std::uint16_t>(requested_slots);
          for (std::size_t j = 1; j < requested_slots; ++j) {
            slot_map_[run_start + j] = Continuation_Slot;
          }

          stats_.used_slots += requested_slots;
          if constexpr (Compile_Stats_Enabled) {
            if (stats_.used_slots > stats_.peak_used_slots) {
              stats_.peak_used_slots = stats_.used_slots;
              stats_.peak_used_bytes = stats_.peak_used_slots * SlotSize;
            }
          }

          free_hint_ = run_start + requested_slots;
          return slotAddress(run_start);
        }
      } else {
        run_length = 0;
      }
    }

    notifyOom(requested_size, requested_slots);
    return nullptr;
  }

  void deallocate(void* p, std::size_t /*requested_size*/ = 0) noexcept {
    if (!p) return;

    auto* ptr = static_cast<std::byte*>(p);
    auto* begin = buffer_.data();
    if (ptr < begin || ptr >= begin + buffer_.size()) return;

    const std::size_t offset = static_cast<std::size_t>(ptr - begin);
    if ((offset % SlotSize) != 0U) return;

    const std::size_t slot_index = offset / SlotSize;

    const std::uint16_t block_slots = slot_map_[slot_index];
    if (block_slots == Free_Slot || block_slots == Continuation_Slot) return;

    const std::size_t block_size = static_cast<std::size_t>(block_slots);

    for (std::size_t i = 0; i < block_size; ++i) {
      slot_map_[slot_index + i] = Free_Slot;
    }

    if (stats_.used_slots >= block_size) {
      stats_.used_slots -= block_size;
    } else {
      stats_.used_slots = 0;
    }

    if (slot_index < free_hint_) {
      free_hint_ = slot_index;
    }
  }

  [[nodiscard]] const Stats& stats() const {
    if constexpr (!Compile_Stats_Enabled) {
      static const Stats kEmptyStats{};
      return kEmptyStats;
    }
    return stats_;
  }

 private:
  static constexpr bool Compile_Stats_Enabled = (M_CORO_POOL_ENABLE_STATS != 0);

  static constexpr std::uint16_t Free_Slot = 0U;
  static constexpr std::uint16_t Continuation_Slot =
      std::numeric_limits<std::uint16_t>::max();

  CoroFramePool() { slot_map_.fill(Free_Slot); }

  static CoroFramePool instance_;

  [[nodiscard]] void* slotAddress(std::size_t index) {
    return static_cast<void*>(buffer_.data() + index * SlotSize);
  }

  void notifyOom(std::size_t requested_size,
                 std::size_t requested_slots) const {
    if (oom_callback_) {
      oom_callback_(requested_size, requested_slots, stats_.used_slots,
                    Capacity);
    }
  }

  alignas(
      std::max_align_t) std::array<std::byte, SlotSize * Capacity> buffer_{};
  std::array<std::uint16_t, Capacity> slot_map_{};

  std::size_t free_hint_{0};
  Stats stats_{};

  OomCallback oom_callback_{nullptr};
};

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

namespace detail {

template <typename Config>
class CoroSchedulerImpl {
  using Pool = CoroFramePool<Config::slot_size, Config::capacity>;

 public:
  using CoroMemoryStats = typename Pool::Stats;
  using OomCallback = typename Pool::OomCallback;

  void setCoroutineOomCallback(OomCallback callback) {
    Pool::getInstance().setOomCallback(callback);
  }

  const CoroMemoryStats& coroutineMemoryStats() {
    return Pool::getInstance().stats();
  }

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

  static CoroSchedulerImpl& getInstance() { return instance_; }

  void enqueue(std::coroutine_handle<> h) {
    if (!h) return;
    auto base = std::coroutine_handle<PromiseBase>::from_address(h.address());

    if (base.promise().scheduled_ || base.promise().waiting_for_nested_) {
      return;
    }
    base.promise().scheduled_ = true;
    fifo_queue_.push(base);
  }

 private:
  FifoQueue fifo_queue_;

  CoroSchedulerImpl() = default;

  static CoroSchedulerImpl instance_;
};

template <typename T, typename Config>
class TaskImpl {
  using Pool = CoroFramePool<Config::slot_size, Config::capacity>;
  using Sched = CoroSchedulerImpl<Config>;

 public:
  struct promise_type : PromiseBase {
    T result_{};

    void* operator new(std::size_t sz) noexcept {
      return Pool::getInstance().allocate(sz);
    }

    void operator delete(void* p, std::size_t sz) noexcept {
      Pool::getInstance().deallocate(p, sz);
    }

    void operator delete(void* p) noexcept {
      Pool::getInstance().deallocate(p);
    }

    static TaskImpl get_return_object_on_allocation_failure() {
      return TaskImpl{std::coroutine_handle<promise_type>{}};
    }

    TaskImpl get_return_object() {
      return TaskImpl{std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          if (h.promise().continuation_) {
            auto cont = h.promise().continuation_;
            h.promise().continuation_ = nullptr;

            auto cont_base = std::coroutine_handle<PromiseBase>::from_address(
                cont.address());
            cont_base.promise().waiting_for_nested_ = false;
            Sched::getInstance().enqueue(cont);
          }
        }
        void await_resume() noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_value(T value) { result_ = std::move(value); }
    void unhandled_exception() {}
  };

  explicit TaskImpl(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~TaskImpl() {
    if (coro_) {
      if (!coro_.done()) {
        coro_.promise().detached_ = true;
      } else if (!coro_.promise().detached_) {
        coro_.destroy();
      }
    }
  }
  TaskImpl(TaskImpl&& other) noexcept
      : coro_(std::exchange(other.coro_, nullptr)) {}
  TaskImpl& operator=(TaskImpl&&) = delete;

  [[nodiscard]] auto operator co_await() & {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<PromiseBase>::from_address(caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          Sched::getInstance().enqueue(coro);
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
            std::coroutine_handle<PromiseBase>::from_address(caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          Sched::getInstance().enqueue(coro);
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

template <typename Config>
class TaskImpl<void, Config> {
  using Pool = CoroFramePool<Config::slot_size, Config::capacity>;
  using Sched = CoroSchedulerImpl<Config>;

 public:
  struct promise_type : PromiseBase {
    void* operator new(std::size_t sz) noexcept {
      return Pool::getInstance().allocate(sz);
    }

    void operator delete(void* p, std::size_t sz) noexcept {
      Pool::getInstance().deallocate(p, sz);
    }

    void operator delete(void* p) noexcept {
      Pool::getInstance().deallocate(p);
    }

    static TaskImpl get_return_object_on_allocation_failure() {
      return TaskImpl{std::coroutine_handle<promise_type>{}};
    }

    TaskImpl get_return_object() {
      return TaskImpl{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_never initial_suspend() { return {}; }

    auto final_suspend() noexcept {
      struct FinalAwaiter {
        bool await_ready() noexcept { return false; }

        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          if (h.promise().continuation_) {
            auto cont = h.promise().continuation_;
            h.promise().continuation_ = nullptr;
            auto cont_base = std::coroutine_handle<PromiseBase>::from_address(
                cont.address());
            cont_base.promise().waiting_for_nested_ = false;
            Sched::getInstance().enqueue(cont);
          }
        }

        void await_resume() noexcept {}
      };
      return FinalAwaiter{};
    }

    void return_void() {}
    void unhandled_exception() {}
  };

  explicit TaskImpl(std::coroutine_handle<promise_type> h) : coro_(h) {}
  ~TaskImpl() {
    if (coro_) {
      if (!coro_.done()) {
        coro_.promise().detached_ = true;
      } else if (!coro_.promise().detached_) {
        coro_.destroy();
      }
    }
  }
  TaskImpl(TaskImpl&& other) noexcept
      : coro_(std::exchange(other.coro_, nullptr)) {}
  TaskImpl& operator=(TaskImpl&&) = delete;

  [[nodiscard]] auto operator co_await() & {
    struct Awaiter {
      std::coroutine_handle<promise_type> coro;

      bool await_ready() { return !coro || coro.done(); }

      bool await_suspend(std::coroutine_handle<> caller) {
        if (coro.done()) {
          return false;
        }
        auto caller_base =
            std::coroutine_handle<PromiseBase>::from_address(caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          Sched::getInstance().enqueue(coro);
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
            std::coroutine_handle<PromiseBase>::from_address(caller.address());
        caller_base.promise().waiting_for_nested_ = true;
        coro.promise().continuation_ = caller;
        if (!coro.promise().scheduled_) {
          Sched::getInstance().enqueue(coro);
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
}  // namespace detail

using CoroScheduler = detail::CoroSchedulerImpl<CoroTraits<>>;

template <typename T>
using Task = detail::TaskImpl<T, CoroTraits<>>;

}  // namespace m

template <typename Config>
m::detail::CoroSchedulerImpl<Config>
    m::detail::CoroSchedulerImpl<Config>::instance_;

template <std::size_t S, std::size_t C>
m::detail::CoroFramePool<S, C> m::detail::CoroFramePool<S, C>::instance_;

#endif  // CORO_SCHEDULER_HPP