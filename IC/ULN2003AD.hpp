/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ULN2003AD_HPP
#define ULN2003AD_HPP

#include <IPin.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace m::ic {

/**
 * Unipolar stepper motor behind a ULN2003AD darlington array: four pins, one
 * per input (IN1..IN4), each switching one motor phase to ground.
 *
 * Half stepping is used — 8 states per electrical cycle, one or two coils at a
 * time. That is the finest resolution four pins can give, and it runs smoother
 * than full stepping. step() advances exactly one half-step and keeps the coils
 * energized; hold() re-energizes the current state (holding torque); release()
 * turns all coils off (no current, no torque — the gearbox holds the position).
 *
 * Usage:
 *   m::ic::Uln2003AD motor{pin1, pin2, pin3, pin4};  // IN1..IN4
 *   motor.step(+1);   // one half-step one way
 *   motor.step(-1);   // ... and back
 *   motor.release();
 *
 * The sequence assumes IN1..IN4 = phases A..D. If the motor turns the wrong
 * way, swap any two neighbouring inputs (or negate the direction).
 */
class Uln2003AD final {
 public:
  /// States per electrical cycle in half-step mode.
  static constexpr uint8_t Phases = 8;

  Uln2003AD(m::ifc::mcu::IPin& in1, m::ifc::mcu::IPin& in2,
            m::ifc::mcu::IPin& in3, m::ifc::mcu::IPin& in4)
      : pins_{&in1, &in2, &in3, &in4} {
    release();
  }

  /// One half-step: direction > 0 forward, direction < 0 backward.
  void step(int direction) {
    // Sign only: the phase always stays inside the sequence.
    phase_ = static_cast<uint8_t>((phase_ + Phases + (direction > 0 ? 1 : -1)) %
                                  Phases);
    drive();
  }

  /// Keeps the current phase energized (holding torque).
  void hold() { drive(); }

  /// All coils off: no current, the gearbox keeps the position.
  void release() {
    for (m::ifc::mcu::IPin* pin : pins_) {
      pin->write(false);
    }
  }

  [[nodiscard]] uint8_t phase() const { return phase_; }

 private:
  /// Half-step pattern, bit 0 = IN1 ... bit 3 = IN4. Gray code: consecutive
  /// states differ by exactly one coil, so the rotor always has a next step.
  static constexpr std::array<uint8_t, Phases> Sequence{
      0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000, 0b1001};

  void drive() {
    const uint8_t coils = Sequence[phase_];
    for (std::size_t i = 0; i < pins_.size(); ++i) {
      pins_[i]->write(((coils >> i) & 1U) != 0);
    }
  }

  std::array<m::ifc::mcu::IPin*, 4> pins_;
  uint8_t phase_ = 0;
};

}  // namespace m::ic

#endif  // ULN2003AD_HPP
