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
#include <DebugLogger.hpp>
#include <Fsm_v4.hpp>
#include <IStepCounter.hpp>
#include <IStepDriverCtrl.hpp>
#include <IStepGen.hpp>
#include <SAccCurve.hpp>
#include <StpPositionerSettings.hpp>
#include <cmath>
#include <cstdint>
#include <optional>

#include "Ms.hpp"

namespace m {

namespace {
struct Idle : public m::State {};
struct Check : public m::State {};
struct Acc : public m::State {};
struct Linear : public m::State {};
struct Deacc : public m::State {};

struct StepsAdded : public m::Event {};
struct Start : public m::Event {};
struct Calc : public m::Event {};
struct Done : public m::Event {};

class Mover
    : public m::Fsm_v4<
          Mover, Idle, m::Transition<Idle, StepsAdded, Check>,

          m::Transition<Check, Done, Idle>, m::Transition<Check, Start, Acc>,

          m::Transition<Acc, StepsAdded, Acc>, m::Transition<Acc, Done, Linear>,
          m::Transition<Acc, Calc, Acc>,

          m::Transition<Linear, StepsAdded, Linear>,
          m::Transition<Linear, Done, Deacc>,
          m::Transition<Linear, Calc, Linear>,

          m::Transition<Deacc, StepsAdded, Deacc>,
          m::Transition<Deacc, Done, Check>, m::Transition<Deacc, Calc, Deacc>

          > {
 public:
  Mover(SAccCurve& ac) : ac_(ac) {}

  void handle() { checkEvents(); }

  int32_t addSteps(int32_t value) {
    if (value >= 0) {
      delta_steps_ = value;
      return value;
    } else {
      uint32_t deacc_steps = std::roundf(ac_.st(v_index_));
      uint32_t temp = target_steps_ - deacc_steps;
      delta_steps_ = -std::min(temp, static_cast<uint32_t>(std::abs(value)));

      return delta_steps_;
    }
  }

  bool done() {
    return isInState<Idle>() && target_steps_ == 0 && delta_steps_ == 0;
  }

  struct Part {
    uint32_t v;
    uint32_t steps;
  };

  std::optional<Part> nextPart() {
    auto temp = part_;
    part_ = std::nullopt;
    return temp;
  }

 private:
  SAccCurve& ac_;

  uint32_t target_steps_ = 0;
  int32_t delta_steps_ = 0;

  Ms<uint32_t> v_index_{0};
  Ms<uint32_t> next_v_index_{0};

  Ms<uint32_t> linear_dt_{0};
  uint32_t steps_remainder_ = 0;

  std::optional<Part> part_;

  // Idle
  bool checkEvent(Idle, StepsAdded) { return delta_steps_ != 0; }
  void handleEvent(Idle, StepsAdded) {
    target_steps_ = 0;
    target_steps_ += delta_steps_;
    delta_steps_ = 0;

    v_index_ = Ms<uint32_t>{0};
    next_v_index_ = Ms<uint32_t>{0};
  }

  // Check
  bool checkEvent(Check, Done) { return target_steps_ == 0; }
  void handleEvent(Check, Done) {}

  bool checkEvent(Check, Start) { return true; }
  void handleEvent(Check, Start) {}

  // Acc
  bool checkEvent(Acc, StepsAdded) { return delta_steps_ != 0; }
  void handleEvent(Acc, StepsAdded) {
    target_steps_ += delta_steps_;
    delta_steps_ = 0;
  }

  bool checkEvent(Acc, Done) {
    if (v_index_ == ac_.getAccT()) return true;
    if (next_v_index_ == ac_.getAccT()) return true;
    ++next_v_index_;
    uint32_t deacc_steps = std::roundf(ac_.st(next_v_index_));
    return deacc_steps > target_steps_;
  }
  void handleEvent(Acc, Done) {
    linear_dt_ = Ms<uint32_t>{0};
    next_v_index_ = v_index_;
  }

  bool checkEvent(Acc, Calc) {
    uint32_t prev = std::roundf(ac_.st(v_index_));
    uint32_t next = std::roundf(ac_.st(next_v_index_));
    uint32_t steps = next - prev;

    if (steps > 0) {
      v_index_ = next_v_index_;
      uint32_t v = std::roundf(ac_.vt(v_index_));
      part_ = Part{.v = v, .steps = steps};
      return true;
    }

    return false;
  }
  void handleEvent(Acc, Calc) { target_steps_ -= part_.value().steps; }

  // Linear
  bool checkEvent(Linear, StepsAdded) { return delta_steps_ != 0; }
  void handleEvent(Linear, StepsAdded) {
    target_steps_ += delta_steps_;
    delta_steps_ = 0;
  }

  bool checkEvent(Linear, Done) {
    if (v_index_ != ac_.getAccT()) return true;

    uint32_t deacc_steps = std::roundf(ac_.st(ac_.getAccT()));
    ++linear_dt_;
    uint32_t steps = std::floorf(
        static_cast<float>(ac_.getMaxV() * linear_dt_.value()) / 1'000.0f);
    return target_steps_ < deacc_steps + steps;
  }
  void handleEvent(Linear, Done) {
    uint32_t deacc_steps = std::roundf(ac_.st(ac_.getAccT()));
    if (target_steps_ > deacc_steps) {
      steps_remainder_ = target_steps_ - deacc_steps;
    }
  }

  bool checkEvent(Linear, Calc) {
    uint32_t steps = std::floorf(
        static_cast<float>(ac_.getMaxV() * linear_dt_.value()) / 1'000.0f);

    if (steps > 0) {
      linear_dt_ = Ms<uint32_t>{0};
      uint32_t v = static_cast<uint32_t>(ac_.getMaxV());
      part_ = Part{.v = v, .steps = steps};
      return true;
    }

    return false;
  }
  void handleEvent(Linear, Calc) { target_steps_ -= part_.value().steps; }

  // Deacc
  bool checkEvent(Deacc, StepsAdded) { return delta_steps_ != 0; }
  void handleEvent(Deacc, StepsAdded) {
    target_steps_ += delta_steps_;
    delta_steps_ = 0;
  }

  bool checkEvent(Deacc, Done) {
    if (target_steps_ == 0) return true;
    if (next_v_index_ > Ms<uint32_t>{0}) {
      --next_v_index_;
    } else {
      steps_remainder_ = target_steps_;
    }
    return false;
  }
  void handleEvent(Deacc, Done) {}

  bool checkEvent(Deacc, Calc) {
    uint32_t prev = std::roundf(ac_.st(v_index_));
    uint32_t next = std::roundf(ac_.st(next_v_index_));
    uint32_t steps = prev - next + steps_remainder_;

    if (steps_remainder_) {
      steps_remainder_ = 0;
    }

    if (steps > 0) {
      uint32_t v = std::roundf(ac_.vt(next_v_index_));
      v_index_ = next_v_index_;

      part_ = Part{.v = v, .steps = steps};
      return true;
    }

    return false;
  }
  void handleEvent(Deacc, Calc) { target_steps_ -= part_.value().steps; }

  void onEvent(StepsAdded) {
    m::DebugLogger<>::getInstance().add("StepsAdded event");
  }
  void onEvent(Start) { m::DebugLogger<>::getInstance().add("Start event"); }
  void onEvent(Calc) { m::DebugLogger<>::getInstance().add("Calc event"); }
  void onEvent(Done) { m::DebugLogger<>::getInstance().add("Done event"); }

  void onStateTransition(Acc) {
    m::DebugLogger<>::getInstance().add("State: Acc");
  }
  void onStateTransition(Linear) {
    m::DebugLogger<>::getInstance().add("State: Linear");
  }
  void onStateTransition(Deacc) {
    m::DebugLogger<>::getInstance().add("State: Deacc");
  }
  void onStateTransition(Check) {
    m::DebugLogger<>::getInstance().add("State: Check");
  }
  void onStateTransition(Idle) {
    m::DebugLogger<>::getInstance().add("State: Idle");
  }

  friend class m::Fsm_v4<
      Mover, Idle, m::Transition<Idle, StepsAdded, Check>,

      m::Transition<Check, Done, Idle>, m::Transition<Check, Start, Acc>,

      m::Transition<Acc, StepsAdded, Acc>, m::Transition<Acc, Done, Linear>,
      m::Transition<Acc, Calc, Acc>,

      m::Transition<Linear, StepsAdded, Linear>,
      m::Transition<Linear, Done, Deacc>, m::Transition<Linear, Calc, Linear>,

      m::Transition<Deacc, StepsAdded, Deacc>,
      m::Transition<Deacc, Done, Check>, m::Transition<Deacc, Calc, Deacc>>;
};

}  // namespace

// TODO: template & concepts
class StepPositioner {
 public:
  StepPositioner(m::ifc::IStepDriverCtrl<mA<uint32_t>>& drv,
                 m::ifc::IStepCounter& ctr, m::ifc::IStepGen& gen)
      : drv_(drv), ctr_(ctr), gen_(gen) {}

  int32_t getTargetPos() const { return 0; }

  bool setTargetPos(int32_t pos) {}

  bool calcTrajectory() {}

 private:
  m::ifc::IStepDriverCtrl<mA<uint32_t>>& drv_;
  m::ifc::IStepCounter& ctr_;
  m::ifc::IStepGen& gen_;

  bool dir_ = true;  // true - forward

  Ms<uint32_t> v_index_{0};
  SAccCurve ac_{Ms<uint32_t>{5'000}};

  StpPositionerSettings st_;
};
}  // namespace m

#endif  // STEP_POSITIONER_HPP
