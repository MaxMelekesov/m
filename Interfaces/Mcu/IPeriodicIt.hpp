/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IPERIODICIT_HPP
#define IPERIODICIT_HPP

#include <Hz.hpp>
#include <IIt.hpp>
#include <concepts>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace m::ifc::mcu {

template <typename UnitT>
class IPeriodicIt {
 public:
  using Unit = UnitT;

  virtual ~IPeriodicIt() = default;

  virtual void setCallback(std::function<void()>&& cb) = 0;

  virtual bool setInterval(Unit value) = 0;
  [[nodiscard]] virtual Unit getInterval() = 0;

  virtual bool start() = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;
};

template <typename T, typename UnitT>
concept CPeriodicItOf =
    requires(T it, std::function<void()>&& cb, UnitT value) {
      { it.setCallback(std::move(cb)) } -> std::same_as<void>;
      { it.setInterval(value) } -> std::same_as<bool>;
      { it.getInterval() } -> std::same_as<UnitT>;
      { it.start() } -> std::same_as<bool>;
      { it.running() } -> std::same_as<bool>;
      { it.stop() } -> std::same_as<bool>;
    } &&
    std::is_same_v<decltype(&T::setCallback),
                   void (T::*)(std::function<void()>&&)> &&
    std::is_same_v<decltype(&T::setInterval), bool (T::*)(UnitT)>;

template <typename T>
concept CPeriodicIt =
    requires { typename T::Unit; } && CPeriodicItOf<T, typename T::Unit>;

static_assert(CPeriodicItOf<IPeriodicIt<Hz<uint32_t>>, Hz<uint32_t>>,
              "IPeriodicIt must satisfy CPeriodicItOf concept");
static_assert(CPeriodicIt<IPeriodicIt<Hz<uint32_t>>>,
              "IPeriodicIt must satisfy CPeriodicIt concept");
static_assert(CIt<IPeriodicIt<Hz<uint32_t>>>,
              "IPeriodicIt must satisfy CIt concept");

}  // namespace m::ifc::mcu

#endif  // IPERIODICIT_HPP
