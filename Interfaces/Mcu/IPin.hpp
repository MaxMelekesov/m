/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IPIN_HPP
#define IPIN_HPP

#include <concepts>

namespace m::ifc::mcu {
class IPin {
 public:
  virtual ~IPin() {};

  virtual void write(bool state) = 0;
  virtual bool read() = 0;
  virtual void toggle() = 0;
};

template <typename T>
concept CPin = requires(T t, bool state) {
  { t.write(state) } -> std::same_as<void>;
  { t.read() } -> std::same_as<bool>;
  { t.toggle() } -> std::same_as<void>;
};

static_assert(CPin<IPin>, "IPin must satisfy CPin concept");

}  // namespace m::ifc::mcu

#endif  // IPIN_HPP