/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CHAIN_TASK_HPP
#define CHAIN_TASK_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

/**
 * ChainTask — cooperative pausable task built from a linear step pipeline.
 *
 * Overhead (Cortex-M4, arm-none-eabi-gcc -std=c++23):
 *
 *  RAM  : 2 (idx_) + 2 (finished_+pad) + sizeof(expected<T,E>) +
 *         sum(sizeof(each lambda capture)).
 *         No heap, no RTTI, no exceptions.
 *
 *  Flash: N instantiations of runStep<I> (often inlined/folded by LTO) +
 *         N×4-byte read-only jump table (Step_Table) in .rodata for long
 *         pipelines.
 *
 *  CPU  : O(1) dispatch per step.
 *         For short pipelines (Step_Count <= 4), handle() uses direct
 *         runStep<I>() dispatch (runCurrentStep), avoiding an indirect call.
 *         For longer pipelines, it falls back to Step_Table[idx_].
 *         Back-to-back synchronous (ctx.done()) steps are chained in one
 *         handle() call without returning to the caller.
 *
 * cstep lambdas take a StepCtx<T,E> argument; all of its methods return the
 * same type, so the lambda return type is deduced automatically — no explicit
 * trailing return type annotation needed:
 *
 *   m::cstep([&](auto ctx) {
 *       if (!ready) return ctx.yield();       // pause
 *       if (bad)    return ctx.err(Err::Fail); // error
 *       return ctx.done();                    // advance to next step
 *   })
 *
 * Usage:
 *   enum class Err : uint8_t { Fail };
 *
 *   auto sub = m::makeChainTask<int, Err>(
 *       m::cstep([&](auto ctx) {
 *           startDma(); return ctx.done();
 *       }) |
 *       m::cresult([&]{ return dmaResult(); }));
 *
 *   // cawait already propagates sub-task error — no manual check needed.
 *   auto task = m::makeChainTask<bool, Err>(
 *       m::cawait(sub) |
 *       m::cresult([&]{ return true; }));
 *
 *   while (!task.handle()) { } // poll
 *   auto res = task.result(); // const std::expected<bool, Err>&
 *
 * Notes:
 *   - cawait propagates sub-task error automatically.
 *   - reset() rewinds state (idx_, finished_, result).
 *   - Pipelines with <=4 steps are on the fastest dispatch path.
 */

namespace m {

// ─── Internal ────────────────────────────────────────────────────────────────

namespace detail {

enum class Step_Ctrl : uint8_t { Yield, Done, Finish, Err };

// Union avoids default-constructing the inactive field and halves
// the in-register size for small T/E (e.g. bool + uint8_t enum).
// T and E must be trivially destructible (no explicit dtor needed).
template <typename T, typename E>
struct Step_Result {
  Step_Ctrl ctrl_;
  union {
    T finish_val_;
    E err_val_;
  };
  // Yield / Done — union member left indeterminate, never accessed.
  explicit Step_Result(Step_Ctrl c) noexcept : ctrl_(c) {}
  // Finish
  Step_Result(Step_Ctrl c, T v) noexcept
      : ctrl_(c), finish_val_(std::move(v)) {}
  // Err — named constructor avoids ambiguity when T == E
  static Step_Result make_err(E e) noexcept {
    Step_Result r{Step_Ctrl::Err};
    std::construct_at(&r.err_val_, std::move(e));
    return r;
  }
};

}  // namespace detail

// ─── StepCtx<T,E> ────────────────────────────────────────────────────────────
//
// Passed as the single argument to cstep lambdas.  Every method returns the
// same detail::Step_Result<T,E>, so the lambda return type is fully deduced
// without any explicit annotation.

template <typename T, typename E>
struct StepCtx {
  [[nodiscard]] detail::Step_Result<T, E> yield() const noexcept {
    return detail::Step_Result<T, E>{detail::Step_Ctrl::Yield};
  }
  [[nodiscard]] detail::Step_Result<T, E> done() const noexcept {
    return detail::Step_Result<T, E>{detail::Step_Ctrl::Done};
  }
  [[nodiscard]] detail::Step_Result<T, E> finish(T v) const noexcept {
    return detail::Step_Result<T, E>{detail::Step_Ctrl::Finish, std::move(v)};
  }
  [[nodiscard]] detail::Step_Result<T, E> err(E e) const noexcept {
    return detail::Step_Result<T, E>::make_err(std::move(e));
  }
};

// ─── Step wrappers ───────────────────────────────────────────────────────────

// cstep: lambda(StepCtx<T,E>) -> Step_Result<T,E>
template <typename Fn>
struct CstepT {
  using _is_chain_step_tag = void;
  Fn fn_;
};
template <typename Fn>
CstepT(Fn) -> CstepT<Fn>;

// cawait: transparent delegation to a sub-ChainTask (shares error type E)
template <typename Sub>
struct CawaitT {
  using _is_chain_step_tag = void;
  using sub_type = Sub;  // used for error-type compatibility check
  Sub* sub_;
};
template <typename Sub>
CawaitT(Sub&) -> CawaitT<Sub>;

// cresult: terminal step — lambda() -> T, always produces a success value
template <typename Fn>
struct CresultT {
  using _is_chain_step_tag = void;
  Fn fn_;
};
template <typename Fn>
CresultT(Fn) -> CresultT<Fn>;

// ─── Pipeline via | ──────────────────────────────────────────────────────────

template <typename... Steps>
struct ChainPipeline {
  std::tuple<Steps...> steps_;
};

namespace detail {
template <typename T>
inline constexpr bool is_chain_step_v =
    requires { typename T::_is_chain_step_tag; };
}  // namespace detail

// Constrained to avoid conflict with std::ranges::operator|.
template <typename A, typename B>
  requires(detail::is_chain_step_v<std::decay_t<A>> ||
           detail::is_chain_step_v<std::decay_t<B>>)
constexpr auto operator|(A&& a, B&& b) noexcept {
  return ChainPipeline<std::decay_t<A>, std::decay_t<B>>{
      std::tuple{std::forward<A>(a), std::forward<B>(b)}};
}

template <typename... A, typename B>
constexpr auto operator|(ChainPipeline<A...> p, B&& b) noexcept {
  return ChainPipeline<A..., std::decay_t<B>>{std::tuple_cat(
      std::move(p.steps_), std::tuple<std::decay_t<B>>{std::forward<B>(b)})};
}

// ─── Internal traits ─────────────────────────────────────────────────────────

namespace detail {

template <typename>
struct is_cstep : std::false_type {};
template <typename Fn>
struct is_cstep<CstepT<Fn>> : std::true_type {};

template <typename>
struct is_cawait : std::false_type {};
template <typename Sub>
struct is_cawait<CawaitT<Sub>> : std::true_type {};

template <typename>
struct is_cresult : std::false_type {};
template <typename Fn>
struct is_cresult<CresultT<Fn>> : std::true_type {};

}  // namespace detail

// ─── ChainTask ───────────────────────────────────────────────────────────────

template <typename T, typename E, typename Pipeline>
class ChainTask {
 public:
  // Public alias used by cawait compatibility checks.
  using error_type = E;

 private:
  static constexpr std::size_t Step_Count = std::tuple_size_v<
      std::remove_cvref_t<decltype(std::declval<Pipeline>().steps_)>>;
  static_assert(Step_Count > 0,
                "ChainTask pipeline must have at least one step");
  static_assert(std::is_default_constructible_v<T>,
                "ChainTask: T must be default-constructible (required by "
                "std::expected<T,E>{})");
  static_assert(std::is_trivially_destructible_v<T> &&
                    std::is_trivially_destructible_v<E>,
                "ChainTask: T and E must be trivially destructible "
                "(Step_Result uses a union)");

  using StepFn = bool (*)(ChainTask&) noexcept;

  Pipeline pl_;
  std::uint16_t idx_{0};
  bool finished_{false};
  std::expected<T, E> res_{};

  // One template instantiation per pipeline slot — compiler/LTO typically
  // inlines simple steps and retains only the table pointer.
  template <std::size_t I>
  static bool runStep(ChainTask& t) noexcept {
    auto& s = std::get<I>(t.pl_.steps_);
    using S = std::remove_cvref_t<decltype(s)>;

    if constexpr (detail::is_cstep<S>::value) {
      StepCtx<T, E> ctx;
      auto r = s.fn_(ctx);
      switch (r.ctrl_) {
        case detail::Step_Ctrl::Yield:
          return false;
        case detail::Step_Ctrl::Done:
          return true;
        case detail::Step_Ctrl::Finish:
          t.res_ = std::move(r.finish_val_);
          t.finished_ = true;
          return true;
        case detail::Step_Ctrl::Err:
          t.res_ = std::unexpected(std::move(r.err_val_));
          t.finished_ = true;
          return true;
      }
      std::unreachable();

    } else if constexpr (detail::is_cawait<S>::value) {
      if (!s.sub_->handle()) return false;
      const auto& sr = s.sub_->result();
      if (!sr.has_value()) {
        t.res_ = std::unexpected(sr.error());
        t.finished_ = true;
      }
      return true;

    } else if constexpr (detail::is_cresult<S>::value) {
      t.res_ = s.fn_();
      t.finished_ = true;
      return true;

    } else {
      static_assert(sizeof(S) == 0,
                    "Unknown step type — use cstep / cawait / cresult");
    }
  }

  // For short pipelines, avoid an indirect function-pointer call.
  // This is hot in nested-task workloads where Step_Count is usually 2-4.
  [[gnu::always_inline]] static bool runCurrentStep(ChainTask& t) noexcept {
    if constexpr (Step_Count == 1) {
      return runStep<0>(t);
    } else if constexpr (Step_Count == 2) {
      if (t.idx_ == 0) return runStep<0>(t);
      return runStep<1>(t);
    } else if constexpr (Step_Count == 3) {
      if (t.idx_ == 0) return runStep<0>(t);
      if (t.idx_ == 1) return runStep<1>(t);
      return runStep<2>(t);
    } else if constexpr (Step_Count == 4) {
      if (t.idx_ == 0) return runStep<0>(t);
      if (t.idx_ == 1) return runStep<1>(t);
      if (t.idx_ == 2) return runStep<2>(t);
      return runStep<3>(t);
    } else {
      // Jump table is declared only here, inside the if constexpr branch that
      // requires Step_Count > 4, so it is never instantiated for short
      // pipelines.
      static constexpr auto Step_Table =
          []<std::size_t... Is>(std::index_sequence<Is...>) constexpr noexcept {
            return std::array<StepFn, Step_Count>{&runStep<Is>...};
          }(std::make_index_sequence<Step_Count>{});
      return Step_Table[t.idx_](t);
    }
  }

 public:
  constexpr explicit ChainTask(Pipeline p) noexcept : pl_(std::move(p)) {}

  // Returns false while running, true when finished (success or error).
  // Synchronous steps (ctx.done()) chain within a single call.
  bool handle() noexcept {
    if (finished_) [[unlikely]]
      return true;
    while (idx_ < static_cast<std::uint16_t>(Step_Count)) {
      if (!runCurrentStep(*this)) return false;
      if (finished_) return true;
      ++idx_;
    }
    // Reaching here means every step returned ctx.done() without any step
    // calling ctx.finish(), ctx.err(), or being a cresult — programming error.
    assert(false &&
           "ChainTask: pipeline ended without result; "
           "last step must call ctx.finish() / ctx.err() or be cresult");
    finished_ = true;
    return true;
  }

  [[nodiscard]] constexpr bool finished() const noexcept { return finished_; }

  [[nodiscard]] constexpr const std::expected<T, E>& result() const noexcept {
    return res_;
  }

  void reset() noexcept {
    idx_ = 0;
    finished_ = false;
    res_ = {};
  }
};

// ─── Factory & step helpers ──────────────────────────────────────────────────

// makeChainTask<T, E>(pipeline) — deduces Pipeline, caller names only T and E.
template <typename T, typename E, typename P>
[[nodiscard]] constexpr auto makeChainTask(P pipeline) noexcept {
  return ChainTask<T, E, P>{std::move(pipeline)};
}

template <typename Fn>
[[nodiscard]] constexpr auto cstep(Fn f) noexcept {
  return CstepT<Fn>{std::move(f)};
}

template <typename Sub>
[[nodiscard]] constexpr auto cawait(Sub& s) noexcept {
  return CawaitT<Sub>{&s};
}

template <typename Fn>
[[nodiscard]] constexpr auto cresult(Fn f) noexcept {
  return CresultT<Fn>{std::move(f)};
}

}  // namespace m

#endif  // CHAIN_TASK_HPP