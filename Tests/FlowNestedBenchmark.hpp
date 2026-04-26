#pragma once

#include <ChainTask.hpp>
#include <CoroScheduler.hpp>
#include <IO_AsyncLog.hpp>
#include <ProtoThread.hpp>
#include <cstdio>

#include "Hardware.hpp"

#ifndef BENCH_ENABLE_CHAIN
#define BENCH_ENABLE_CHAIN 1
#endif

#ifndef BENCH_ENABLE_CORO
#define BENCH_ENABLE_CORO 1
#endif

#ifndef BENCH_ENABLE_PT
#define BENCH_ENABLE_PT 1
#endif

// 0 - original nested cawait chain (ct14 -> ... -> ct0)
// 1 - minimal-flash runtime path (single ChainTask type + yield loop)
#ifndef BENCH_CHAIN_RUNTIME_MODE
#define BENCH_CHAIN_RUNTIME_MODE 0
#endif

#if !BENCH_ENABLE_CHAIN && !BENCH_ENABLE_CORO && !BENCH_ENABLE_PT
#error "At least one benchmark path must be enabled"
#endif

namespace bench {

namespace detail {
[[nodiscard]] inline uint32_t mixWork(uint32_t x, uint32_t level) {
  x ^= 0x9E3779B9u + level * 0x85EBCA6Bu;
  x = (x << 7) | (x >> 25);
  x += 0x27D4EB2Du;
  return x;
}

template <std::size_t Line_Length, std::size_t Lines>
inline void flushLog(m::IO_AsyncLog<Line_Length, Lines>& log, int n = 24) {
  for (int i = 0; i < n; ++i) {
    log.handle();
  }
}

template <uint32_t Level>
class PtNested final : public m::Proto<PtNested<Level>, void> {
 public:
  explicit PtNested(volatile uint32_t* state)
      : state_ptr_(state), child_(state) {}

  m::PtStatus run() {
    PT_BEGIN();
    *state_ptr_ = mixWork(*state_ptr_, Level);
    PT_AWAIT(child_);
    PT_RETURN();
    PT_END();
  }

 private:
  volatile uint32_t* state_ptr_;
  PtNested<Level - 1> child_;
};

template <>
class PtNested<0> final : public m::Proto<PtNested<0>, void> {
 public:
  explicit PtNested(volatile uint32_t* state) : state_ptr_(state) {}

  m::PtStatus run() {
    PT_BEGIN();
    *state_ptr_ = mixWork(*state_ptr_, 0);
    PT_RETURN();
    PT_END();
  }

 private:
  volatile uint32_t* state_ptr_;
};

}  // namespace detail

template <std::size_t Line_Length, std::size_t Lines>
void runNested15Benchmark(Hardware& hw, m::IO_AsyncLog<Line_Length, Lines>& log) {
  static constexpr int kDepth = 15;
  static constexpr int kIter = 350;
  static constexpr int kRounds = 8;

  // Baseline payload timing: same mixWork workload without framework overhead.
  volatile uint32_t payload_state = 0;
  volatile uint32_t payload_sink = 0;
  uint32_t payload_sum_us = 0;
  for (int r = 0; r < kRounds; ++r) {
    const auto t0 = hw.timeUs().now();
    for (int i = 0; i < kIter; ++i) {
      payload_state = 0x12345678u ^ static_cast<uint32_t>((r << 16) + i);
      for (uint32_t lvl = static_cast<uint32_t>(kDepth); lvl-- > 0;) {
        payload_state = detail::mixWork(payload_state, lvl);
      }
      payload_sink ^= payload_state;
    }
    payload_sum_us += hw.timeUs().diff(t0).value();
  }
  const uint32_t payload_avg_us = payload_sum_us / static_cast<uint32_t>(kRounds);

#if BENCH_ENABLE_CHAIN
  volatile uint32_t chain_state = 0;
  volatile uint32_t chain_sink = 0;

#if BENCH_CHAIN_RUNTIME_MODE
  volatile uint32_t chain_level = 0;
  auto chain_task = m::makeChainTask<std::monostate, std::monostate>(
      m::cstep([&](auto ctx) {
        chain_state = detail::mixWork(chain_state, chain_level);
        if (chain_level > 0) {
          --chain_level;
          return ctx.yield();
        }
        return ctx.done();
      }) |
      m::cresult([] { return std::monostate{}; }));

  uint32_t chain_sum_us = 0;
  for (int r = 0; r < kRounds; ++r) {
    const auto t0 = hw.timeUs().now();
    for (int i = 0; i < kIter; ++i) {
      chain_state = 0x12345678u ^ static_cast<uint32_t>((r << 16) + i);
      chain_level = static_cast<uint32_t>(kDepth - 1);
      chain_task.reset();
      while (!chain_task.handle()) {
      }
      chain_sink ^= chain_state;
    }
    chain_sum_us += hw.timeUs().diff(t0).value();
  }
  const uint32_t chain_avg_us = chain_sum_us / static_cast<uint32_t>(kRounds);
#else
  volatile uint32_t* const chain_ptr = &chain_state;

  auto make_leaf = [chain_ptr](uint32_t level) {
    return m::makeChainTask<std::monostate, std::monostate>(
        m::cstep([chain_ptr, level](auto ctx) {
          *chain_ptr = detail::mixWork(*chain_ptr, level);
          return ctx.finish(std::monostate{});
        }) |
        m::cresult([] { return std::monostate{}; }));
  };

  auto make_node = [chain_ptr](uint32_t level, auto& sub) {
    return m::makeChainTask<std::monostate, std::monostate>(
        m::cstep([chain_ptr, level](auto ctx) {
          *chain_ptr = detail::mixWork(*chain_ptr, level);
          return ctx.done();
        }) |
        m::cawait(sub) | m::cresult([] { return std::monostate{}; }));
  };

  // 15-level nested cawait chain: ct14 -> ... -> ct0
  static auto ct0 = make_leaf(0);
  static auto ct1 = make_node(1, ct0);
  static auto ct2 = make_node(2, ct1);
  static auto ct3 = make_node(3, ct2);
  static auto ct4 = make_node(4, ct3);
  static auto ct5 = make_node(5, ct4);
  static auto ct6 = make_node(6, ct5);
  static auto ct7 = make_node(7, ct6);
  static auto ct8 = make_node(8, ct7);
  static auto ct9 = make_node(9, ct8);
  static auto ct10 = make_node(10, ct9);
  static auto ct11 = make_node(11, ct10);
  static auto ct12 = make_node(12, ct11);
  static auto ct13 = make_node(13, ct12);
  static auto ct14 = make_node(14, ct13);

  uint32_t chain_sum_us = 0;
  for (int r = 0; r < kRounds; ++r) {
    const auto t0 = hw.timeUs().now();
    for (int i = 0; i < kIter; ++i) {
      chain_state = 0x12345678u ^ static_cast<uint32_t>((r << 16) + i);

      ct0.reset();
      ct1.reset();
      ct2.reset();
      ct3.reset();
      ct4.reset();
      ct5.reset();
      ct6.reset();
      ct7.reset();
      ct8.reset();
      ct9.reset();
      ct10.reset();
      ct11.reset();
      ct12.reset();
      ct13.reset();
      ct14.reset();

      ct14.handle();
      chain_sink ^= chain_state;
    }
    chain_sum_us += hw.timeUs().diff(t0).value();
  }
  const uint32_t chain_avg_us = chain_sum_us / static_cast<uint32_t>(kRounds);
#endif
#else
  [[maybe_unused]] const uint32_t chain_avg_us = 0;
  [[maybe_unused]] const uint32_t chain_sink = 0;
#endif

#if BENCH_ENABLE_CORO
  volatile uint32_t coro_state = 0;
  volatile uint32_t coro_sink = 0;

  auto coroNested = [&](auto& self, int level,
                        volatile uint32_t* state) -> m::Task<void> {
    *state = detail::mixWork(*state, static_cast<uint32_t>(level));
    if (level > 0) {
      co_await self(self, level - 1, state);
    }
    co_return;
  };

  uint32_t coro_sum_us = 0;
  for (int r = 0; r < kRounds; ++r) {
    volatile bool done = false;

    [[maybe_unused]] auto round_task = [&]() -> m::Task<void> {
      const auto t0 = hw.timeUs().now();
      for (int i = 0; i < kIter; ++i) {
        coro_state = 0x12345678u ^ static_cast<uint32_t>((r << 16) + i);
        co_await coroNested(coroNested, kDepth - 1, &coro_state);
        coro_sink ^= coro_state;
      }
      coro_sum_us += hw.timeUs().diff(t0).value();
      done = true;
      co_return;
    }();

    while (!done) {
      m::CoroScheduler::getInstance().handle();
    }
  }
  const uint32_t coro_avg_us = coro_sum_us / static_cast<uint32_t>(kRounds);
#else
  [[maybe_unused]] const uint32_t coro_avg_us = 0;
  [[maybe_unused]] const uint32_t coro_sink = 0;
#endif

#if BENCH_ENABLE_PT
  volatile uint32_t pt_state = 0;
  volatile uint32_t pt_sink = 0;

  auto& pt_sched = m::PtScheduler::getInstance();
  pt_sched.clear();
  detail::PtNested<kDepth - 1> pt_root{&pt_state};
  pt_sched.add(pt_root);
  uint32_t pt_sum_us = 0;
  for (int r = 0; r < kRounds; ++r) {
    const auto t0 = hw.timeUs().now();
    for (int i = 0; i < kIter; ++i) {
      pt_state = 0x12345678u ^ static_cast<uint32_t>((r << 16) + i);
      pt_root.reset();
      while (!pt_root.done()) {
        pt_sched.handle();
      }
      pt_sink ^= pt_state;
    }
    pt_sum_us += hw.timeUs().diff(t0).value();
  }
  pt_sched.remove(pt_root);
  const uint32_t pt_avg_us = pt_sum_us / static_cast<uint32_t>(kRounds);
#else
  [[maybe_unused]] const uint32_t pt_avg_us = 0;
  [[maybe_unused]] const uint32_t pt_sink = 0;
#endif

  {
    char buf[192];
    int len =
        std::snprintf(buf, sizeof(buf), "bench d15 i%d r%d:", kIter, kRounds);
#if BENCH_ENABLE_CHAIN
    if (len > 0 && static_cast<std::size_t>(len) < sizeof(buf)) {
      len += std::snprintf(buf + len, sizeof(buf) - static_cast<std::size_t>(len),
                           " chain=%lu us",
                           static_cast<unsigned long>(chain_avg_us));
    }
#endif
#if BENCH_ENABLE_CORO
    if (len > 0 && static_cast<std::size_t>(len) < sizeof(buf)) {
      len += std::snprintf(buf + len, sizeof(buf) - static_cast<std::size_t>(len),
                           " coro=%lu us",
                           static_cast<unsigned long>(coro_avg_us));
    }
#endif
#if BENCH_ENABLE_PT
    if (len > 0 && static_cast<std::size_t>(len) < sizeof(buf)) {
      len += std::snprintf(buf + len, sizeof(buf) - static_cast<std::size_t>(len),
                           " pt=%lu us", static_cast<unsigned long>(pt_avg_us));
    }
#endif
    if (len > 0 && static_cast<std::size_t>(len) < sizeof(buf)) {
      len += std::snprintf(buf + len, sizeof(buf) - static_cast<std::size_t>(len),
                           "\n");
    }
    if (len > 0) {
      const std::size_t n =
          std::min<std::size_t>(static_cast<std::size_t>(len), sizeof(buf) - 1);
      log.add(std::string_view(buf, n));
    }

    bool checksum_ok = true;
#if BENCH_ENABLE_CHAIN && BENCH_ENABLE_CORO
    checksum_ok = checksum_ok && (chain_sink == coro_sink);
#endif
#if BENCH_ENABLE_CHAIN && BENCH_ENABLE_PT
    checksum_ok = checksum_ok && (chain_sink == pt_sink);
#endif
#if BENCH_ENABLE_CORO && BENCH_ENABLE_PT
    checksum_ok = checksum_ok && (coro_sink == pt_sink);
#endif

    int len2 = std::snprintf(
      buf, sizeof(buf), "checksum: chain=%08lx coro=%08lx pt=%08lx %s\n",
      static_cast<unsigned long>(chain_sink),
      static_cast<unsigned long>(coro_sink), static_cast<unsigned long>(pt_sink),
      checksum_ok ? "OK" : "MISMATCH");
    if (len2 > 0) {
      const std::size_t n2 = std::min<std::size_t>(
          static_cast<std::size_t>(len2), sizeof(buf) - 1);
      log.add(std::string_view(buf, n2));
    }

#if BENCH_ENABLE_CORO
    const auto& ps = m::CoroScheduler::getInstance().coroutineMemoryStats();
    int len3 = std::snprintf(buf, sizeof(buf), "pool: peak_slot=%u peak_req=%u\n",
                             static_cast<unsigned>(ps.peak_used_slots),
                             static_cast<unsigned>(ps.peak_requested_size));
#else
    int len3 = std::snprintf(buf, sizeof(buf), "pool: disabled\n");
#endif
    if (len3 > 0) {
      const std::size_t n3 = std::min<std::size_t>(
          static_cast<std::size_t>(len3), sizeof(buf) - 1);
      log.add(std::string_view(buf, n3));
    }

#if BENCH_ENABLE_CHAIN || BENCH_ENABLE_CORO || BENCH_ENABLE_PT
    const uint64_t denom =
        static_cast<uint64_t>(kIter) * static_cast<uint64_t>(kDepth);

    int len4 = std::snprintf(buf, sizeof(buf), "overhead ns/level:\n");
    if (len4 > 0) {
      const std::size_t n4 = std::min<std::size_t>(
          static_cast<std::size_t>(len4), sizeof(buf) - 1);
      log.add(std::string_view(buf, n4));
    }

#if BENCH_ENABLE_CHAIN
    const auto chain_ns_per_level = static_cast<unsigned long>(
        (static_cast<uint64_t>(chain_avg_us) * 1000ull) / denom);
    int len5 = std::snprintf(buf, sizeof(buf), "  chain: %lu\n",
                             chain_ns_per_level);
    if (len5 > 0) {
      const std::size_t n5 = std::min<std::size_t>(
          static_cast<std::size_t>(len5), sizeof(buf) - 1);
      log.add(std::string_view(buf, n5));
    }
#endif

#if BENCH_ENABLE_CORO
    const auto coro_ns_per_level = static_cast<unsigned long>(
        (static_cast<uint64_t>(coro_avg_us) * 1000ull) / denom);
    int len6 = std::snprintf(buf, sizeof(buf), "  coro : %lu\n",
                             coro_ns_per_level);
    if (len6 > 0) {
      const std::size_t n6 = std::min<std::size_t>(
          static_cast<std::size_t>(len6), sizeof(buf) - 1);
      log.add(std::string_view(buf, n6));
    }
#endif

#if BENCH_ENABLE_PT
    const auto pt_ns_per_level = static_cast<unsigned long>(
        (static_cast<uint64_t>(pt_avg_us) * 1000ull) / denom);
    int len7 =
        std::snprintf(buf, sizeof(buf), "  pt   : %lu\n", pt_ns_per_level);
    if (len7 > 0) {
      const std::size_t n7 = std::min<std::size_t>(
          static_cast<std::size_t>(len7), sizeof(buf) - 1);
      log.add(std::string_view(buf, n7));
    }
#endif
#endif

    int len8 = std::snprintf(buf, sizeof(buf), "payload: %lu us\n",
                             static_cast<unsigned long>(payload_avg_us));
    if (len8 > 0) {
      const std::size_t n8 = std::min<std::size_t>(
          static_cast<std::size_t>(len8), sizeof(buf) - 1);
      log.add(std::string_view(buf, n8));
    }

    int len9 = std::snprintf(buf, sizeof(buf), "overhead split (%%):\n");
    if (len9 > 0) {
      const std::size_t n9 = std::min<std::size_t>(
          static_cast<std::size_t>(len9), sizeof(buf) - 1);
      log.add(std::string_view(buf, n9));
    }

    auto logSplit = [&](const char* name, uint32_t total_us) {
      if (total_us == 0) return;
      const uint32_t payload_us = std::min(payload_avg_us, total_us);
      const uint32_t useful_pct10 = static_cast<uint32_t>(
          (static_cast<uint64_t>(payload_us) * 1000ull) / total_us);
      const uint32_t overhead_pct10 = 1000u - useful_pct10;
      const uint32_t overhead_us = total_us - payload_us;

      int l = std::snprintf(buf, sizeof(buf),
                            "  %s useful=%lu.%lu overhead=%lu.%lu (over=%lu us)\n",
                            name,
                            static_cast<unsigned long>(useful_pct10 / 10u),
                            static_cast<unsigned long>(useful_pct10 % 10u),
                            static_cast<unsigned long>(overhead_pct10 / 10u),
                            static_cast<unsigned long>(overhead_pct10 % 10u),
                            static_cast<unsigned long>(overhead_us));
      if (l > 0) {
        const std::size_t n = std::min<std::size_t>(
            static_cast<std::size_t>(l), sizeof(buf) - 1);
        log.add(std::string_view(buf, n));
      }
    };

#if BENCH_ENABLE_CHAIN
    logSplit("chain", chain_avg_us);
#endif
#if BENCH_ENABLE_CORO
    logSplit("coro ", coro_avg_us);
#endif
#if BENCH_ENABLE_PT
    logSplit("pt   ", pt_avg_us);
#endif

    detail::flushLog(log);
  }
}

}  // namespace bench
