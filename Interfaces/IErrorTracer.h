/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IERRORTRACER_H
#define IERRORTRACER_H

#include <span>

namespace m::ifc {

template <typename T>
class IErrorTracer {
 public:
  using type = T;

  virtual ~IErrorTracer() {}

  virtual bool add(type value) = 0;
  virtual void clear() = 0;
  virtual std::span<type> getTrace() = 0;
};

}  // namespace m::ifc

#endif  // IERRORTRACER_H
