/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PID_H
#define PID_H

#include <algorithm>
#include <cstdint>

namespace m {

class Pid {
 public:
  struct Settings {
    float kp = 1.0f, ki = 0.0f, kd = 0.0f;
    float kt = 1.0f;
    float dt = 1.0f;
    float min_out = -1.0f;
    float max_out = 1.0f;
  };

  Pid(Settings settings) : st_(settings) {}

  float update(float setpoint, float measurement) {
    float error = setpoint - measurement;

    float proportional = st_.kp * error;

    integral_ += st_.ki * error * st_.dt;

    float derivative = st_.kd * (error - prevError_) / st_.dt;

    float output = proportional + integral_ + derivative;

    float constrainedOutput = std::clamp(output, st_.min_out, st_.max_out);

    float antiWindupCorrection = st_.kt * (constrainedOutput - output);
    integral_ += antiWindupCorrection;

    step_ = {proportional, integral_, derivative};

    prevPrevError_ = prevError_;
    prevError_ = error;

    return constrainedOutput;
  }

  struct RegulationStep {
    float p, i, d;
  };

  RegulationStep getRegulationStep() { return step_; }

  void reset() {
    prevError_ = 0.0f;
    prevPrevError_ = 0.0f;
    integral_ = 0.0f;
  }

 private:
  Settings st_;

  float prevError_ = 0;
  float prevPrevError_ = 0;
  float integral_ = 0;

  RegulationStep step_{0.0f, 0.0f, 0.0f};
};
}  // namespace m

#endif  // PID_H