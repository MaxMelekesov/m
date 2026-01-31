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

#include <Ms.hpp>
#include <Us.hpp>
#include <concepts>
#include <cstdint>

namespace m::ifc {

template <typename UnitT>
concept CTimeUnit = CUs<UnitT> || CMs<UnitT>;

template <CTimeUnit UnitT>
class ITime {
 public:
  virtual ~ITime() {}

  virtual void delay(UnitT value) = 0;
  virtual UnitT getTick() = 0;
  virtual UnitT getDiff(UnitT value) = 0;
};

template <typename T>
concept CTime = requires(T time) {
  { time.delay(time.getTick()) } -> std::same_as<void>;
  { time.getTick() };
  { time.getDiff(time.getTick()) };
};

static_assert(CTime<ITime<Ms<uint32_t>>>, "ITime must satisfy CTime concept");

}  // namespace m::ifc

#endif  // ITIME_HPP
