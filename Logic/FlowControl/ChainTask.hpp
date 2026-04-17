/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#pragma once

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
 *         N×4-byte read-only jump table (Step_Table) in .rodata.
 *
 *  CPU  : O(1) per handle() call — single Step_Table[idx_](*this) indirect
 *         call.  Back-to-back synchronous (ctx.done()) steps are chained in
 *         one handle() without returning to the caller.
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
 *   auto task = m::makeChainTask<bool, Err>(
 *       m::cawait(sub) |
 *       m::cstep([&](auto ctx) {
 *           if (!sub.result().has_value())
 *               return ctx.err(sub.result().error());
 *           return ctx.done();
 *       }) |
 *       m::cresult([&]{ return true; }));
 *
 *   while (!task.handle()) { ... }
 *   auto res = task.result(); // std::expected<bool, Err>
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

namespace m {

// ─── Internal ────────────────────────────────────────────────────────────────

namespace detail {

enum class Step_Ctrl : uint8_t { Yield, Done, Finish, Err };

template <typename T, typename E>
struct Step_Result {
  Step_Ctrl ctrl_;
  T finish_val_{};
  E err_val_{};
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
    return {detail::Step_Ctrl::Yield};
  }
  [[nodiscard]] detail::Step_Result<T, E> done() const noexcept {
    return {detail::Step_Ctrl::Done};
  }
  [[nodiscard]] detail::Step_Result<T, E> finish(T v) const noexcept {
    return {detail::Step_Ctrl::Finish, std::move(v)};
  }
  [[nodiscard]] detail::Step_Result<T, E> err(E e) const noexcept {
    return {detail::Step_Ctrl::Err, {}, std::move(e)};
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
  static constexpr std::size_t Step_Count = std::tuple_size_v<
      std::remove_cvref_t<decltype(std::declval<Pipeline>().steps_)>>;
  static_assert(Step_Count > 0,
                "ChainTask pipeline must have at least one step");

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
      auto sr = s.sub_->result();
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

  template <std::size_t... Is>
  static constexpr std::array<StepFn, Step_Count> makeTable(
      std::index_sequence<Is...>) noexcept {
    return {&runStep<Is>...};
  }

  // O(1) jump table in .rodata
  static constexpr std::array<StepFn, Step_Count> Step_Table =
      makeTable(std::make_index_sequence<Step_Count>{});

 public:
  constexpr explicit ChainTask(Pipeline p) noexcept : pl_(std::move(p)) {}

  // Returns false while running, true when finished (success or error).
  // Synchronous steps (ctx.done()) chain within a single call.
  bool handle() noexcept {
    if (finished_) [[unlikely]] return true;
    while (idx_ < static_cast<std::uint16_t>(Step_Count)) {
      if (!Step_Table[idx_](*this)) return false;
      if (finished_) return true;
      ++idx_;
    }
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
