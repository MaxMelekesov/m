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
  using UnitT = T;

  virtual ~ITime() {}

  virtual void delay(UnitT value) = 0;
  virtual UnitT getTick() = 0;
  virtual UnitT getDiff(UnitT value) = 0;
};

template <typename T, typename UnitT>
concept CTime = requires(T t, UnitT value) {
  { t.delay(value) } -> std::same_as<void>;
  { t.getTick() } -> std::same_as<UnitT>;
  { t.getDiff(value) } -> std::same_as<UnitT>;
};

static_assert(CTime<ITime<uint32_t>, uint32_t>,
              "ITime does not satisfy CTime concept");
}  // namespace m::ifc

#endif  // ITIME_HPP
