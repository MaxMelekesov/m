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

#include <ITime.hpp>
#include <concepts>
#include <functional>

namespace m {

template <typename TimeIfc, typename TimeUnit>
  requires m::ifc::CTime<TimeIfc, TimeUnit>
bool execWithTimeout(TimeIfc& time, const std::function<bool()>& code,
                     TimeUnit timeout) {
  auto start = time.getTick();
  while (!code()) {
    if (time.getDiff(start) > timeout) return false;
  }
  return true;
}

template <typename TimeUnit>
class Timeout {
 public:
  using type = TimeUnit;

  Timeout(ifc::ITime<type>& time) : time_(time) {}

  // Run while code is false and time diff < timeout
  bool execWithTimeout(const std::function<bool()>& code, type timeout) {
    auto start = time_.getTick();

    while (!code()) {
      if (time_.getDiff(start) > timeout) return false;
    }

    return true;
  }

 private:
  ifc::ITime<type>& time_;
};
}  // namespace m

#endif  // TIMEOUT_H