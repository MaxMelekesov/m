/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in
 * the project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_SCHEDULER_HPP
#define CORO_SCHEDULER_HPP

#include <algorithm>
#include <array>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <type_traits>
#include <utility>

/**
 * CoroScheduler — cooperative scheduler with a frame arena per activity: no
 * heap, frame sizes and concurrency fixed at build time.
 *
 *   m::CoroArena<448> g_homing_arena;   // static frame buffer
 *
 *   // Root coroutine ("activity"): the arena is the first parameter — the
 *   // compiler checks it; the body starts on the first handle().
 *   m::Activity<void> homing(m::Arena& arena, Plant& plant, Clock& clock) {
 *     m::Task<bool> move = moveToLimit(plant, clock);  // starts at once
 *     co_await m::coroDelay(clock, 20);                // own work meanwhile
 *     const bool reached = co_await move;              // join by awaiting it
 *   }
 *
 *   m::Activity<void> homing_task = homing(g_homing_arena, plant, clock);
 *
 *   void loop() {                                 // superloop / SysTick
 *     m::CoroScheduler::getInstance().handle();   // one round, then returns
 *   }
 */

namespace m {

#ifndef M_CORO_ARENA_ENABLE_STATS
#define M_CORO_ARENA_ENABLE_STATS 1  // 0 strips peak()/peakFrames()/frames_
#endif
#ifndef M_CORO_ARENA_TRAPS
#define M_CORO_ARENA_TRAPS 1  // 0 drops the precondition traps
#endif

template <std::size_t Bytes>
class CoroArena;

class Arena;

template <typename T>
class Task;

template <typename T>
class Activity;

enum class FaultCode : std::uint8_t {
  Arena_Overflow,
  Arena_Busy,
  Arena_Misaligned,
  No_Activity,
  Children_Alive,
  Reentrant_Handle,
  Task_Already_Awaited,
};

struct FaultInfo {
  FaultCode code;
  std::size_t need;
  std::size_t have;
};

using Fault_Handler = void (*)(const FaultInfo&);

// Not part of the API: names, layout and behaviour change without notice.

namespace detail::coro {

using Offset = std::size_t;
inline constexpr Offset No_Node = static_cast<Offset>(-1);

// Frames start here; a coroutine local must not ask for more alignment.
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
inline constexpr std::size_t Arena_Alignment =
    std::max(alignof(std::max_align_t),
             static_cast<std::size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__));
#else
inline constexpr std::size_t Arena_Alignment = alignof(std::max_align_t);
#endif

inline constexpr bool Traps_Enabled = (M_CORO_ARENA_TRAPS != 0);
inline Fault_Handler g_handler = nullptr;

[[noreturn]] inline void fault(FaultCode code, std::size_t need,
                               std::size_t have) noexcept {
  if (g_handler != nullptr) g_handler(FaultInfo{code, need, have});
  std::abort();
}

/// Read only by the Children_Alive trap.
template <bool Enabled>
struct Children {
  std::size_t n{0};
  void born() noexcept { ++n; }
  void died() noexcept { --n; }
  [[nodiscard]] std::size_t live() const noexcept { return n; }
};

template <>
struct Children<false> {
  void born() noexcept {}
  void died() noexcept {}
  [[nodiscard]] std::size_t live() const noexcept { return 0; }
};

// Forward declarations: the promise machinery below, and Arena, befriend these.
template <typename T>
struct Value_Promise;
template <typename T>
struct Task_Promise;
template <typename T>
struct Activity_Promise;
template <typename PromiseT>
class Task_Handle;
template <class Awaiter>
struct Slot_Awaiter;
struct Final_Awaiter;

struct PromiseBase {
  explicit PromiseBase(Arena& arena) noexcept : arena_(arena) {}

  template <class Operand>
  auto await_transform(Operand&& operand) {
    if constexpr (requires {
                    std::forward<Operand>(operand).operator co_await();
                  }) {
      using Awaiter =
          decltype(std::forward<Operand>(operand).operator co_await());
      return Slot_Awaiter<Awaiter>{
          *this, std::forward<Operand>(operand).operator co_await()};
    } else {
      return Slot_Awaiter<std::remove_cvref_t<Operand>>{
          *this, std::forward<Operand>(operand)};
    }
  }

  Arena& arena_;
  Offset parent_{No_Node};
  Offset below_{No_Node};
  Offset end_{0};
  [[no_unique_address]] Children<Traps_Enabled> children_{};
  bool owned_{false};

  std::coroutine_handle<PromiseBase> next_ready_{nullptr};
  PromiseBase* continuation_{nullptr};
  bool scheduled_{false};
  bool waiting_for_nested_{false};
};

// Who is running: operator new() and the promise constructor get no caller
// context, so a nested frame learns its arena and its owner from here.
inline PromiseBase* g_slot = nullptr;

void attach(PromiseBase& self) noexcept;

}  // namespace detail::coro

class Arena {
 public:
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;
  Arena(Arena&&) = delete;
  Arena& operator=(Arena&&) = delete;

  [[nodiscard]] std::size_t capacity() const noexcept {
    return storage_.size();
  }
  [[nodiscard]] std::size_t used() const noexcept {
    return (top_ == No_Node) ? 0 : nodeAt(top_).end_;
  }
  [[nodiscard]] std::size_t peak() const noexcept { return peak_; }
  [[nodiscard]] std::size_t peakFrames() const noexcept { return peak_frames_; }

  static constexpr std::size_t Alignment = detail::coro::Arena_Alignment;

 private:
  template <std::size_t>
  friend class CoroArena;

  constexpr explicit Arena(std::span<std::byte> storage) noexcept
      : storage_(storage) {
    if (std::is_constant_evaluated()) {
      return;
    }
    if constexpr (detail::coro::Traps_Enabled) {
      const auto base = reinterpret_cast<std::uintptr_t>(storage_.data());
      if (base % detail::coro::Arena_Alignment != 0) {
        detail::coro::fault(
            FaultCode::Arena_Misaligned, detail::coro::Arena_Alignment,
            static_cast<std::size_t>(base % detail::coro::Arena_Alignment));
      }
    }
  }

  template <typename>
  friend struct detail::coro::Task_Promise;
  template <typename>
  friend struct detail::coro::Activity_Promise;
  template <typename>
  friend class detail::coro::Task_Handle;
  template <class>
  friend struct detail::coro::Slot_Awaiter;
  friend struct detail::coro::Final_Awaiter;
  friend void detail::coro::attach(detail::coro::PromiseBase&) noexcept;

  using Offset = detail::coro::Offset;
  using Node = detail::coro::PromiseBase;
  static constexpr Offset No_Node = detail::coro::No_Node;

  [[nodiscard]] void* allocate(std::size_t bytes) noexcept {
    sweep();
    const std::size_t begin = alignUp(used());
    const std::size_t end = begin + bytes;
    if (end > capacity()) {
      detail::coro::fault(FaultCode::Arena_Overflow, end, capacity());
    }
    // 0 means "nothing pending"; bind() reads the frame end back from here.
    pending_ = end;
    if constexpr (Stats_Enabled) {
      peak_ = std::max(end, peak_);
    }
    return &storage_[begin];
  }

  void release(Offset offset) noexcept {
    for (Offset at = top_; at != No_Node; at = nodeAt(at).below_) {
      if (at == offset) {
        nodeAt(at).owned_ = false;
        break;
      }
    }
    sweep();
  }

  void bind(Node& node) noexcept {
    if constexpr (detail::coro::Traps_Enabled) {
      if (node.parent_ == No_Node && top_ != No_Node) {
        detail::coro::fault(FaultCode::Arena_Busy, 0, 0);
      }
    }
    node.below_ = top_;
    node.end_ =
        pending_ != 0 ? pending_ : (top_ == No_Node ? 0 : nodeAt(top_).end_);
    pending_ = 0;
    top_ = offsetOf(node);
    if constexpr (Stats_Enabled) {
      ++frames_;
      peak_frames_ = std::max(frames_, peak_frames_);
    }
  }

  void sweep() noexcept {
    while (top_ != No_Node) {
      Node& node = nodeAt(top_);
      auto handle = std::coroutine_handle<Node>::from_promise(node);
      if (!handle.done()) {
        break;
      }
      if (node.owned_) {
        const bool creator_done =
            node.parent_ != No_Node && done(nodeAt(node.parent_));
        if (!creator_done) {
          break;
        }
      }
      if (node.continuation_ != nullptr) {
        break;  // a suspended waiter is about to read this frame's value
      }
      top_ = node.below_;
      if constexpr (Stats_Enabled) {
        --frames_;
      }
      handle.destroy();
    }
  }

  [[nodiscard]] static bool done(Node& node) noexcept {
    return std::coroutine_handle<Node>::from_promise(node).done();
  }
  [[nodiscard]] Node& nodeAt(Offset offset) const noexcept {
    return reinterpret_cast<Node&>(storage_[offset]);
  }
  [[nodiscard]] Offset offsetOf(const Node& node) const noexcept {
    const auto* address = reinterpret_cast<const std::byte*>(&node);
    return static_cast<Offset>(address - storage_.data());
  }

  static constexpr bool Stats_Enabled = (M_CORO_ARENA_ENABLE_STATS != 0);

  [[nodiscard]] static constexpr std::size_t alignUp(
      std::size_t value) noexcept {
    return (value + detail::coro::Arena_Alignment - 1U) &
           ~(detail::coro::Arena_Alignment - 1U);
  }

  std::span<std::byte> storage_;
  Offset top_{No_Node};
  std::size_t pending_{0};
  std::size_t peak_{0};
  std::size_t peak_frames_{0};
  std::size_t frames_{0};
};

template <std::size_t Bytes>
class CoroArena {
 public:
  constexpr CoroArena() noexcept : arena_(storage_) {}

  CoroArena(const CoroArena&) = delete;
  CoroArena& operator=(const CoroArena&) = delete;
  CoroArena(CoroArena&&) = delete;
  CoroArena& operator=(CoroArena&&) = delete;

  operator Arena&() noexcept { return arena_; }

  [[nodiscard]] std::size_t capacity() const noexcept {
    return arena_.capacity();
  }
  [[nodiscard]] std::size_t used() const noexcept { return arena_.used(); }
  [[nodiscard]] std::size_t peak() const noexcept { return arena_.peak(); }
  [[nodiscard]] std::size_t peakFrames() const noexcept {
    return arena_.peakFrames();
  }

 private:
  alignas(Arena::Alignment) std::array<std::byte, Bytes> storage_{};
  Arena arena_;
};

class CoroScheduler {
 public:
  /// Where a fault goes before std::abort(); the storage stays with the fault
  /// machinery, so an Arena alone does not pull the scheduler in.
  void setFaultHandler(Fault_Handler handler) noexcept {
    detail::coro::g_handler = handler;
  }

  void handle() {
    if constexpr (detail::coro::Traps_Enabled) {
      if (running_) {
        detail::coro::fault(FaultCode::Reentrant_Handle, 0, 0);
      }
    }
    running_ = true;
    // One round = what is ready right now: the tail remembers that set.
    const Handle stop = queue_.tail_;
    while (queue_.head_ != nullptr) {
      Handle head = queue_.pop();
      detail::coro::PromiseBase& promise = head.promise();
      promise.scheduled_ = false;
      // A queued coroutine is never finished, and a wakeup clears
      // waiting_for_nested_ before enqueueing it.
      detail::coro::g_slot = &promise;
      head.resume();
      if (head == stop) {
        break;
      }
    }
    detail::coro::g_slot = nullptr;
    running_ = false;
  }

  /// For an awaiter of your own: the coroutine to resume in a later round.
  void enqueue(std::coroutine_handle<> handle) noexcept {
    if (!handle) {
      return;
    }
    Handle base = Handle::from_address(handle.address());
    if (base.promise().scheduled_ || base.promise().waiting_for_nested_) {
      return;
    }
    base.promise().scheduled_ = true;
    queue_.push(base);
  }

  static CoroScheduler& getInstance() noexcept {
    static CoroScheduler instance;
    return instance;
  }

  CoroScheduler(const CoroScheduler&) = delete;
  CoroScheduler& operator=(const CoroScheduler&) = delete;

 private:
  CoroScheduler() noexcept = default;

  using Handle = std::coroutine_handle<detail::coro::PromiseBase>;

  struct Queue {
    void push(Handle handle) noexcept {
      handle.promise().next_ready_ = nullptr;
      if (tail_ != nullptr) {
        tail_.promise().next_ready_ = handle;
      } else {
        head_ = handle;
      }
      tail_ = handle;
    }

    Handle pop() noexcept {
      Handle handle = head_;
      head_ = handle.promise().next_ready_;
      if (head_ == nullptr) {
        tail_ = nullptr;
      }
      return handle;
    }

    Handle head_{nullptr};
    Handle tail_{nullptr};
  };

  Queue queue_{};
  bool running_{false};
};

namespace detail::coro {

[[nodiscard]] inline PromiseBase& currentCreator() noexcept {
  PromiseBase* creator = g_slot;
  if constexpr (Traps_Enabled) {
    if (creator == nullptr) {
      fault(FaultCode::No_Activity, 0, 0);
    }
  }
  return *creator;
}

[[nodiscard]] inline Arena& currentArena() noexcept {
  return currentCreator().arena_;
}

inline void attach(PromiseBase& self) noexcept {
  PromiseBase& creator = currentCreator();
  self.parent_ = self.arena_.offsetOf(creator);
  creator.children_.born();
  self.arena_.bind(self);
  g_slot = &self;
}

struct Reschedule_Awaiter {
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    CoroScheduler::getInstance().enqueue(handle);
  }
  void await_resume() noexcept {}
};

template <class Awaiter>
struct Slot_Awaiter {
  PromiseBase& self;
  Awaiter awaiter;

  bool await_ready() { return awaiter.await_ready(); }

  template <class Caller>
  decltype(auto) await_suspend(Caller caller) {
    g_slot =
        self.parent_ == No_Node ? nullptr : &self.arena_.nodeAt(self.parent_);
    return awaiter.await_suspend(caller);
  }

  decltype(auto) await_resume() {
    g_slot = &self;
    return awaiter.await_resume();
  }
};

struct Final_Awaiter {
  bool await_ready() noexcept { return false; }

  template <typename PromiseT>
  void await_suspend(std::coroutine_handle<PromiseT> handle) noexcept {
    PromiseBase& promise = handle.promise();
    if constexpr (Traps_Enabled) {
      if (promise.children_.live() != 0) {
        fault(FaultCode::Children_Alive, promise.children_.live(), 0);
      }
    }
    if (promise.parent_ != No_Node) {
      PromiseBase& parent = promise.arena_.nodeAt(promise.parent_);
      parent.children_.died();
      g_slot = &parent;
    } else {
      g_slot = nullptr;
    }
    if (promise.continuation_ != nullptr) {
      // continuation_ stays set until the waiter clears it; until then the
      // frame stays in the arena (Arena::sweep).
      PromiseBase& continuation = *promise.continuation_;
      continuation.waiting_for_nested_ = false;
      CoroScheduler::getInstance().enqueue(
          std::coroutine_handle<PromiseBase>::from_promise(continuation));
    }
  }

  void await_resume() noexcept {}
};

template <typename T>
struct Value_Promise : PromiseBase {
  using Value = T;
  T value{};

  explicit Value_Promise(Arena& arena) noexcept : PromiseBase(arena) {}

  void return_value(T produced) { this->value = std::move(produced); }
};

template <>
struct Value_Promise<void> : PromiseBase {
  using Value = void;

  explicit Value_Promise(Arena& arena) noexcept : PromiseBase(arena) {}

  void return_void() noexcept {}
};

template <typename T>
struct Task_Promise : Value_Promise<T> {
  Task_Promise() noexcept : Value_Promise<T>(currentArena()) { attach(*this); }

  static void* operator new(std::size_t bytes) {
    return currentArena().allocate(bytes);
  }
  static void operator delete(void*, std::size_t) noexcept {}
  static void operator delete(void*) noexcept {}

  Task<T> get_return_object() noexcept {
    return Task<T>{std::coroutine_handle<Task_Promise>::from_promise(*this)};
  }

  std::suspend_never initial_suspend() noexcept { return {}; }
  Final_Awaiter final_suspend() noexcept { return {}; }

  void unhandled_exception() noexcept {}
};

template <typename T>
struct Activity_Promise : Value_Promise<T> {
  template <typename... Rest>
  explicit Activity_Promise(Arena& arena, Rest&&...) noexcept
      : Value_Promise<T>(arena) {
    arena.bind(*this);
  }

  template <typename... Rest>
  static void* operator new(std::size_t bytes, Arena& arena, Rest&&...) {
    return arena.allocate(bytes);
  }
  static void operator delete(void*, std::size_t) noexcept {}
  static void operator delete(void*) noexcept {}

  Activity<T> get_return_object() noexcept {
    return Activity<T>{
        std::coroutine_handle<Activity_Promise>::from_promise(*this)};
  }

  Reschedule_Awaiter initial_suspend() noexcept { return {}; }
  Final_Awaiter final_suspend() noexcept { return {}; }

  void unhandled_exception() noexcept {}
};

template <class PromiseT>
struct Task_Awaiter {
  std::coroutine_handle<PromiseT> coro;
  bool suspended_{false};

  bool await_ready() noexcept { return !coro || coro.done(); }

  template <class Caller>
  void await_suspend(Caller caller) noexcept {
    if constexpr (Traps_Enabled) {
      if (coro.promise().continuation_ != nullptr) {
        fault(FaultCode::Task_Already_Awaited, 0, 0);
      }
    }
    PromiseBase& waiting = caller.promise();
    // The one-waiter claim, and the pin that keeps Arena::sweep() from
    // recycling the frame before the value is read.
    coro.promise().continuation_ = &waiting;
    suspended_ = true;
    waiting.waiting_for_nested_ = true;
  }

  auto await_resume() noexcept {
    if (suspended_) {
      coro.promise().continuation_ = nullptr;
      suspended_ = false;
    }
    using Value = typename PromiseT::Value;
    if constexpr (std::is_void_v<Value>) {
      return;
    } else {
      if (!coro) {
        return Value{};
      }
      return Value{std::move(coro.promise().value)};
    }
  }
};

template <typename PromiseT>
class Task_Handle {
 public:
  Task_Handle() noexcept = default;
  ~Task_Handle() {
    if (arena_ != nullptr) arena_->release(offset_);
  }
  Task_Handle(Task_Handle&& other) noexcept
      : arena_{std::exchange(other.arena_, nullptr)},
        offset_{std::exchange(other.offset_, No_Node)} {}
  Task_Handle& operator=(Task_Handle&& other) noexcept {
    if (this != &other) {
      Task_Handle moved{std::move(other)};
      std::swap(arena_, moved.arena_);
      std::swap(offset_, moved.offset_);
    }
    return *this;
  }
  Task_Handle(const Task_Handle&) = delete;
  Task_Handle& operator=(const Task_Handle&) = delete;

  /// True once the body has finished; an empty handle counts as done.
  [[nodiscard]] bool done() const noexcept {
    return arena_ == nullptr || handle().done();
  }

 protected:
  template <typename>
  friend struct Task_Promise;
  template <typename>
  friend struct Activity_Promise;

  explicit Task_Handle(std::coroutine_handle<PromiseT> coro) noexcept
      : arena_{coro ? &coro.promise().arena_ : nullptr} {
    if (coro) {
      PromiseBase& node = coro.promise();
      node.owned_ = true;
      offset_ = arena_->offsetOf(node);
    }
  }

  [[nodiscard]] std::coroutine_handle<PromiseT> handle() const noexcept {
    return arena_ == nullptr
               ? std::coroutine_handle<PromiseT>{}
               : std::coroutine_handle<PromiseT>::from_promise(
                     static_cast<PromiseT&>(arena_->nodeAt(offset_)));
  }

 private:
  Arena* arena_{nullptr};
  Offset offset_{No_Node};
};
}  // namespace detail::coro

template <typename T>
class Task : public detail::coro::Task_Handle<detail::coro::Task_Promise<T>> {
 public:
  using promise_type = detail::coro::Task_Promise<T>;

  Task() noexcept = default;
  Task(Task&&) noexcept = default;
  Task& operator=(Task&&) noexcept = default;
  ~Task() = default;

  auto operator co_await() && {
    return detail::coro::Task_Awaiter<promise_type>{this->handle()};
  }

  auto operator co_await() & {
    return detail::coro::Task_Awaiter<promise_type>{this->handle()};
  }

 private:
  template <typename>
  friend struct detail::coro::Task_Promise;

  using detail::coro::Task_Handle<promise_type>::Task_Handle;
};

template <typename T>
class Activity
    : public detail::coro::Task_Handle<detail::coro::Activity_Promise<T>> {
 public:
  using promise_type = detail::coro::Activity_Promise<T>;

  Activity() noexcept = default;
  Activity(Activity&&) noexcept = default;
  Activity& operator=(Activity&&) noexcept = default;
  ~Activity() = default;

  auto operator co_await() & {
    return detail::coro::Task_Awaiter<promise_type>{this->handle()};
  }

 private:
  template <typename>
  friend struct detail::coro::Activity_Promise;

  using detail::coro::Task_Handle<promise_type>::Task_Handle;
};

}  // namespace m

#endif
