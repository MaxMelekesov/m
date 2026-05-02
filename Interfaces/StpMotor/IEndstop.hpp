
/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IENDSTOP_HPP
#define IENDSTOP_HPP

#include <concepts>
#include <functional>

namespace m::ifc {

class IEndstop {
 public:
  virtual ~IEndstop() = default;

  struct State {
    bool left;
    bool right;
  };
  virtual State getSwitchesState() = 0;

  virtual bool setCallbacks(std::function<void()>&& left,
                            std::function<void()>&& right) = 0;

  virtual void setSwapSwitches(bool value) = 0;
  virtual bool getSwapSwitches() = 0;

  virtual void setIgnoreSwL(bool value) = 0;
  virtual bool getIgnoreSwL() = 0;
  virtual void setActiveLevelSwL(bool value) = 0;
  virtual bool getActiveLevelSwL() = 0;

  virtual void setIgnoreSwR(bool value) = 0;
  virtual bool getIgnoreSwR() = 0;
  virtual void setActiveLevelSwR(bool value) = 0;
  virtual bool getActiveLevelSwR() = 0;
};

template <typename T>
concept CEndstop = requires(T ctrl, bool b, std::function<void()> left,
                            std::function<void()> right) {
  { ctrl.getSwitchesState() } -> std::same_as<typename T::State>;
  {
    ctrl.setCallbacks(std::move(left), std::move(right))
  } -> std::same_as<bool>;
  { ctrl.setSwapSwitches(b) } -> std::same_as<void>;
  { ctrl.getSwapSwitches() } -> std::same_as<bool>;
  { ctrl.setIgnoreSwL(b) } -> std::same_as<void>;
  { ctrl.getIgnoreSwL() } -> std::same_as<bool>;
  { ctrl.setActiveLevelSwL(b) } -> std::same_as<void>;
  { ctrl.getActiveLevelSwL() } -> std::same_as<bool>;
  { ctrl.setIgnoreSwR(b) } -> std::same_as<void>;
  { ctrl.getIgnoreSwR() } -> std::same_as<bool>;
  { ctrl.setActiveLevelSwR(b) } -> std::same_as<void>;
  { ctrl.getActiveLevelSwR() } -> std::same_as<bool>;
};

static_assert(CEndstop<IEndstop>, "IEndstop must satisfy CEndstop concept");

}  // namespace m::ifc

#endif  // IENDSTOP_HPP