/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef LINEAR_STEP_POSITIONER_HPP
#define LINEAR_STEP_POSITIONER_HPP
#include <CoroDelay.hpp>
#include <CoroScheduler.hpp>
#include <IStepCounter.hpp>
#include <IStepDriver.hpp>
#include <IStepGen.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Timer.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <utility>

namespace m {

template <m::ifc::CTimeMs TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
class LinearStepPositioner {
 private:
  using TimeMsUnit = decltype(std::declval<TimeMsT&>().getTick());

 public:
  LinearStepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                       StepGenT& gen)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen) {
    setSpeed(1'500);
  }

  m::Task<void> coroRun() {
    if (start_pending_) {
      start_pending_ = false;
      co_await m::coroDelay(time_, TimeMsUnit{10});
    } else {
      co_return;
    }

    gen_.setCallback([&]() {
      int32_t pending = pending_steps_.load(std::memory_order_acquire);
      if (!pending) {
        return typename StepGenT::Step{.freq = 0, .steps = 0};
      }

      typename StepDriverT::Dir desired_dir = (pending > 0)
                                                  ? StepDriverT::Dir::Forward
                                                  : StepDriverT::Dir::Backward;
      if (desired_dir != current_dir_) {
        drv_.setDirection(desired_dir);
        ctr_.setDirection((desired_dir == StepDriverT::Dir::Forward)
                              ? StepCounterT::Dir::Up
                              : StepCounterT::Dir::Down);
        current_dir_ = desired_dir;
      }

      uint32_t v = v_.load(std::memory_order_acquire);
      if (v != last_v_) {
        spms_ = calcStepsPerMs(v);
        last_v_ = v;
      }

      uint32_t remaining =
          static_cast<uint32_t>((pending > 0) ? pending : -pending);
      uint32_t chunk = std::min(spms_, remaining);
      if (!chunk) {
        return typename StepGenT::Step{.freq = 0, .steps = 0};
      }

      int32_t delta = (pending > 0) ? -static_cast<int32_t>(chunk)
                                    : static_cast<int32_t>(chunk);
      pending_steps_.fetch_add(delta, std::memory_order_acq_rel);

      return typename StepGenT::Step{.freq = v, .steps = chunk};
    });

    if (!ctr_.running()) {
      if (!ctr_.start()) co_return;
    }
    if (!gen_.start()) co_return;

    co_await m::coroUntil([&]() { return !moving(); });

    if (!autohold_) {
      drv_.setEnable(0);
    }
  }

  bool moving() { return gen_.running(); }

  bool addSteps(int32_t steps) {
    int32_t prev = pending_steps_.fetch_add(steps, std::memory_order_acq_rel);
    int32_t next = prev + steps;

    if (next != 0 && !gen_.running()) {
      typename StepDriverT::Dir dir =
          (next > 0) ? StepDriverT::Dir::Forward : StepDriverT::Dir::Backward;
      drv_.setDirection(dir);
      ctr_.setDirection((dir == StepDriverT::Dir::Forward)
                            ? StepCounterT::Dir::Up
                            : StepCounterT::Dir::Down);
      current_dir_ = dir;

      drv_.setEnable(1);
      start_pending_ = true;
    }

    return true;
  }

  bool softStop() {
    pending_steps_.store(0, std::memory_order_release);
    return true;
  }

  bool emgStop() {
    pending_steps_.store(0, std::memory_order_release);
    return gen_.stop();
  }

  bool setSpeed(uint32_t value) {
    v_.store(value, std::memory_order_release);
    return true;
  }

  void setAutohold(bool value) { autohold_ = value; }
  bool getAutohold() { return autohold_; }

 private:
  TimeMsT& time_;
  StepDriverT& drv_;
  StepCounterT& ctr_;
  StepGenT& gen_;

  bool autohold_ = false;
  bool start_pending_ = false;

  uint32_t calcStepsPerMs(uint32_t v) const {
    float temp = static_cast<float>(v);
    temp = std::ceilf(temp / 1'000.0f);
    uint32_t spms = (temp > 1.0f) ? static_cast<uint32_t>(temp) : 1u;
    uint32_t max_steps = gen_.maxSteps();
    if (max_steps && spms > max_steps) {
      spms = max_steps;
    }
    return spms;
  }

  std::atomic<int32_t> pending_steps_{0};
  std::atomic<uint32_t> v_{1'500};
  uint32_t last_v_ = 0;
  uint32_t spms_ = 1;
  typename StepDriverT::Dir current_dir_ = StepDriverT::Dir::Forward;
};

template <m::ifc::CTimeMs TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
LinearStepPositioner(TimeMsT&, StepDriverT&, StepCounterT&, StepGenT&)
    -> LinearStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT>;
}  // namespace m

#endif  // LINEAR_STEP_POSITIONER_HPP
