/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef STEP_POSITIONER_HPP
#define STEP_POSITIONER_HPP
#include <CoroDelay.hpp>
#include <CoroMutex.hpp>
#include <CoroScheduler.hpp>
#include <FinalAction.hpp>
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

template <m::ifc::CTimeMs TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
class StepPositioner {
 private:
  using MsT = decltype(std::declval<TimeMsT&>().getTick());
  using StepT = typename StepGenT::Step;

 public:
  StepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                 StepGenT& gen)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen) {
    ctr_.setCount(0);
    gen_.setCallback([&]() -> StepT { return nextStep(); });
  }

  ~StepPositioner() { emgStop(); }

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

    target_pos_.fetch_add(steps, std::memory_order_relaxed);

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
    target_pos_.store(0, std::memory_order_relaxed);
    return true;
  }
  bool emgStop() {
    pending_epoch_.fetch_add(1, std::memory_order_acq_rel);
    target_pos_.store(0, std::memory_order_relaxed);
    bool res = gen_.stop();
    return res;
  }

  SAccCurve& getAccCurve() { return acc_curve_; }

  void setAutohold(bool value) { autohold_ = value; }
  bool getAutohold() { return autohold_; }

  void setStopDelay(MsT ms) { stop_delay_ = ms; }
  MsT getStopDelay() const { return stop_delay_; }

 public:
  TimeMsT& time_;
  StepDriverT& drv_;
  StepCounterT& ctr_;
  StepGenT& gen_;

  CoroMutex mutex_;

  SAccCurve acc_curve_{MsT{250}, 4'000, 40'000};
  bool autohold_ = false;
  MsT driver_en_delay_{10};
  MsT stop_delay_{100};

  std::atomic<int32_t> target_pos_{0};
  std::atomic<uint32_t> pending_epoch_{0};

  MsT t_{0};
  int32_t loaded_pos_{0};
  uint32_t speed_{0};
  uint32_t steps_to_load_{0};

  MsT stop_counter_{0};

  uint32_t last_st_{0};
  float step_acc_{0.0f};
  uint32_t next_deacc_speed_{0};

  static constexpr MsT Time_Step_{1};

  enum class State : uint8_t {
    Idle,
    Acc,
    LoadAcc,
    Linear,
    LoadLinear,
    Deacc,
    LoadDeacc,
    Stop,
    WaitStop,
  };
  State state_ = State::Idle;

  StepT nextStep() {
    auto target = target_pos_.load(std::memory_order_acquire);

    return fsm(target);
  }

  StepT fsm(int32_t target) {
    int32_t diff = target - loaded_pos_;
    switch (state_) {
      case State::Idle: {
        if (diff == 0) {
          state_ = State::Stop;
          stop_counter_ = MsT{2};
          return StepT{
              .freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
        }

        if (diff > 0) {
          drv_.setDirection(StepDriverT::Dir::Forward);
          ctr_.setDirection(StepCounterT::Dir::Up);
        } else {
          drv_.setDirection(StepDriverT::Dir::Backward);
          ctr_.setDirection(StepCounterT::Dir::Down);
        }

        last_st_ = 0;
        step_acc_ = 0.0f;
        state_ = State::Acc;
        return StepT{.freq = 1'000, .steps = Time_Step_.value(), .dummy = true};
      } break;
      case State::Acc: {
        uint32_t speed = std::roundf(acc_curve_.vt(t_));
        uint32_t predicted_timestep = std::ceilf(1'000.0f / speed);
        MsT timestep = std::max(MsT{predicted_timestep}, Time_Step_);
        MsT t = t_ + timestep;

        // uint32_t st = std::roundf(acc_curve_.st(t_));
        // if (abs_u32(diff) <= st || dirChanged(diff)) {
        //   state_ = State::Deacc;
        //   return fsm(target);
        // }

        if (t > acc_curve_.getAccT()) {
          t_ = acc_curve_.getAccT();
          state_ = State::Linear;
          return fsm(target);
        }

        float temp = acc_curve_.st(t) + step_acc_;
        uint32_t st_new = static_cast<uint32_t>(temp);
        step_acc_ = temp - st_new;
        if (abs_u32(diff) < st_new + st_new - last_st_) {
          state_ = State::Linear;
          return fsm(target);
        } else {
          speed_ = speed;
          steps_to_load_ = st_new - last_st_;
        }

        t_ = t;
        last_st_ = st_new;
        state_ = State::LoadAcc;
        return fsm(target);
      } break;
      case State::LoadAcc: {
        auto step = fsmLoad();
        if (steps_to_load_ == 0) {
          state_ = State::Acc;
        }
        return step;
      } break;

      case State::Linear: {
        if (abs_u32(diff) <= last_st_) {
          next_deacc_speed_ = speed_;
          step_acc_ = 0.0f;
          state_ = State::Deacc;
          return fsm(target);
        }

        steps_to_load_ = speed_ * Time_Step_.value() / 1'000;
        if (steps_to_load_ == 0) steps_to_load_ = 1;

        if (abs_u32(diff) < last_st_ + steps_to_load_) {
          steps_to_load_ = abs_u32(diff) - last_st_;
        }

        state_ = State::LoadLinear;
        return fsm(target);
      } break;
      case State::LoadLinear: {
        auto step = fsmLoad();
        if (steps_to_load_ == 0) {
          state_ = State::Linear;
        }
        return step;
      } break;

      case State::Deacc: {
        speed_ = next_deacc_speed_;
        uint32_t predicted_timestep = std::ceilf(1'000.0f / speed_);
        MsT timestep = std::max(MsT{predicted_timestep}, Time_Step_);
        MsT t = (t_ > timestep) ? t_ - timestep : MsT{0};

        if (t_ == MsT{0}) {
          state_ = State::Idle;
          return fsm(target);
        }

        float temp = acc_curve_.st(t) + step_acc_;
        uint32_t st_new = static_cast<uint32_t>(temp);
        step_acc_ = temp - st_new;
        steps_to_load_ = last_st_ - st_new;

        next_deacc_speed_ = std::roundf(acc_curve_.vt(t));

        t_ = t;
        last_st_ = st_new;
        state_ = State::LoadDeacc;
        return fsm(target);
      } break;
      case State::LoadDeacc: {
        auto step = fsmLoad();
        if (steps_to_load_ == 0) {
          state_ = State::Deacc;
        }
        return step;
      } break;

      case State::Stop: {
        if (abs_u32(diff) != 0) {
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
        if (abs_u32(diff) != 0) {
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

  StepT fsmLoad() {
    uint32_t steps = 0;
    if (steps_to_load_ > gen_.maxSteps() &&
        steps_to_load_ < gen_.maxSteps() * 2) {
      steps = steps_to_load_ / 2;
    } else {
      steps = std::min(steps_to_load_, gen_.maxSteps());
    }
    steps_to_load_ -= steps;
    loaded_pos_ +=
        (drv_.getDirection() == StepDriverT::Dir::Forward) ? steps : -steps;
    return StepT{.freq = speed_, .steps = steps, .dummy = false};
  }

  bool dirChanged(int32_t diff) {
    return (diff > 0 && drv_.getDirection() == StepDriverT::Dir::Backward) ||
           (diff < 0 && drv_.getDirection() == StepDriverT::Dir::Forward);
  }

  constexpr uint32_t abs_u32(int32_t x) {
    const uint32_t ux = static_cast<uint32_t>(x);
    return (x < 0) ? (0u - ux) : ux;
  }
};
template <m::ifc::CTimeMs TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
StepPositioner(TimeMsT&, StepDriverT&, StepCounterT&, StepGenT&)
    -> StepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT>;
}  // namespace m

#endif  // STEP_POSITIONER_HPP
