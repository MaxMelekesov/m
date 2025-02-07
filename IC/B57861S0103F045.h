/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef B57861S0103F045_H
#define B57861S0103F045_H

#include <cmath>
#include <cstdint>

namespace m::ic {
class B57861S0103F045 {
 public:
  // R/T No. 8016
  // B25/100 = 3988 K
  // 10K 1%

  float resToTemp(float res) {
    float l = std::log(res);
    return 1.0f / (a_ + b_ * l + c_ * l * l * l) - 273.15f;
  }

 private:
  float a_ = 0.001125308852122f;
  float b_ = 0.000234711863267f;
  float c_ = 0.000000085663516f;
};
}  // namespace m::ic

#endif  // B57861S0103F045_H