/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ITIME_HPP
#define ITIME_HPP

#include <concepts>
#include <cstdint>

namespace m::ifc {

template <typename T>
class ITime {
 public:
  using TimeUnit = T;

  virtual ~ITime() {}

  virtual void delay(TimeUnit value) = 0;
  virtual TimeUnit getTick() = 0;
  virtual TimeUnit getDiff(TimeUnit value) = 0;
};

template <typename T, typename TimeUnit>
concept CTime = requires(T t, TimeUnit value) {
  { t.delay(value) } -> std::same_as<void>;
  { t.getTick() } -> std::same_as<TimeUnit>;
  { t.getDiff(value) } -> std::same_as<TimeUnit>;
};

static_assert(CTime<ITime<uint32_t>, uint32_t>,
              "ITime does not satisfy CTime concept");
}  // namespace m::ifc

#endif  // ITIME_HPP
