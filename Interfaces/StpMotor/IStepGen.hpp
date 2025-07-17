/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISTEP_GEN_HPP
#define ISTEP_GEN_HPP

#include <cstdint>
#include <functional>

namespace m::ifc {

class IStepGen {
 public:
  virtual ~IStepGen() = default;

  struct Step {
    uint32_t freq;
    uint32_t steps;
  };

  using NextStepCallback = std::function<Step()>;

  virtual void setCallback(NextStepCallback&& next_step_cb) = 0;
  virtual bool start() = 0;
  virtual bool stop() = 0;
  virtual bool running() const = 0;
  virtual uint32_t maxPeriod() const = 0;
  virtual uint32_t maxSteps() const = 0;
};

template <typename T>
concept CStepGen = requires(T gen, typename T::NextStepCallback cb) {
  { gen.setCallback(std::move(cb)) } -> std::same_as<void>;
  { gen.start() } -> std::same_as<bool>;
  { gen.stop() } -> std::same_as<bool>;
  { gen.running() } -> std::same_as<bool>;
  { gen.maxPeriod() } -> std::same_as<uint32_t>;
  { gen.maxSteps() } -> std::same_as<uint32_t>;
};

static_assert(CStepGen<IStepGen>, "IStepGen must satisfy CStepGen concept");

}  // namespace m::ifc

#endif  // ISTEP_GEN_HPP
