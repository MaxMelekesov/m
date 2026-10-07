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
 * Usage:
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
 *
 * Public API — everything else in this header is internal:
 *   using the scheduler
 *     CoroArena<Bytes>            static frame buffer + arena
 *     Arena                       the arena reference an activity takes
 *     Activity<T>                 root coroutine, one per activity
 *     Task<T>                     nested coroutine, borrows the arena
 *     CoroScheduler::getInstance().handle()   one round of the superloop
 *     CoroScheduler::getInstance().enqueue(h) for a custom awaiter that wants
 *                                 `h` resumed in a later round
 *     Arena::capacity/used/peak/peakFrames    sizing and accounting
 *     Arena::Alignment                        what a frame is aligned to
 *   the coro* helpers (CoroYield/CoroDelay/CoroUntil/CoroMutex headers)
 *     coroYield(), coroDelay(), coroUntil(), coroWhile(), CoroMutex
 *   joining a child
 *     co_await the child's own Task<T> handle - keep the handle in the frame
 *     that needs the result, that is the whole join mechanism
 *   diagnostics
 *     FaultCode, FaultInfo
 *     CoroScheduler::getInstance().setFaultHandler(h)   where a fault goes
 *
 * Everything else lives in namespace m::detail::coro: it is implementation, the
 * names and the layout change without notice, and nothing outside this header
 * may name it.  api_boundary.sh in the session harness asserts that none of it
 * is reachable from the outside.
 *
 * Notes:
 *   - Nested coroutines are ordinary functions returning m::Task<T>: no arena
 *     of their own, they borrow the activity's.
 *   - A nested coroutine runs to its first suspension as it is created, so
 *     building a deep chain costs stack per level (about 50-130 bytes).
 *   - Size an arena from the measured worst case: g_homing_arena.peak().
 *     Frames are reclaimed only from the top of the stack, so a burst of
 *     detached children needs room for all of them at once: the number to
 *     size with is g_homing_arena.peakFrames().
 *   - A frame local must not ask for more alignment than m::Arena::Alignment:
 *     frames are aligned to it, and the compiler does not diagnose a mismatch.
 *   - A creator must await its nested coroutines before returning.
 *   - The arena is made only by CoroArena: it owns the frame buffer, so there
 *     is no arena over anyone else's buffer.  Put the CoroArena object where
 *     the frames should live (a static, a member, a local scope).
 *   - An Activity handle is a lease on the arena: while it is alive, a new
 *     activity on the same arena reaches the fault handler as Arena_Busy, even
 *     if the previous activity already finished.  Let the handle leave its
 *     scope before starting the next activity on that arena.
 *   - One task has one waiter: awaiting a task that another coroutine is
 *     already awaiting reaches the fault handler as Task_Already_Awaited.
 *   - A task handle must not outlive the frame of the coroutine that created
 *     it.  It does not have to outlive its waiters: a frame that is still
 *     awaited is kept in the arena until the waiter has read its value, and
 *     then stays there until everything above it is gone.
 *   - Faults (overflow, a busy arena, a task outside an activity, live nested
 *     coroutines, a task awaited twice, a re-entrant handle()) reach the
 *     installed handler as m::FaultInfo{code, need, have}; nothing is printed,
 *     and without a handler std::abort() runs:
 *
 *       m::CoroScheduler::getInstance().setFaultHandler(
 *           [](const m::FaultInfo& f) {
 *             tracer.report(static_cast<unsigned>(f.code), f.need, f.have);
 *           });
 *
 *     M_CORO_ARENA_TRAPS=0 compiles these traps out for a release build; the
 *     faults that report a resource limit (overflow) are always compiled in.
 */

namespace m {

// Compile-time switches for the tightest builds:
//   M_CORO_ARENA_ENABLE_STATS=0 - strip peak()/peakFrames()/frames_ tracking
//     (~6% off creation); peak() and peakFrames() then report 0, so run the
//     sizing pass with the statistics on.
//   M_CORO_ARENA_TRAPS=0 - remove the precondition traps (a task created
//     outside an activity, a task awaited twice, a re-entrant handle(), a
//     second activity on a busy arena, a creator returning with live nested
//     coroutines).  Valid code never reaches one, and each costs a compare on
//     the creation or suspension path; with the traps gone an already broken
//     precondition is silent instead of loud, so keep them on in any build
//     that can still be debugged.  The children counter behind the
//     Children_Alive trap, and the memory it takes in every frame, go with
//     them: a build without traps joins a child by awaiting its handle.
#ifndef M_CORO_ARENA_ENABLE_STATS
#define M_CORO_ARENA_ENABLE_STATS 1
#endif
#ifndef M_CORO_ARENA_TRAPS
#define M_CORO_ARENA_TRAPS 1
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

// ───────────────────────────── implementation ──────────────────────────────
// Nothing below this point is part of the API: the names, the layout and the
// behaviour of detail::coro change without notice, and no other header may
// name anything inside it.

namespace detail::coro {

using Offset = std::size_t;
inline constexpr Offset No_Node = static_cast<Offset>(-1);

// Every frame starts on this boundary; it is what a coroutine frame needs for
// any object the standard library may put in one (the larger of max_align_t
// and the default operator new alignment).
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
inline constexpr std::size_t Arena_Alignment =
    std::max(alignof(std::max_align_t),
             static_cast<std::size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__));
#else
inline constexpr std::size_t Arena_Alignment = alignof(std::max_align_t);
#endif

inline constexpr bool Traps_Enabled = (M_CORO_ARENA_TRAPS != 0);
[[nodiscard]] inline Fault_Handler& faultHandler() noexcept {
  static Fault_Handler handler = nullptr;
  return handler;
}

[[noreturn]] inline void fault(FaultCode code, std::size_t need,
                               std::size_t have) noexcept {
  if (Fault_Handler handler = faultHandler(); handler != nullptr) {
    handler(FaultInfo{code, need, have});
  }
  std::abort();
}

/// How many nested coroutines a frame is still waiting for.  Only the
/// Children_Alive trap reads it, so a build without traps has no counter and
/// pays no memory for one.
template <bool Enabled>
struct Children {
  std::size_t n{0};
  void born() noexcept { ++n; }
  void died() noexcept {
    if (n > 0) {
      --n;
    }
  }
  [[nodiscard]] std::size_t live() const noexcept { return n; }
};

template <>
struct Children<false> {
  void born() noexcept {}
  void died() noexcept {}
  [[nodiscard]] std::size_t live() const noexcept { return 0; }
};

struct PromiseBase {
  explicit PromiseBase(Arena& arena) noexcept : arena_(arena) {}

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

[[nodiscard]] inline PromiseBase*& currentSlot() noexcept {
  static PromiseBase* current = nullptr;
  return current;
}

// Forward declarations: Arena befriends these, they are defined below.
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

  /// What a frame is aligned to.  A local variable inside a coroutine must not
  /// ask for more alignment than this: the compiler does not diagnose it, and
  /// the object would silently land misaligned in the frame.
  static constexpr std::size_t Alignment = detail::coro::Arena_Alignment;

 private:
  // Only CoroArena makes an Arena: it is the one that owns the storage, so the
  // storage is always aligned and always as large as it says.  An Arena is
  // never built on a buffer from the outside.
  template <std::size_t>
  friend class CoroArena;

  constexpr explicit Arena(std::span<std::byte> storage) noexcept
      : storage_(storage) {
    if (std::is_constant_evaluated()) {
      return;
    }
    if constexpr (detail::coro::Traps_Enabled) {
      // CoroArena declares its storage alignas(Alignment), so this can only
      // fire if the library itself regresses - it is kept as a guard.
      const auto base = reinterpret_cast<std::uintptr_t>(storage_.data());
      if (base % detail::coro::Arena_Alignment != 0) {
        detail::coro::fault(
            FaultCode::Arena_Misaligned, detail::coro::Arena_Alignment,
            static_cast<std::size_t>(base % detail::coro::Arena_Alignment));
      }
    }
  }

  // The frame machinery is the only thing allowed to touch the stack; the
  // list is also the documentation of who does what:
  //   Task_Promise/Activity_Promise - take the frame (allocate, bind)
  //   Task_Handle                   - give it back (release, offsetOf, nodeAt)
  //   attach                        - link a nested frame to its creator
  //   Slot_Awaiter/Final_Awaiter    - walk to the creator while suspending
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
    // The promise is constructed after the frame, so bind() reads the frame
    // end back from here.  0 means "nothing pending", and a frame end is never
    // 0, so the flag needs no second member.
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
    if (pending_ != 0) {
      node.end_ = pending_;
      pending_ = 0;
    } else {
      node.end_ = (top_ == No_Node) ? 0 : nodeAt(top_).end_;
    }
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
            node.parent_ != No_Node &&
            std::coroutine_handle<Node>::from_promise(nodeAt(node.parent_))
                .done();
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
  /// Where a fault is reported before std::abort().  Kept on the instance,
  /// like setCoroutineOomCallback() on the pool scheduler in
  /// m/Logic/FlowControl; the storage itself stays with the fault machinery so
  /// that an Arena alone does not pull the scheduler into a translation unit.
  void setFaultHandler(Fault_Handler handler) noexcept {
    detail::coro::faultHandler() = handler;
  }

  void handle() {
    if constexpr (detail::coro::Traps_Enabled) {
      if (running_) {
        detail::coro::fault(FaultCode::Reentrant_Handle, 0, 0);
      }
    }
    running_ = true;
    // One round = the coroutines that are ready right now: remembering the
    // tail is exactly that set, and saves a counter update per pop.
    const Handle stop = queue_.tail();
    if (!queue_.empty()) {
      for (;;) {
        Handle head = queue_.pop();
        detail::coro::PromiseBase& promise = head.promise();
        promise.scheduled_ = false;
        // No done() test here: a queued coroutine is never finished.  It can
        // enter the queue only while it is suspended (yield, task await or an
        // external awaiter), and every wakeup clears waiting_for_nested_ before
        // enqueueing it.  tests/model_check asserts this after every round.
        detail::coro::currentSlot() = &promise;
        head.resume();
        if (head == stop) {
          break;
        }
      }
    }
    detail::coro::currentSlot() = nullptr;
    running_ = false;
  }

  /// The extension point for an awaiter of your own: hand it the coroutine it
  /// should resume in a later round, exactly as coroYield() does.
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
    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
    [[nodiscard]] Handle tail() const noexcept { return tail_; }

    void push(Handle handle) noexcept {
      handle.promise().next_ready_ = nullptr;
      if (tail_ != nullptr) {
        tail_.promise().next_ready_ = handle;
      } else {
        head_ = handle;
      }
      tail_ = handle;
    }

    [[nodiscard]] Handle pop() noexcept {
      Handle handle = head_;
      head_ = handle.promise().next_ready_;
      if (head_ == nullptr) {
        tail_ = nullptr;
      }
      return handle;
    }

   private:
    Handle head_{nullptr};
    Handle tail_{nullptr};
  };

  Queue queue_{};
  bool running_{false};
};

namespace detail::coro {

[[nodiscard]] inline PromiseBase& currentCreator() noexcept {
  PromiseBase* creator = currentSlot();
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
  currentSlot() = &self;
}

struct Reschedule_Awaiter {
  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    CoroScheduler::getInstance().enqueue(handle);
  }
  void await_resume() noexcept {}
};

[[nodiscard]] inline Reschedule_Awaiter yield() noexcept { return {}; }

template <class Operand>
concept Has_Co_Await = requires(Operand&& value) {
  std::forward<Operand>(value).operator co_await();
};

template <class Awaiter>
struct Slot_Awaiter {
  PromiseBase& self;
  Awaiter awaiter;

  bool await_ready() { return awaiter.await_ready(); }

  template <class Caller>
  decltype(auto) await_suspend(Caller caller) {
    currentSlot() =
        self.parent_ == No_Node ? nullptr : &self.arena_.nodeAt(self.parent_);
    return awaiter.await_suspend(caller);
  }

  decltype(auto) await_resume() {
    currentSlot() = &self;
    return awaiter.await_resume();
  }
};

struct Slot_Promise : PromiseBase {
  explicit Slot_Promise(Arena& arena) noexcept : PromiseBase(arena) {}

  template <class Operand>
  auto await_transform(Operand&& operand) {
    if constexpr (Has_Co_Await<Operand>) {
      using Awaiter =
          decltype(std::forward<Operand>(operand).operator co_await());
      return Slot_Awaiter<Awaiter>{
          *this, std::forward<Operand>(operand).operator co_await()};
    } else {
      using Awaiter = std::remove_cvref_t<Operand>;
      return Slot_Awaiter<Awaiter>{*this, std::forward<Operand>(operand)};
    }
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
      currentSlot() = &parent;
    } else {
      currentSlot() = nullptr;
    }
    if (promise.continuation_ != nullptr) {
      // continuation_ is left set: the waiter clears it in await_resume, and
      // until it does the frame below must stay in the arena (Arena::sweep).
      PromiseBase& continuation = *promise.continuation_;
      continuation.waiting_for_nested_ = false;
      CoroScheduler::getInstance().enqueue(
          std::coroutine_handle<PromiseBase>::from_promise(continuation));
    }
  }

  void await_resume() noexcept {}
};

template <typename T>
struct Value_Promise : Slot_Promise {
  using Value = T;
  T value{};

  explicit Value_Promise(Arena& arena) noexcept : Slot_Promise(arena) {}

  void return_value(T produced) { this->value = std::move(produced); }
};

template <>
struct Value_Promise<void> : Slot_Promise {
  using Value = void;

  explicit Value_Promise(Arena& arena) noexcept : Slot_Promise(arena) {}

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
    // Stays set until this awaiter resumes: it is both the "one task, one
    // waiter" claim and the pin that keeps Arena::sweep() from recycling the
    // frame before the value is read below.
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
  ~Task_Handle() { releaseFrame(); }
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
               : std::coroutine_handle<PromiseT>::from_promise(promise());
  }

 private:
  [[nodiscard]] PromiseT& promise() const noexcept {
    return static_cast<PromiseT&>(arena_->nodeAt(offset_));
  }

  void releaseFrame() noexcept {
    if (arena_ != nullptr) {
      arena_->release(offset_);
    }
    arena_ = nullptr;
    offset_ = No_Node;
  }

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
