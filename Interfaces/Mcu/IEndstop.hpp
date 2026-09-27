/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IENDSTOP_HPP
#define IENDSTOP_HPP

#include <IIt.hpp>
#include <concepts>

namespace m::ifc::mcu {

/**
 * Mechanical end position switch: a digital input with a pull-up and an
 * interrupt on the active edge, so both the level and the event are available.
 *
 * The interrupt side is IIt (callback / start / stop), the switch side adds the
 * level and a latch that remembers a press even when the switch is released
 * again before the application looks at it:
 *
 *   endstop.start();                   // clears the latch, enables the
 * callback if (endstop.pressed())   { ... }   // sits on the switch right now
 *   if (endstop.triggered()) { ... }   // was pressed since start()/clear()
 *
 * A homing routine runs the motor towards the switch and stops on triggered(),
 * while pressed() tells whether the mechanism simply rests on it.
 */
class IEndstop : public m::ifc::mcu::IIt {
 public:
  ~IEndstop() override = default;

  [[nodiscard]] virtual bool pressed() = 0;
  [[nodiscard]] virtual bool triggered() = 0;
  virtual void clear() = 0;
};

template <typename T>
concept CEndstop = requires(T e) {
  { e.pressed() } -> std::same_as<bool>;
  { e.triggered() } -> std::same_as<bool>;
  { e.clear() } -> std::same_as<void>;
  { e.start() } -> std::same_as<bool>;
  { e.running() } -> std::same_as<bool>;
  { e.stop() } -> std::same_as<bool>;
};

static_assert(CEndstop<IEndstop>, "IEndstop must satisfy CEndstop concept");

}  // namespace m::ifc::mcu

#endif  // IENDSTOP_HPP
