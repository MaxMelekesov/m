/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef S_ACC_CURVE_HPP
#define S_ACC_CURVE_HPP
#include <Ms.hpp>
#include <cmath>
#include <cstdint>

namespace m {

class SAccCurve {
 public:
  SAccCurve(Ms<uint32_t> max_acc_t) : Max_Acc_T_(max_acc_t) {}

  bool setMinV(float value) {
    if (value < 1.0f || value >= max_v_) return false;
    min_v_ = value;
    return true;
  }
  float getMinV() const { return min_v_; }

  bool setMaxV(float value) {
    if (value <= min_v_ || value > 500'000.0f) return false;
    max_v_ = value;
    return true;
  }
  float getMaxV() const { return max_v_; }

  bool setAccT(Ms<uint32_t> ms) {
    if (ms < Ms<uint32_t>{10} || ms > Max_Acc_T_) return false;

    acc_t_ = ms;

    return true;
  }
  Ms<uint32_t> getAccT() { return acc_t_; }

  float vt(Ms<uint32_t> t) {
    if (t > acc_t_) return 0.0f;
    float t_div_max_t =
        static_cast<float>(t.value()) / static_cast<float>(acc_t_.value());
    float t3 = t_div_max_t * (min_v_ - max_v_) * t_div_max_t * t_div_max_t;
    float t4 = t_div_max_t * t3;
    float t5 = t_div_max_t * t4;

    float temp = min_v_ - 6.0f * t5 + 15.0f * t4 - 10.0f * t3;

    return temp;
  }

  float st(Ms<uint32_t> t) {
    // TODO: add cache
    if (t > acc_t_) return 0.0f;

    float t_div_max_t =
        static_cast<float>(t.value()) / static_cast<float>(acc_t_.value());
    float t3 = t_div_max_t * (min_v_ - max_v_) * t_div_max_t *
               static_cast<float>(t.value()) / 1'000.0f * t_div_max_t;
    float t4 = t_div_max_t * t3;
    float t5 = t_div_max_t * t4;

    float temp = min_v_ * static_cast<float>(t.value()) / 1'000.0f - t5 +
                 3.0f * t4 - 5.0f * t3;

    return temp;
  };

 private:
  const Ms<uint32_t> Max_Acc_T_;

  float min_v_ = 1'000.0f;
  float max_v_ = 4'000.0f;
  Ms<uint32_t> acc_t_{500};
};
}  // namespace m

#endif  // S_ACC_CURVE_HPP
