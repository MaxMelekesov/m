/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef S_CURVE_STEP_POSITIONER_HPP
#define S_CURVE_STEP_POSITIONER_HPP

#include <CoroDelay.hpp>
#include <CoroMutex.hpp>
#include <CoroScheduler.hpp>
#include <IEndstop.hpp>
#include <IStepCounter.hpp>
#include <IStepDriver.hpp>
#include <IStepGen.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <SAccCurve.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <utility>

namespace m {

// One-shot 1 ms tick S-curve positioner driving a hardware step generator.
//
// Behavior:
//   * startMove(d)    — atomically target += d (queueable on the fly).
//   * startMoveTo(p)  — atomically target  = p (overrides previous targets).
//   * softStop()      — smooth deceleration along the S-curve down to v_min,
//                       then stop. Subsequent startMove*/ resumes normally.
//   * emgStop()       — hard stop: kill generator first, then reset.
//   * moving()        — true while the generator is running (incl. settle /
//                       disable phases).
//
// All speed transitions follow SAccCurve (no step jumps); peak speed is
// limited automatically by the available distance (triangular profile when
// max_v can not be reached).
template <typename TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT,
          m::ifc::CEndstop EndstopT>
  requires m::ifc::CTime<TimeMsT> && m::ifc::CMs<typename TimeMsT::Unit>
class SCurveStepPositioner {
 private:
  using MsT = decltype(std::declval<TimeMsT&>().now());
  using StepT = typename StepGenT::Step;
  using DrvDir = typename StepDriverT::Dir;
  using CtrDir = typename StepCounterT::Dir;
  using mAT =
      std::remove_cvref_t<decltype(std::declval<StepDriverT&>().getCurrent())>;

 public:
  SCurveStepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                       StepGenT& gen, EndstopT& endstop)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen), endstop_(endstop) {
    ctr_.setCount(0);
  }

  ~SCurveStepPositioner() {
    emgStop();
    gen_.setCallback({});
  }

  bool moving() const { return gen_.running(); }

  m::Task<bool> startMove(int32_t steps) {
    auto guard = co_await mutex_.lock();
    soft_stop_.store(false, std::memory_order_release);
    target_pos_.fetch_add(steps, std::memory_order_acq_rel);
    co_return co_await ensureRunning();
  }

  m::Task<bool> startMoveTo(int32_t pos) {
    auto guard = co_await mutex_.lock();
    soft_stop_.store(false, std::memory_order_release);
    target_pos_.store(pos, std::memory_order_release);
    co_return co_await ensureRunning();
  }

  bool softStop() {
    soft_stop_.store(true, std::memory_order_release);
    return true;
  }

  bool emgStop() {
    epoch_.fetch_add(1, std::memory_order_acq_rel);
    soft_stop_.store(false, std::memory_order_relaxed);
    const bool res = gen_.stop();
    const int32_t pos = ctr_.getCount();
    loaded_pos_sync_.store(pos, std::memory_order_release);
    target_pos_.store(pos, std::memory_order_release);
    resetIsrState(pos);
    return res;
  }

  void reset(int32_t pos = 0) {
    emgStop();
    ctr_.setCount(pos);
    loaded_pos_sync_.store(pos, std::memory_order_release);
    target_pos_.store(pos, std::memory_order_release);
    resetIsrState(pos);
  }

  SAccCurve& getAccCurve() { return acc_curve_; }

  void setAutohold(bool v) {
    autohold_ = v;
    if (!v && !gen_.running() && drv_.getEnable()) drv_.setEnable(0);
  }
  bool getAutohold() const { return autohold_; }

  void setDriverEnDelay(MsT v) { driver_en_delay_ = v; }
  MsT getDriverEnDelay() const { return driver_en_delay_; }

  void setStopDelay(MsT v) { stop_delay_ = v; }
  MsT getStopDelay() const { return stop_delay_; }

  void setRunCurrent(mAT v) { run_current_ = v; }
  mAT getRunCurrent() const { return run_current_; }

  void setHoldCurrent(mAT v) { hold_current_ = v; }
  mAT getHoldCurrent() const { return hold_current_; }

 private:
  TimeMsT& time_;
  StepDriverT& drv_;
  StepCounterT& ctr_;
  StepGenT& gen_;
  EndstopT& endstop_;
  CoroMutex mutex_;

  SAccCurve acc_curve_{MsT{5'000}, 100, 100'000};
  MsT driver_en_delay_{10};
  MsT gen_drain_delay_{5};
  MsT stop_delay_{100};
  bool autohold_ = false;
  mAT run_current_{1'000};
  mAT hold_current_{500};

  // ── shared state (caller threads + ISR) ───────────────────────────────────
  std::atomic<int32_t> target_pos_{0};       // commanded absolute target
  std::atomic<int32_t> loaded_pos_sync_{0};  // ISR-published loaded position
  std::atomic<uint32_t> epoch_{0};           // invalidates pending prepare()
  std::atomic<bool> soft_stop_{false};

  // ── ISR-only state ────────────────────────────────────────────────────────
  enum class State : uint8_t {
    Idle,
    Running,
    ReverseDrain,  // wait for in-flight DMA chunks to flush before flipping DIR
    Settle,
    WaitDisable,
    Done,
  };
  State state_ = State::Idle;
  MsT phase_t_{0};          // point on S-curve, [0 .. acc_t_]
  int32_t loaded_pos_ = 0;  // position already scheduled to the generator
  uint32_t pending_steps_ = 0;
  uint32_t pending_freq_ = 0;
  float step_acc_ = 0.0f;  // fractional step carry across 1 ms ticks
  MsT settle_left_{0};
  MsT reverse_drain_left_{0};  // dummy ticks to drain the DMA pipeline
  bool first_step_ = false;    // first ISR call after start(); resync baseline

  static constexpr MsT Time_Step_{1};

  void resetIsrState(int32_t pos) {
    state_ = State::Idle;
    phase_t_ = MsT{0};
    loaded_pos_ = pos;
    pending_steps_ = 0;
    pending_freq_ = 0;
    step_acc_ = 0.0f;
    settle_left_ = MsT{0};
    reverse_drain_left_ = MsT{0};
    first_step_ = false;
  }

  m::Task<bool> ensureRunning() {
    const auto epoch = epoch_.load(std::memory_order_acquire);
    auto alive = [&] {
      return epoch == epoch_.load(std::memory_order_acquire);
    };

    if (!ctr_.running() && !ctr_.start()) co_return false;

    // Endstop check before enabling the driver — avoid wasted work.
    {
      const int32_t pos = ctr_.getCount();
      const int32_t target_now = target_pos_.load(std::memory_order_acquire);
      if (endstopBlocks(target_now - pos)) co_return false;
    }

    if (!drv_.getEnable()) {
      drv_.setEnable(1);
      drv_.setCurrent(run_current_);
      co_await m::coroDelay(time_, driver_en_delay_);
      if (!alive()) co_return false;
    }

    if (!gen_.running()) {
      // Generator is stopped → ISR is not running. It is safe to (a) replace
      // the std::function callback and (b) reset the ISR-only FSM so the
      // newly started generator does not pick up a stale State::Done from a
      // previously completed move.
      //
      // After emgStop() the timer/DMA pipeline may still emit 1–2 buffered
      // chunks for a couple of milliseconds. Wait for it to drain before
      // snapshotting the counter, otherwise we start the next move from a
      // stale `loaded_pos_` and overshoot by exactly the leaked steps.
      co_await m::coroDelay(time_, gen_drain_delay_);
      if (!alive()) co_return false;
      const int32_t pos = ctr_.getCount();
      loaded_pos_sync_.store(pos, std::memory_order_release);
      resetIsrState(pos);

      // Pre-arm DIR pin & counter direction here, *before* the generator
      // emits any pulse. If we leave this to the first ISR tick, the DIR
      // flip can race the rising edge of STEP and the counter will miss
      // (or double-count) one pulse.
      const int32_t target_now = target_pos_.load(std::memory_order_acquire);
      const int32_t diff_now = target_now - pos;
      if (diff_now != 0) setDirection(diff_now);

      gen_.setCallback([this]() -> StepT { return nextStep(); });
      first_step_ = true;
      if (!gen_.start()) co_return false;
    }
    co_return alive();
  }

  StepT nextStep() {
    // First ISR call after gen_.start() — re-baseline `loaded_pos_` from the
    // hardware counter to absorb any transient edge produced by enabling
    // PWM (CC1E toggle / OC1 idle-state change). Safe: we are in the ISR,
    // no concurrent tick() ran yet, and `loaded_pos_` has not been advanced.
    if (first_step_) {
      first_step_ = false;
      const int32_t pos = ctr_.getCount();
      loaded_pos_ = pos;
      loaded_pos_sync_.store(pos, std::memory_order_release);
    }

    if (pending_steps_ > 0) return emitChunk();

    const int32_t target = effectiveTarget();
    return tick(target);
  }

  int32_t effectiveTarget() {
    if (soft_stop_.load(std::memory_order_acquire)) {
      // Replace the target by "loaded_pos + braking distance" — this lets
      // the standard tick logic decelerate naturally along the S-curve.
      const int32_t brake =
          static_cast<int32_t>(std::roundf(acc_curve_.st(phase_t_)));
      const int32_t signed_brake =
          (drv_.getDirection() == DrvDir::Forward) ? brake : -brake;
      return loaded_pos_ + signed_brake;
    }
    return target_pos_.load(std::memory_order_acquire);
  }

  StepT tick(int32_t target) {
    switch (state_) {
      case State::Idle:
        return tickIdle(target);
      case State::Running:
        return tickRunning(target);
      case State::ReverseDrain:
        return tickReverseDrain(target);
      case State::Settle:
        return tickSettle(target);
      case State::WaitDisable:
        return tickWaitDisable(target);
      case State::Done:
        return idleTick();
    }
    return idleTick();
  }

  StepT tickIdle(int32_t target) {
    const int32_t diff = target - loaded_pos_;
    if (diff == 0) {
      settle_left_ = MsT{2};
      state_ = State::Settle;
      return idleTick();
    }
    setDirection(diff);
    phase_t_ = MsT{0};
    step_acc_ = 0.0f;
    state_ = State::Running;
    return idleTick();
  }

  StepT tickRunning(int32_t target) {
    const int32_t diff = target - loaded_pos_;
    const bool reverse = dirMismatch(diff);
    const uint32_t adiff = absU32(diff);

    // Steps required to decelerate from current v to 0 along the S-curve.
    const uint32_t brake_dist =
        static_cast<uint32_t>(std::roundf(acc_curve_.st(phase_t_)));
    const bool brake = reverse || adiff <= brake_dist;

    const MsT acc_t = acc_curve_.getAccT();
    const MsT t_next =
        brake ? (phase_t_ > Time_Step_ ? phase_t_ - Time_Step_ : MsT{0})
              : std::min(phase_t_ + Time_Step_, acc_t);

    // We've decelerated all the way to v=0.
    if (brake && phase_t_ == MsT{0}) {
      if (reverse) {
        // DIR pin must NOT be flipped while the generator still has forward
        // chunks queued in the DMA buffer (timer would emit them with the new
        // DIR, both motor and step counter would walk backwards).
        // Drain the pipeline first.
        state_ = State::ReverseDrain;
        reverse_drain_left_ = MsT{2};
        step_acc_ = 0.0f;
        return idleTick();
      }
      // True stop at the target (or with soft_stop active).
      state_ = State::Idle;
      return tick(target);
    }

    // Steps to emit in this 1 ms slice.
    float ds;
    const bool cruise = !brake && phase_t_ == acc_t && t_next == acc_t;
    if (cruise) {
      ds = acc_curve_.vt(t_next) * static_cast<float>(Time_Step_.value()) /
               1'000.0f +
           step_acc_;
    } else {
      ds = std::fabs(acc_curve_.st(t_next) - acc_curve_.st(phase_t_)) +
           step_acc_;
    }

    uint32_t steps = static_cast<uint32_t>(ds);
    step_acc_ = ds - static_cast<float>(steps);

    // Never overshoot the target.
    if (steps > adiff) {
      steps = adiff;
      step_acc_ = 0.0f;
    }

    phase_t_ = t_next;

    if (steps == 0) return idleTick();

    uint32_t freq = static_cast<uint32_t>(std::roundf(acc_curve_.vt(t_next)));
    if (freq == 0) freq = 1;

    pending_freq_ = freq;
    pending_steps_ = steps;
    return emitChunk();
  }

  StepT tickReverseDrain(int32_t target) {
    if (reverse_drain_left_ > MsT{0}) {
      --reverse_drain_left_;
      return idleTick();
    }
    const int32_t diff = target - loaded_pos_;
    if (diff == 0) {
      state_ = State::Idle;
      return tick(target);
    }
    setDirection(diff);
    state_ = State::Running;
    return idleTick();
  }

  StepT tickSettle(int32_t target) {
    const int32_t diff = target - loaded_pos_;
    if (diff != 0) {
      state_ = State::Idle;
      return tick(target);
    }
    if (settle_left_ > MsT{0}) {
      --settle_left_;
      return idleTick();
    }
    if (autohold_) {
      drv_.setCurrent(hold_current_);
      state_ = State::Done;
      gen_.stop();
      return idleTick();
    }
    state_ = State::WaitDisable;
    return StepT{.freq = 1'000, .steps = stop_delay_.value(), .dummy = true};
  }

  StepT tickWaitDisable(int32_t target) {
    const int32_t diff = target - loaded_pos_;
    if (diff != 0) {
      state_ = State::Idle;
      return tick(target);
    }
    drv_.setEnable(0);
    state_ = State::Done;
    gen_.stop();
    return idleTick();
  }

  StepT emitChunk() {
    const uint32_t cap = gen_.maxSteps();
    uint32_t s = pending_steps_;
    if (s > cap && s < cap * 2) {
      s = s / 2;
    } else {
      s = std::min(s, cap);
    }
    pending_steps_ -= s;
    loaded_pos_ +=
        (drv_.getDirection() == DrvDir::Forward) ? int32_t(s) : -int32_t(s);
    loaded_pos_sync_.store(loaded_pos_, std::memory_order_release);
    return StepT{.freq = pending_freq_, .steps = s, .dummy = false};
  }

  void setDirection(int32_t diff) {
    if (diff >= 0) {
      drv_.setDirection(DrvDir::Forward);
      ctr_.setDirection(CtrDir::Up);
    } else {
      drv_.setDirection(DrvDir::Backward);
      ctr_.setDirection(CtrDir::Down);
    }
  }

  bool dirMismatch(int32_t diff) const {
    return (diff > 0 && drv_.getDirection() == DrvDir::Backward) ||
           (diff < 0 && drv_.getDirection() == DrvDir::Forward);
  }

  // True if the endstop on the side we are about to move into is triggered.
  // swap / ignore / active-level are already accounted for by getSwitchesState.
  bool endstopBlocks(int32_t diff) const {
    if (diff == 0) return false;
    auto sw = endstop_.getSwitchesState();
    if (diff > 0 && sw.right) return true;
    if (diff < 0 && sw.left) return true;
    return false;
  }

  static StepT idleTick() {
    return StepT{.freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
  }

  static constexpr uint32_t absU32(int32_t x) {
    const uint32_t ux = static_cast<uint32_t>(x);
    return (x < 0) ? (0u - ux) : ux;
  }
};

template <m::ifc::CTime TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT,
          m::ifc::CEndstop EndstopT>
SCurveStepPositioner(TimeMsT&, StepDriverT&, StepCounterT&, StepGenT&,
                     EndstopT&)
    -> SCurveStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT,
                            EndstopT>;

}  // namespace m

#endif  // S_CURVE_STEP_POSITIONER_HPP
