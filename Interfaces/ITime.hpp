/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ITIME_H
#define ITIME_H
#include <cstdint>

namespace m::ifc {

template <typename TimeUnit>
class ITime {
 public:
  using type = TimeUnit;

  virtual ~ITime() {}

  virtual void delay(type value) = 0;
  virtual type getTick() = 0;
  virtual type getDiff(type value) = 0;
};
}  // namespace m::ifc

#endif  // ITIME_H