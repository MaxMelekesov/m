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
#include <IStepCounter.hpp>
#include <IStepDriver.hpp>
#include <IStepGen.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <cmath>
#include <cstdint>

namespace m {

// TODO: template & concepts
template <m::ifc::CTimeMs TimeMsT, m::ifc::CStepDriver StepDriverT,
          m::ifc::CStepCounter StepCounterT, m::ifc::CStepGen StepGenT>
class LinearStepPositioner {
 private:
 public:
  LinearStepPositioner(TimeMsT& time, StepDriverT& drv, StepCounterT& ctr,
                       StepGenT& gen)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen) {
    gen_.setCallback([&]() {
      if (steps_to_load_) {
        if (v_ != last_v_) {
          float temp = v_;
          temp = std::ceilf(temp / 1'000.0f);
          spms_ = temp;
          if (!spms_) {
            spms_ = 1;
          }
          last_v_ = v_;
        }
        if (steps_to_load_ >= spms_) {
          steps_to_load_ -= spms_;
          return typename StepGenT::Step{.freq = v_, .steps = spms_};
        } else {
          uint32_t steps = steps_to_load_;
          steps_to_load_ = 0;
          return typename StepGenT::Step{.freq = v_, .steps = steps};
        }
      }
      return typename StepGenT::Step{.freq = 0, .steps = 0};
    });

    setSpeed(1'500);
  }

  void handle() {
    if (!autohold_) {
      if (!moving()) {
        drv_.setEnable(0);
      }
    }
  }

  bool moving() { return gen_.running(); }

  bool addSteps(int32_t steps) {
    if (moving()) return false;

    if (steps == 0) return true;

    if (steps > 0) {
      drv_.setDirection(StepDriverT::Dir::Forward);
      ctr_.setDirection(StepCounterT::Dir::Up);
    } else {
      drv_.setDirection(StepDriverT::Dir::Backward);
      ctr_.setDirection(StepCounterT::Dir::Down);
    }

    drv_.setEnable(1);
    time_.delay(Ms<uint32_t>{10});

    steps_to_load_ = std::abs(steps);

    if (!ctr_.running()) {
      if (!ctr_.start()) return false;
    }
    if (!gen_.start()) return false;

    return true;
  }

  bool softStop() {
    steps_to_load_ = 0;
    return true;
  }

  bool emgStop() { return gen_.stop(); }

  bool setSpeed(uint32_t value) {
    v_ = value;

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

  uint32_t steps_to_load_ = 0;
  uint32_t v_ = 1'500;
  uint32_t last_v_ = 1'500;
  uint32_t spms_ = 0;
};
}  // namespace m

#endif  // LINEAR_STEP_POSITIONER_HPP
