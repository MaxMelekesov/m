/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PROTO_THREAD_TEST_HPP
#define PROTO_THREAD_TEST_HPP

#include <Ms.hpp>
#include <ProtoThread.hpp>
#include <PtDelay.hpp>
#include <array>

namespace m::tsts {

// ─── Fake time for testing ──────────────────────────────────────

class FakeTimeMs {
  Ms<uint32_t> tick_{0};

 public:
  void advance(Ms<uint32_t> dt) { tick_ += dt; }

  Ms<uint32_t> now() { return tick_; }

  Ms<uint32_t> diff(Ms<uint32_t> start) {
    return Ms<uint32_t>(tick_.value() - start.value());
  }

  void delay(Ms<uint32_t>) {}
};

static_assert(ifc::CTime<FakeTimeMs>);

// ─── Test log ───────────────────────────────────────────────────

template <std::size_t N>
struct TestLog {
  std::array<uint8_t, N> events{};
  std::size_t count = 0;

  void log(uint8_t event) {
    if (count < N) events[count++] = event;
  }

  void reset() { count = 0; }

  bool verify(std::initializer_list<uint8_t> expected) const {
    if (count != expected.size()) return false;
    auto it = expected.begin();
    for (std::size_t i = 0; i < count; ++i, ++it) {
      if (events[i] != *it) return false;
    }
    return true;
  }
};

// ─── 1. Basic void task with yield ─────────────────────────────

template <std::size_t N>
struct BasicYield : Proto<BasicYield<N>> {
  TestLog<N>& log_;

  explicit BasicYield(TestLog<N>& log) : log_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(1);
    PT_YIELD();
    log_.log(2);
    PT_YIELD();
    log_.log(3);
    PT_END();
  }
};

// ─── 2. Task with return value ──────────────────────────────────

template <std::size_t N>
struct Add : Proto<Add<N>, int> {
  TestLog<N>& log_;
  int a_, b_;

  Add(TestLog<N>& log, int a, int b) : log_(log), a_(a), b_(b) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(10);
    PT_YIELD();
    log_.log(11);
    PT_RETURN(a_ + b_);
    PT_END();
  }
};

// ─── 3. Task with PT_WAIT_UNTIL ─────────────────────────────────

template <std::size_t N>
struct WaitForFlag : Proto<WaitForFlag<N>> {
  TestLog<N>& log_;
  bool& flag_;

  WaitForFlag(TestLog<N>& log, bool& flag) : log_(log), flag_(flag) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(20);
    PT_WAIT_UNTIL(flag_);
    log_.log(21);
    PT_END();
  }
};

// ─── 4. Task with PT_WAIT_WHILE ─────────────────────────────────

template <std::size_t N>
struct WaitWhileBusy : Proto<WaitWhileBusy<N>> {
  TestLog<N>& log_;
  bool& busy_;

  WaitWhileBusy(TestLog<N>& log, bool& busy) : log_(log), busy_(busy) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(30);
    PT_WAIT_WHILE(busy_);
    log_.log(31);
    PT_END();
  }
};

// ─── 5. Task with PT_EXIT (early exit) ─────────────────────────

template <std::size_t N>
struct EarlyExit : Proto<EarlyExit<N>, bool> {
  TestLog<N>& log_;
  bool should_exit_;

  EarlyExit(TestLog<N>& log, bool should_exit)
      : log_(log), should_exit_(should_exit) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(40);
    if (should_exit_) {
      log_.log(41);
      PT_RETURN(false);
    }
    log_.log(42);
    PT_RETURN(true);
    PT_END();
  }
};

// ─── 6. Task with PT_RESTART ────────────────────────────────────

template <std::size_t N>
struct RestartOnce : Proto<RestartOnce<N>> {
  TestLog<N>& log_;
  uint8_t attempts_ = 0;

  explicit RestartOnce(TestLog<N>& log) : log_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(50);
    if (attempts_++ < 1) {
      log_.log(51);
      PT_RESTART();
    }
    log_.log(52);
    PT_END();
  }
};

// ─── 7. Nested PT_AWAIT: parent awaits child ───────────────────

template <std::size_t N>
struct NestedAwait : Proto<NestedAwait<N>, int> {
  TestLog<N>& log_;
  Add<N> add_;
  int captured_ = 0;

  NestedAwait(TestLog<N>& log) : log_(log), add_(log, 3, 7) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(60);
    captured_ = PT_AWAIT(add_);
    log_.log(61);
    PT_RETURN(captured_);
    PT_END();
  }
};

// ─── 8. Deep nesting (3 levels) ────────────────────────────────

template <std::size_t N>
struct Leaf : Proto<Leaf<N>, int> {
  TestLog<N>& log_;

  explicit Leaf(TestLog<N>& log) : log_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(70);
    PT_YIELD();
    log_.log(71);
    PT_RETURN(42);
    PT_END();
  }
};

template <std::size_t N>
struct Middle : Proto<Middle<N>, int> {
  TestLog<N>& log_;
  Leaf<N> leaf_;
  int val_ = 0;

  explicit Middle(TestLog<N>& log) : log_(log), leaf_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(72);
    val_ = PT_AWAIT(leaf_);
    log_.log(73);
    PT_RETURN(val_ + 1);
    PT_END();
  }
};

template <std::size_t N>
struct Root3Level : Proto<Root3Level<N>, int> {
  TestLog<N>& log_;
  Middle<N> middle_;
  int val_ = 0;

  explicit Root3Level(TestLog<N>& log) : log_(log), middle_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(74);
    val_ = PT_AWAIT(middle_);
    log_.log(75);
    PT_RETURN(val_ + 1);
    PT_END();
  }
};

// ─── 9. PtDelay ────────────────────────────────────────────────

template <std::size_t N>
struct DelayTask : Proto<DelayTask<N>> {
  TestLog<N>& log_;
  PtDelay<FakeTimeMs> delay_;

  DelayTask(TestLog<N>& log, FakeTimeMs& time) : log_(log), delay_(time) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(80);
    PT_AWAIT(delay_(Ms<uint32_t>(100)));
    log_.log(81);
    PT_END();
  }
};

// ─── 10. Async spawn + join ─────────────────────────────────────

template <std::size_t N>
struct AsyncWorker : Proto<AsyncWorker<N>, int> {
  TestLog<N>& log_;

  explicit AsyncWorker(TestLog<N>& log) : log_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(90);
    PT_YIELD();
    log_.log(91);
    PT_YIELD();
    log_.log(92);
    PT_RETURN(99);
    PT_END();
  }
};

template <std::size_t N>
struct AsyncSpawner : Proto<AsyncSpawner<N>, int> {
  TestLog<N>& log_;
  AsyncWorker<N> worker_;
  int captured_ = 0;

  explicit AsyncSpawner(TestLog<N>& log) : log_(log), worker_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(93);
    PT_SPAWN(worker_);
    log_.log(94);
    PT_YIELD();
    log_.log(95);
    PT_WAIT_UNTIL(worker_.done());
    captured_ = worker_.result();
    log_.log(96);
    PT_RETURN(captured_);
    PT_END();
  }
};

// ─── 11. Sequential awaits of same child ────────────────────────

template <std::size_t N>
struct TwiceAwait : Proto<TwiceAwait<N>> {
  TestLog<N>& log_;
  BasicYield<N> child_;

  explicit TwiceAwait(TestLog<N>& log) : log_(log), child_(log) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(100);
    PT_AWAIT(child_);
    log_.log(101);
    PT_AWAIT(child_);
    log_.log(102);
    PT_END();
  }
};

// ─── 12. Multiple sequential awaits ─────────────────────────────

template <std::size_t N>
struct MultiAwait : Proto<MultiAwait<N>, int> {
  TestLog<N>& log_;
  Add<N> add1_;
  Add<N> add2_;
  int sum_ = 0;

  explicit MultiAwait(TestLog<N>& log)
      : log_(log), add1_(log, 10, 20), add2_(log, 30, 40) {}

  PtStatus run() {
    PT_BEGIN();
    log_.log(110);
    sum_ = PT_AWAIT(add1_);
    log_.log(111);
    sum_ += PT_AWAIT(add2_);
    log_.log(112);
    PT_RETURN(sum_);
    PT_END();
  }
};

// ─── 13. PtMutex ────────────────────────────────────────────────

template <std::size_t N>
struct MutexUser : Proto<MutexUser<N>> {
  TestLog<N>& log_;
  PtMutex& mtx_;
  uint8_t id_;

  MutexUser(TestLog<N>& log, PtMutex& mtx, uint8_t id)
      : log_(log), mtx_(mtx), id_(id) {}

  PtStatus run() {
    PT_BEGIN();
    PT_WAIT_UNTIL(mtx_.tryLock());
    log_.log(id_);
    PT_YIELD();
    log_.log(id_ + 10);
    mtx_.unlock();
    PT_END();
  }
};

// ═══════════════════════════════════════════════════════════════
//  Test runner
// ═══════════════════════════════════════════════════════════════

inline bool protoThreadTest() {
  constexpr std::size_t Log_Size = 64;
  using Log = TestLog<Log_Size>;

  auto& sched = PtScheduler::getInstance();

  auto tick = [&](auto& /*task*/) { sched.handle(); };

  auto run_until_done = [&](auto& task, int max_ticks = 100) {
    for (int i = 0; i < max_ticks && !task.done(); ++i) {
      sched.handle();
    }
    return task.done();
  };

  // ── Test 1: Basic yield ──
  {
    sched.clear();
    Log log;
    BasicYield<Log_Size> task(log);
    sched.add(task);

    tick(task);  // logs 1
    if (!log.verify({1})) return false;

    tick(task);  // logs 2
    if (!log.verify({1, 2})) return false;

    tick(task);  // logs 3, done
    if (!log.verify({1, 2, 3})) return false;
    if (!task.done()) return false;
  }

  // ── Test 2: Return value ──
  {
    sched.clear();
    Log log;
    Add<Log_Size> task(log, 5, 8);
    sched.add(task);

    tick(task);
    if (!log.verify({10})) return false;

    tick(task);
    if (!log.verify({10, 11})) return false;
    if (!task.done()) return false;
    if (task.result() != 13) return false;
  }

  // ── Test 3: PT_WAIT_UNTIL ──
  {
    sched.clear();
    Log log;
    bool flag = false;
    WaitForFlag<Log_Size> task(log, flag);
    sched.add(task);

    tick(task);  // logs 20, waits
    if (!log.verify({20})) return false;

    tick(task);  // still waiting
    if (!log.verify({20})) return false;
    if (task.done()) return false;

    flag = true;
    tick(task);  // logs 21, done
    if (!log.verify({20, 21})) return false;
    if (!task.done()) return false;
  }

  // ── Test 4: PT_WAIT_WHILE ──
  {
    sched.clear();
    Log log;
    bool busy = true;
    WaitWhileBusy<Log_Size> task(log, busy);
    sched.add(task);

    tick(task);
    if (!log.verify({30})) return false;

    tick(task);
    if (task.done()) return false;

    busy = false;
    tick(task);
    if (!log.verify({30, 31})) return false;
    if (!task.done()) return false;
  }

  // ── Test 5: Early exit with PT_RETURN ──
  {
    sched.clear();
    Log log;
    EarlyExit<Log_Size> task(log, true);
    sched.add(task);

    tick(task);
    if (!log.verify({40, 41})) return false;
    if (!task.done()) return false;
    if (task.result() != false) return false;
  }
  {
    sched.clear();
    Log log;
    EarlyExit<Log_Size> task(log, false);
    sched.add(task);

    tick(task);
    if (!log.verify({40, 42})) return false;
    if (!task.done()) return false;
    if (task.result() != true) return false;
  }

  // ── Test 6: PT_RESTART ──
  {
    sched.clear();
    Log log;
    RestartOnce<Log_Size> task(log);
    sched.add(task);

    tick(task);  // logs 50, 51, restarts
    if (!log.verify({50, 51})) return false;
    if (task.done()) return false;

    tick(task);  // logs 50, 52, done
    if (!log.verify({50, 51, 50, 52})) return false;
    if (!task.done()) return false;
  }

  // ── Test 7: Nested PT_AWAIT with result ──
  {
    sched.clear();
    Log log;
    NestedAwait<Log_Size> task(log);
    sched.add(task);

    if (!run_until_done(task)) return false;
    if (task.result() != 10) return false;
    if (!log.verify({60, 10, 11, 61})) return false;
  }

  // ── Test 8: Deep 3-level nesting ──
  {
    sched.clear();
    Log log;
    Root3Level<Log_Size> task(log);
    sched.add(task);

    if (!run_until_done(task)) return false;
    if (task.result() != 44) return false;
    if (!log.verify({74, 72, 70, 71, 73, 75})) return false;
  }

  // ── Test 9: PtDelay ──
  {
    sched.clear();
    Log log;
    FakeTimeMs time;
    DelayTask<Log_Size> task(log, time);
    sched.add(task);

    tick(task);  // logs 80, starts delay
    if (!log.verify({80})) return false;
    if (task.done()) return false;

    time.advance(Ms<uint32_t>(50));
    tick(task);
    if (task.done()) return false;

    time.advance(Ms<uint32_t>(50));
    tick(task);  // 100ms elapsed, logs 81
    if (!log.verify({80, 81})) return false;
    if (!task.done()) return false;
  }

  // ── Test 10: Async spawn + join ──
  {
    sched.clear();
    Log log;
    AsyncSpawner<Log_Size> task(log);
    sched.add(task);

    tick(task);  // spawner: 93, 94; worker: 90
    tick(task);  // spawner: 95, waits; worker: 91
    tick(task);  // worker: 92, done; spawner sees done → 96

    if (!task.done()) return false;
    if (task.result() != 99) return false;
  }

  // ── Test 11: Sequential awaits of same child ──
  {
    sched.clear();
    Log log;
    TwiceAwait<Log_Size> task(log);
    sched.add(task);

    if (!run_until_done(task)) return false;
    // Parent: 100, await child (1,2,3), parent: 101, await child again (1,2,3),
    // parent: 102
    if (!log.verify({100, 1, 2, 3, 101, 1, 2, 3, 102})) return false;
  }

  // ── Test 12: Multiple sequential awaits ──
  {
    sched.clear();
    Log log;
    MultiAwait<Log_Size> task(log);
    sched.add(task);

    if (!run_until_done(task)) return false;
    if (task.result() != 100) return false;  // 10+20 + 30+40
    if (!log.verify({110, 10, 11, 111, 10, 11, 112})) return false;
  }

  // ── Test 13: PtMutex ──
  {
    sched.clear();
    Log log;
    PtMutex mtx;
    MutexUser<Log_Size> a(log, mtx, 1);
    MutexUser<Log_Size> b(log, mtx, 2);
    sched.add(a);
    sched.add(b);

    // Tick 1: a locks (logs 1), b waits
    sched.handle();
    if (!log.verify({1})) return false;

    // Tick 2: a logs 11 & unlocks, b locks (logs 2)
    sched.handle();
    if (!log.verify({1, 11, 2})) return false;

    // Tick 3: b logs 12 & unlocks
    sched.handle();
    if (!log.verify({1, 11, 2, 12})) return false;
    if (!a.done() || !b.done()) return false;
  }

  // ── Test 14: handle() returns bool ──
  {
    sched.clear();
    Log log;
    BasicYield<Log_Size> task(log);
    sched.add(task);

    if (!sched.handle()) return false;  // task active
    if (!sched.handle()) return false;  // still active
    sched.handle();                     // task finishes
    if (sched.handle()) return false;   // all done → false
  }

  sched.clear();
  return true;
}

}  // namespace m::tsts

#endif  // PROTO_THREAD_TEST_HPP
