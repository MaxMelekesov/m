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
#include <cstdint>

namespace m {

class SAccCurve {
 public:
  SAccCurve(Ms<uint32_t> acc_t_limit, uint32_t min_v_limit,
            uint32_t max_v_limit)
      : Acc_T_Limit_(acc_t_limit),
        Min_V_Limit_(min_v_limit),
        Max_V_Limit_(max_v_limit) {}

  bool setMinV(float value) {
    if (value < Min_V_Limit_ || value >= max_v_) return false;
    min_v_ = value;
    return true;
  }
  float getMinV() const { return min_v_; }

  bool setMaxV(float value) {
    if (value <= min_v_ || value > Max_V_Limit_) return false;
    max_v_ = value;
    return true;
  }
  float getMaxV() const { return max_v_; }

  bool setAccT(Ms<uint32_t> ms) {
    if (ms < Ms<uint32_t>{10} || ms > Acc_T_Limit_) return false;
    acc_t_ = ms;
    return true;
  }
  Ms<uint32_t> getAccT() { return acc_t_; }

  float vt(Ms<uint32_t> t) {
    float t_div_max_t =
        static_cast<float>(t.value()) / static_cast<float>(acc_t_.value());
    float t3 = t_div_max_t * (min_v_ - max_v_) * t_div_max_t * t_div_max_t;
    float t4 = t_div_max_t * t3;
    float t5 = t_div_max_t * t4;

    float temp = min_v_ - 6.0f * t5 + 15.0f * t4 - 10.0f * t3;

    return temp;
  }

  float st(Ms<uint32_t> t) {
    float t_div_max_t =
        static_cast<float>(t.value()) / static_cast<float>(acc_t_.value());
    float t3 = t_div_max_t * (min_v_ - max_v_) * t_div_max_t *
               static_cast<float>(t.value()) / 1'000.0f * t_div_max_t;
    float t4 = t_div_max_t * t3;
    float t5 = t_div_max_t * t4;

    float temp = min_v_ * static_cast<float>(t.value()) / 1'000.0f - t5 +
                 3.0f * t4 - 2.5f * t3;

    return temp;
  };

 private:
  const Ms<uint32_t> Acc_T_Limit_;
  const uint32_t Min_V_Limit_;
  const uint32_t Max_V_Limit_;

  float min_v_ = Min_V_Limit_;
  float max_v_ = Max_V_Limit_;
  Ms<uint32_t> acc_t_ = Acc_T_Limit_;
};
}  // namespace m

#endif  // S_ACC_CURVE_HPP
