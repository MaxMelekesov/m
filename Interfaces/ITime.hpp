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

namespace m::ifc {

template <typename UnitT>
class ITime {
 public:
  using Unit = UnitT;
  virtual ~ITime() = default;

  virtual void delay(Unit value) = 0;
  virtual Unit now() = 0;
  virtual Unit diff(Unit value) = 0;
};

template <typename TimeT, typename UnitT>
concept CTimeOf = requires(TimeT& t, UnitT v) {
  { t.delay(v) } -> std::same_as<void>;
  { t.now() } -> std::same_as<UnitT>;
  { t.diff(v) } -> std::same_as<UnitT>;
};

template <typename TimeT>
concept CTime =
    requires { typename TimeT::Unit; } && CTimeOf<TimeT, typename TimeT::Unit>;

static_assert(CTime<ITime<int>>, "ITime must satisfy CTime concept");
static_assert(CTimeOf<ITime<int>, int>, "ITime must satisfy CTimeOf concept");

}  // namespace m::ifc

#endif  // ITIME_HPP
