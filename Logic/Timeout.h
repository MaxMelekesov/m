/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <ITime.h>

#include <functional>

namespace m {
class Timeout {
 private:
  using Ms = m::ifc::Ms;
  using Us = m::ifc::Us;
  ifc::ITime& time_;

 public:
  Timeout(ifc::ITime& time) : time_(time) {}

  // Run while code is false and time diff < timeout
  bool execWithTimeout(std::function<bool()> code, Ms timeout) {
    auto start = time_.getTickMs();

    while (!code()) {
      //(uint32_t u1 - uint32_t u2) gives correct result if u1 < u2
      if (time_.getDiff(start) > timeout) return false;
    }

    return true;
  }

  bool execWithTimeout(std::function<bool()> code, Us timeout) {
    auto start = time_.getTickUs();

    while (!code()) {
      if (time_.getDiff(start) > timeout) return false;
    }

    return true;
  }
};
}  // namespace m

#endif  // TIMEOUT_H