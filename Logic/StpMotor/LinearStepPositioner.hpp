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
#include <CoroMutex.hpp>
#include <CoroScheduler.hpp>
#include <FinalAction.hpp>
#include <IStepCounter.hpp>
#include <IStepDriver.hpp>
#include <IStepGen.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace m {

template <typename TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
  requires m::ifc::CTime<TimeMsT> && m::ifc::CMs<typename TimeMsT::Unit>
class LinearStepPositioner {
 private:
  using MsT = decltype(std::declval<TimeMsT&>().now());
  using StepT = typename StepGenT::Step;

 public:
  LinearStepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                       StepGenT& gen)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen) {
    ctr_.setCount(0);
    gen_.setCallback([&]() -> StepT { return nextStep(); });
  }

  ~LinearStepPositioner() { emgStop(); }

  bool moving() { return gen_.running(); }

  m::Task<bool> startMove(int32_t steps) {
    co_await mutex_.lock();

    if (!ctr_.running()) {
      if (!ctr_.start()) co_return false;
    }

    if (!drv_.getEnable()) {
      drv_.setEnable(1);
      co_await m::coroDelay(time_, driver_en_delay_);
    }

    if (!gen_.running()) {
      if (!gen_.start()) co_return false;
    }

    pending_steps_.fetch_add(steps, std::memory_order_relaxed);

    co_return true;
  }

  m::Task<bool> startMoveTo(int32_t pos) {
    if (moving()) co_return false;
    auto current = ctr_.getCount();
    auto res = co_await startMove(pos - current);
    co_return res;
  }

  bool softStop() {
    pending_epoch_.fetch_add(1, std::memory_order_acq_rel);
    pending_steps_.store(0, std::memory_order_relaxed);
    return true;
  }
  bool emgStop() {
    pending_epoch_.fetch_add(1, std::memory_order_acq_rel);
    pending_steps_.store(0, std::memory_order_relaxed);
    bool res = gen_.stop();
    return res;
  }

  void setSpeed(uint32_t value) { speed_ = value; }
  uint32_t getSpeed() const { return speed_; }

  void setAutohold(bool value) { autohold_ = value; }
  bool getAutohold() { return autohold_; }

  void setStopDelay(MsT ms) { stop_delay_ = ms; }
  MsT getStopDelay() const { return stop_delay_; }

 private:
  TimeMsT& time_;
  StepDriverT& drv_;
  StepCounterT& ctr_;
  StepGenT& gen_;

  CoroMutex mutex_;

  uint32_t speed_ = 3'000;
  bool autohold_ = false;
  MsT driver_en_delay_{10};
  MsT stop_delay_{100};

  std::atomic<int32_t> pending_steps_{0};
  std::atomic<uint32_t> pending_epoch_{0};

  MsT stop_counter_{0};

  static constexpr MsT Time_Step_{1};

  enum class State : uint8_t {
    Idle,
    Moving,
    Stop,
    WaitStop,
  };
  State state_ = State::Idle;

  StepT nextStep() {
    const auto pending_epoch = pending_epoch_.load(std::memory_order_acquire);
    auto pending = pending_steps_.exchange(0, std::memory_order_relaxed);

    auto update_pending = m::finally([&] {
      if (pending == 0) return;
      if (pending_epoch_.load(std::memory_order_acquire) != pending_epoch)
        return;
      pending_steps_.fetch_add(pending, std::memory_order_relaxed);
    });

    switch (state_) {
      case State::Idle: {
        if (pending == 0) {
          state_ = State::Stop;
          stop_counter_ = MsT{2};
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        }

        if (pending > 0) {
          drv_.setDirection(StepDriverT::Dir::Forward);
          ctr_.setDirection(StepCounterT::Dir::Up);
        } else {
          drv_.setDirection(StepDriverT::Dir::Backward);
          ctr_.setDirection(StepCounterT::Dir::Down);
        }

        state_ = State::Moving;
        return StepT{.freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
      } break;
      case State::Moving: {
        if (pending == 0) {
          state_ = State::Stop;
          stop_counter_ = MsT{2};
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        }

        if ((pending > 0 &&
             drv_.getDirection() == StepDriverT::Dir::Backward) ||
            (pending < 0 && drv_.getDirection() == StepDriverT::Dir::Forward)) {
          state_ = State::Idle;

          return StepT{
              .freq = 1'000, .steps = stop_delay_.value(), .dummy = true};
        }

        uint32_t steps = std::min(abs_u32(pending), gen_.maxSteps());
        steps = std::min(steps, sT(Time_Step_));
        int32_t delta = (pending > 0) ? static_cast<int32_t>(steps)
                                      : -static_cast<int32_t>(steps);
        pending -= delta;

        return StepT{.freq = speed_,
                     .steps = static_cast<uint32_t>(steps),
                     .dummy = false};
      } break;
      case State::Stop: {
        if (pending != 0) {
          state_ = State::Idle;
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        }

        if (stop_counter_ > MsT{0}) {
          --stop_counter_;
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        } else {
          if (autohold_) {
            gen_.stop();
            state_ = State::Idle;
            return StepT{
                .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
          } else {
            state_ = State::WaitStop;
            return StepT{.freq = 1'000,
                         .steps = driver_en_delay_.value(),
                         .dummy = true};
          }
        }
      } break;
      case State::WaitStop: {
        if (pending != 0) {
          state_ = State::Idle;
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        }
        drv_.setEnable(0);
        gen_.stop();
        state_ = State::Idle;
        return StepT{.freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
      } break;
      default: {
        gen_.stop();
        state_ = State::Idle;
        return StepT{.freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
      } break;
    }
  }

  uint32_t sT(MsT ms) {
    auto steps = ms.value() * ((speed_ / 1'000) + 1);
    return steps;
  }
  constexpr uint32_t abs_u32(int32_t x) {
    const uint32_t ux = static_cast<uint32_t>(x);
    return (x < 0) ? (0u - ux) : ux;
  }
};

template <m::ifc::CTime TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
LinearStepPositioner(TimeMsT&, StepDriverT&, StepCounterT&, StepGenT&)
    -> LinearStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT>;
}  // namespace m

#endif  // LINEAR_STEP_POSITIONER_HPP
