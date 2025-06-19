/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IENABLE_HPP
#define IENABLE_HPP

#include <concepts>

namespace m::ifc::mcu {
class IEnable {
 public:
  virtual ~IEnable() {}

  virtual bool enable() = 0;
  virtual bool isEnabled() = 0;
  virtual bool disable() = 0;
};

template <typename T>
concept CEnable = requires(T t) {
  { t.enable() } -> std::same_as<bool>;
  { t.isEnabled() } -> std::same_as<bool>;
  { t.disable() } -> std::same_as<bool>;
};

static_assert(CEnable<IEnable>, "IEnable must satisfy CEnable concept");

}  // namespace m::ifc::mcu

#endif  // IENABLE_HPP
