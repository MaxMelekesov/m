/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef BUTTERWORTH_LPF_HPP
#define BUTTERWORTH_LPF_HPP

#include <cmath>
#include <concepts>
#include <numbers>

namespace m {
template <std::floating_point T, T CutoffHz, T SampleRateHz>
class ButterworthLPF {
  static_assert(CutoffHz > T{} && CutoffHz < SampleRateHz / T{2});

  struct Coeffs {
    T b0, b1, b2, a1, a2;
  };

  static constexpr Coeffs compute_coeffs() {
    using std::numbers::pi_v;
    constexpr T w = pi_v<T> * CutoffHz / SampleRateHz;
    constexpr T t = std::tan(w);
    constexpr T s = std::sqrt(T{2});
    constexpr T t2 = t * t;
    constexpr T n = T{1} + s * t + t2;
    return Coeffs{.b0 = t2 / n,
                  .b1 = T{2} * t2 / n,
                  .b2 = t2 / n,
                  .a1 = T{2} * (t2 - T{1}) / n,
                  .a2 = (T{1} - s * t + t2) / n};
  }

  static constexpr Coeffs coeffs = compute_coeffs();

 public:
  constexpr T add(T x) {
    const T y = coeffs.b0 * x + z1;
    z1 = coeffs.b1 * x - coeffs.a1 * y + z2;
    z2 = coeffs.b2 * x - coeffs.a2 * y;
    return y;
  }

  constexpr void reset() { z1 = z2 = T{}; }

 private:
  T z1{}, z2{};
};

}  // namespace m

#endif  // BUTTERWORTH_LPF_HPP