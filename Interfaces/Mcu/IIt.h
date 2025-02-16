/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIT_H
#define IIT_H

#include <functional>

namespace m::ifc::mcu {
class IIt {
 public:
  IIt(const std::function<void()>& cb) : cb_(cb) {}
  virtual ~IIt() {}

  virtual bool start() = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;

 protected:
  const std::function<void()>& cb_;
};
}  // namespace m::ifc::mcu

#endif  // IIT_H