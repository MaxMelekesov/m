/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISTEP_COUNTER_HPP
#define ISTEP_COUNTER_HPP

#include <concepts>
#include <cstdint>

namespace m::ifc {

class IStepCounter {
 public:
  virtual ~IStepCounter() = default;

  virtual bool start() = 0;

  virtual bool stop() = 0;

  virtual bool running() = 0;

  enum class Dir : uint8_t { Up = 0, Down };
  enum class DirInversion : uint8_t { No = 0, Yes };

  virtual bool setDirection(Dir dir) = 0;
  virtual Dir getDirection() = 0;

  virtual bool setDirectionInversion(DirInversion inv) = 0;
  virtual DirInversion getDirectionInversion() = 0;

  virtual int32_t getCount() = 0;
  virtual bool setCount(int32_t cnt) = 0;
};

template <typename T>
concept CStepCounter = requires(T counter, typename T::Dir dir,
                                typename T::DirInversion inv, int32_t cnt) {
  { counter.start() } -> std::same_as<bool>;
  { counter.stop() } -> std::same_as<bool>;
  { counter.running() } -> std::same_as<bool>;
  { counter.setDirection(dir) } -> std::same_as<bool>;
  { counter.getDirection() } -> std::same_as<typename T::Dir>;
  { counter.setDirectionInversion(inv) } -> std::same_as<bool>;
  { counter.getDirectionInversion() } -> std::same_as<typename T::DirInversion>;
  { counter.getCount() } -> std::same_as<int32_t>;
  { counter.setCount(cnt) } -> std::same_as<bool>;
};

static_assert(CStepCounter<IStepCounter>,
              "IStepCounter must satisfy CStepCounter concept");

}  // namespace m::ifc

#endif  // ISTEP_COUNTER_HPP
