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
#include <IStepDriverCtrl.hpp>
#include <IStepGen.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <cmath>
#include <cstdint>

namespace m {

// TODO: template & concepts
class LinearStepPositioner {
 private:
  using DrvT = m::ifc::IStepDriverCtrl<mA<uint32_t>>;
  using CtrT = m::ifc::IStepCounter;
  using GenT = m::ifc::IStepGen;

 public:
  LinearStepPositioner(m::ifc::ITime<Ms<uint32_t>>& time, DrvT& drv, CtrT& ctr,
                       GenT& gen)
      : time_(time), drv_(drv), ctr_(ctr), gen_(gen) {
    gen_.setCallback([&]() {
      if (steps_to_load_) {
        if (steps_to_load_ >= spms_) {
          steps_to_load_ -= spms_;
          return GenT::Step{.freq = v_, .steps = spms_};
        } else {
          uint32_t steps = steps_to_load_;
          steps_to_load_ = 0;
          return GenT::Step{.freq = v_, .steps = steps};
        }
      }
      return GenT::Step{.freq = 0, .steps = 0};
    });

    setSpeed(1'500);
  }

  void handle() {
    if (!moving()) {
      drv_.setEnable(0);
    }
  }

  bool moving() { return gen_.running(); }

  bool addSteps(int32_t steps) {
    if (moving()) return false;

    if (steps == 0) return true;

    if (steps > 0) {
      drv_.setDirection(DrvT::Dir::Forward);
      ctr_.setDirection(CtrT::Dir::Up);
    } else {
      drv_.setDirection(DrvT::Dir::Backward);
      ctr_.setDirection(CtrT::Dir::Down);
    }

    drv_.setMicrostep(DrvT::Microstep::M_8);
  //  drv_.setEnable(1);
    time_.delay(Ms<uint32_t>{10});

    steps_to_load_ = std::abs(steps);

    if (!ctr_.start()) return false;
    if (!gen_.start()) return false;

    return true;
  }

  bool emgStop() { return gen_.stop(); }

  bool setSpeed(uint32_t value) {
    // TODO: fix race v_ & smps_
    v_ = value;
    float temp = v_;
    temp = std::ceil(temp / 1'000.0f);
    spms_ = temp;
    if (!spms_) spms_ = 1;
    return true;
  }

 private:
  m::ifc::ITime<Ms<uint32_t>>& time_;
  DrvT& drv_;
  CtrT& ctr_;
  GenT& gen_;

  uint32_t steps_to_load_ = 0;
  uint32_t v_ = 1'500;
  uint32_t spms_ = 0;
};
}  // namespace m

#endif  // LINEAR_STEP_POSITIONER_HPP
