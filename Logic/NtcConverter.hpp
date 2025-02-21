/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef NTC_CONVERTER_H
#define NTC_CONVERTER_H

#include <math.h>

#include <Kelvin.hpp>
#include <Ohm.hpp>
#include <cstdint>

namespace m {

class NtcConverter {
 public:
  NtcConverter(Kelvin<float> b25_100, Ohm<float> r0, Kelvin<float> t0)
      : b25_100_(b25_100), r0_(r0), t0_(t0) {}

  Kelvin<float> getTemperature(Ohm<float> value) {
    return Kelvin<float>(
        1.0f / (1.0f / t0_.value() + std::log(value / r0_) / b25_100_.value()));
  }

 private:
  Kelvin<float> b25_100_;
  Ohm<float> r0_;
  Kelvin<float> t0_;
};

}  // namespace m

#endif  // NTC_CONVERTER_H