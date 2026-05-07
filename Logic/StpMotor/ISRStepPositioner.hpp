/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISR_STEP_POSITIONER_HPP
#define ISR_STEP_POSITIONER_HPP

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

/// Minimal ISR-only S-curve step positioner.
///
/// All motion logic runs inside the ISR callback — tick() is the single entry
/// point called from the step generator's DMA interrupt. Users only set flags
/// via the public API; there are no coroutines, no mutex, no epoch tracking.
///
/// The generator starts in the constructor and runs continuously —
/// when idle it emits dummy 1 kHz ticks (no STEP pulse).
///
/// Usage:
///   1. Construct with references to hardware (generator starts immediately).
///   2. Call startMove() / startMoveTo() / softStop() / emgStop() from any
///      context — they just set atomic flags and return immediately.
///   3. The ISR handles driver enable/disable, S-curve acceleration,
///      deceleration, direction reversal, endstop checks, and autohold.
template <typename TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT,
          m::ifc::CEndstop EndstopT>
  requires m::ifc::CTime<TimeMsT> && m::ifc::CMs<typename TimeMsT::Unit>
class ISRStepPositioner {
 private:
  using MsT = decltype(std::declval<TimeMsT&>().now());
  using StepT = typename StepGenT::Step;
  using DrvDir = typename StepDriverT::Dir;
  using CtrDir = typename StepCounterT::Dir;
  using mAT =
      std::remove_cvref_t<decltype(std::declval<StepDriverT&>().getCurrent())>;

 public:
  ISRStepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                    StepGenT& gen, EndstopT& endstop)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen), endstop_(endstop) {
    ctr_.setCount(0);
    ctr_.start();
    resetIsrState();
    gen_.setCallback([this]() -> StepT { return tick(); });
    gen_.start();
  }

  ~ISRStepPositioner() {
    gen_.stop();
    gen_.setCallback({});
  }

  // ── User API (set flags, return immediately) ───────────────────────────

  /// Add @p steps to the current target.  Queuable on the fly.
  /// Restarts the generator automatically if it was stopped by emgStop/reset.
  void startMove(int32_t steps) {
    emg_active_.store(false, std::memory_order_release);
    soft_stop_req_.store(false, std::memory_order_release);
    if (!gen_.running()) gen_.start();
    target_pos_.fetch_add(steps, std::memory_order_acq_rel);
  }

  /// Override target to absolute position @p pos.
  void startMoveTo(int32_t pos) {
    emg_active_.store(false, std::memory_order_release);
    soft_stop_req_.store(false, std::memory_order_release);
    if (!gen_.running()) gen_.start();
    target_pos_.store(pos, std::memory_order_release);
  }

  /// Request smooth deceleration along the S-curve down to v_min, then stop.
  void softStop() { soft_stop_req_.store(true, std::memory_order_release); }

  /// Hard stop: kill driver, reset state.  Motion stops immediately.
  /// The generator is stopped; the next startMove/startMoveTo restarts it.
  void emgStop() { emg_stop_req_.store(true, std::memory_order_release); }

  /// Hard stop + set hardware counter and target to @p pos.
  /// All register writes are deferred to the ISR to avoid races.
  void reset(int32_t pos = 0) {
    reset_pos_.store(pos, std::memory_order_release);
    reset_req_.store(true, std::memory_order_release);
  }

  /// True while a move is active (accelerating, cruising, decelerating,
  /// reversing). False when idle, settled, or holding.
  bool moving() const {
    return moving_sync_.load(std::memory_order_acquire);
  }

  /// Snapshot of the ISR-published loaded position (for monitoring).
  int32_t getLoadedPos() const {
    return loaded_pos_sync_.load(std::memory_order_acquire);
  }

  // ── Configuration ──────────────────────────────────────────────────────

  SAccCurve& getAccCurve() { return acc_curve_; }

  void setAutohold(bool v) { autohold_ = v; }
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
  // ── References ─────────────────────────────────────────────────────────
  TimeMsT& time_;
  StepDriverT& drv_;
  StepCounterT& ctr_;
  StepGenT& gen_;
  EndstopT& endstop_;

  // ── Atomic flags (user → ISR) ──────────────────────────────────────────
  std::atomic<int32_t> target_pos_{0};
  std::atomic<bool> soft_stop_req_{false};
  std::atomic<bool> emg_stop_req_{false};
  std::atomic<bool> reset_req_{false};
  std::atomic<int32_t> reset_pos_{0};

  // ── Atomic flags (ISR → user) ──────────────────────────────────────────
  std::atomic<int32_t> loaded_pos_sync_{0};
  std::atomic<bool> moving_sync_{false};

  // ── emgStop guard (user ⟷ ISR) ─────────────────────────────────────────
  std::atomic<bool> emg_active_{false};

  // ── Configuration ──────────────────────────────────────────────────────
  SAccCurve acc_curve_{MsT{5'000}, 100, 100'000};
  MsT driver_en_delay_{10};
  MsT stop_delay_{100};
  bool autohold_ = false;
  mAT run_current_{1'000};
  mAT hold_current_{500};

  // ── ISR-only state ─────────────────────────────────────────────────────
  enum class State : uint8_t { Idle, WaitEnable, Run, WaitReverse, Settle, WaitDisable, Hold };
  State state_ = State::Idle;
  int32_t loaded_pos_ = 0;
  MsT phase_t_{0};
  float step_acc_ = 0.0f;
  uint32_t pending_steps_ = 0;
  uint32_t pending_freq_ = 0;
  MsT wait_left_{0};

  static constexpr MsT Time_Step_{1};

  void setState(State s) {
    state_ = s;
    moving_sync_.store(s == State::WaitEnable || s == State::Run ||
                           s == State::WaitReverse,
                       std::memory_order_release);
  }

  void resetIsrState() {
    setState(State::Idle);
    phase_t_ = MsT{0};
    step_acc_ = 0.0f;
    pending_steps_ = 0;
    pending_freq_ = 0;
    wait_left_ = MsT{0};
  }

  // ── ISR entry point ────────────────────────────────────────────────────

  StepT tick() {
    // 1. Reset request — highest priority (deferred from user context).
    //    Stops the generator to flush the DMA pipeline.  The next
    //    startMove/startMoveTo will restart it from user context.
    if (reset_req_.exchange(false, std::memory_order_acq_rel)) {
      drv_.setEnable(0);
      gen_.stop();
      const int32_t pos = reset_pos_.load(std::memory_order_acquire);
      ctr_.setCount(pos);
      loaded_pos_ = pos;
      target_pos_.store(pos, std::memory_order_release);
      loaded_pos_sync_.store(pos, std::memory_order_release);
      emg_active_.store(false, std::memory_order_release);
      resetIsrState();
      return idleTick();
    }

    // 2. Emergency stop — stop the generator but do NOT overwrite
    //    target_pos_ (a concurrent startMove may have already updated it).
    //    Block tickIdle via emg_active_ until the next startMove clears it.
    if (emg_stop_req_.exchange(false, std::memory_order_acq_rel)) {
      drv_.setEnable(0);
      gen_.stop();
      loaded_pos_ = ctr_.getCount();
      loaded_pos_sync_.store(loaded_pos_, std::memory_order_release);
      emg_active_.store(true, std::memory_order_release);
      resetIsrState();
      return idleTick();
    }

    // 3. Drain pending chunk (multi-burst emission of the same 1 ms slice).
    if (pending_steps_ > 0) return emitChunk();

    // 4. Fetch the current command.
    int32_t target = target_pos_.load(std::memory_order_acquire);

    // 5. Soft-stop is a continuous override — do NOT consume the flag.
    //    It stays active until startMove/startMoveTo clears it.
    if (soft_stop_req_.load(std::memory_order_acquire)) {
      target = softStopTarget();
    }

    // 6. Dispatch.
    switch (state_) {
      case State::Idle:        return tickIdle(target);
      case State::WaitEnable:  return tickWaitEnable(target);
      case State::Run:         return tickRun(target);
      case State::WaitReverse: return tickWaitReverse(target);
      case State::Settle:      return tickSettle(target);
      case State::WaitDisable: return tickWaitDisable(target);
      case State::Hold:        return tickHold(target);
    }
    return idleTick();
  }

  // ── State handlers ─────────────────────────────────────────────────────

  StepT tickIdle(int32_t target) {
    if (target == loaded_pos_) return idleTick();
    if (emg_active_.load(std::memory_order_acquire)) return idleTick();
    if (endstopBlocks(target - loaded_pos_)) return idleTick();

    setDirection(target - loaded_pos_);
    drv_.setEnable(1);
    drv_.setCurrent(run_current_);
    wait_left_ = driver_en_delay_;
    setState(State::WaitEnable);
    return idleTick();
  }

  StepT tickWaitEnable(int32_t target) {
    if (target == loaded_pos_) {
      drv_.setEnable(0);
      setState(State::Idle);
      return idleTick();
    }
    if (wait_left_ > MsT{0}) {
      --wait_left_;
      return idleTick();
    }
    phase_t_ = MsT{0};
    step_acc_ = 0.0f;
    setState(State::Run);
    return tickRun(target);
  }

  StepT tickRun(int32_t target) {
    const int32_t diff = target - loaded_pos_;
    const bool reverse = dirMismatch(diff);
    const uint32_t adiff = absU32(diff);

    const uint32_t brake_dist =
        static_cast<uint32_t>(std::roundf(acc_curve_.st(phase_t_)));
    const bool brake = reverse || adiff <= brake_dist;

    const MsT acc_t = acc_curve_.getAccT();
    const MsT t_next =
        brake ? (phase_t_ > Time_Step_ ? phase_t_ - Time_Step_ : MsT{0})
              : std::min(phase_t_ + Time_Step_, acc_t);

    // Decelerated to v = 0.
    if (brake && phase_t_ == MsT{0}) {
      if (reverse) {
        wait_left_ = stop_delay_;
        setState(State::WaitReverse);
        return idleTick();
      }
      setState(State::Settle);
      wait_left_ = MsT{2};
      return idleTick();
    }

    // Steps to emit in this 1 ms slice.
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

  StepT tickWaitReverse(int32_t target) {
    if (wait_left_ > MsT{0}) {
      --wait_left_;
      return idleTick();
    }
    const int32_t diff = target - loaded_pos_;
    if (diff == 0) {
      // Target was set to current position during the wait — stop.
      setState(State::Settle);
      wait_left_ = MsT{2};
      return idleTick();
    }
    setDirection(diff);
    phase_t_ = MsT{0};
    step_acc_ = 0.0f;
    setState(State::Run);
    return tickRun(target);
  }

  StepT tickSettle(int32_t target) {
    if (target != loaded_pos_) {
      setState(State::Idle);
      return tickIdle(target);
    }
    if (wait_left_ > MsT{0}) {
      --wait_left_;
      return idleTick();
    }
    if (autohold_) {
      drv_.setCurrent(hold_current_);
      setState(State::Hold);
      return idleTick();
    }
    if (driver_en_delay_ > MsT{0}) {
      wait_left_ = driver_en_delay_;
      setState(State::WaitDisable);
      return idleTick();
    }
    drv_.setEnable(0);
    setState(State::Idle);
    return idleTick();
  }

  StepT tickWaitDisable(int32_t target) {
    if (target != loaded_pos_) {
      // New command arrived — cancel the disable delay and start moving.
      setState(State::Idle);
      return tickIdle(target);
    }
    if (wait_left_ > MsT{0}) {
      --wait_left_;
      return idleTick();
    }
    drv_.setEnable(0);
    setState(State::Idle);
    return idleTick();
  }

  StepT tickHold(int32_t target) {
    if (target != loaded_pos_) {
      if (endstopBlocks(target - loaded_pos_)) return idleTick();
      // Driver is already enabled; skip the enable delay.
      setDirection(target - loaded_pos_);
      drv_.setCurrent(run_current_);
      phase_t_ = MsT{0};
      step_acc_ = 0.0f;
      setState(State::Run);
      return tickRun(target);
    }
    return idleTick();
  }

  // ── Helpers ────────────────────────────────────────────────────────────

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

  bool endstopBlocks(int32_t diff) const {
    if (diff == 0) return false;
    auto sw = endstop_.getSwitchesState();
    if (diff > 0 && sw.right) return true;
    if (diff < 0 && sw.left) return true;
    return false;
  }

  int32_t softStopTarget() {
    const int32_t brake =
        static_cast<int32_t>(std::roundf(acc_curve_.st(phase_t_)));
    return loaded_pos_ +
           ((drv_.getDirection() == DrvDir::Forward) ? brake : -brake);
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
ISRStepPositioner(TimeMsT&, StepDriverT&, StepCounterT&, StepGenT&, EndstopT&)
    -> ISRStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT,
                         EndstopT>;

}  // namespace m

#endif  // ISR_STEP_POSITIONER_HPP
