/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IDAC_HPP
#define IDAC_HPP

#include <concepts>
#include <cstdint>

namespace m::ifc {

class IDac {
 public:
  virtual ~IDac() {}

  virtual bool start() = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;

  virtual bool setValue(uint32_t value) = 0;
};

template <typename T>
concept CDac = requires(T dac, uint32_t value) {
  { dac.start() } -> std::same_as<bool>;
  { dac.running() } -> std::same_as<bool>;
  { dac.stop() } -> std::same_as<bool>;
  { dac.setValue(value) } -> std::same_as<bool>;
};

static_assert(CDac<IDac>, "IDac must satisfy CDac concept");

}  // namespace m::ifc

#endif  // IDAC_HPP
