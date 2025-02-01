/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIMER_H
#define TIMER_H

#include <ITime.hpp>

namespace m {

template <typename TimeUnit>
class Timer {
 public:
  using type = TimeUnit;

  Timer(ifc::ITime<type>& time) : time_(time) {}

  bool restart(type value) {
    stop();
    return start(value);
  }

  [[nodiscard]] bool start(type value) {
    if (running_) return false;
    start_ = time_.getTick();
    wait_ = value;
    running_ = true;
    return true;
  }

  bool reset() {
    if (!running_) return false;
    start_ = time_.getTick();
    return true;
  }

  void stop() { running_ = false; }

  bool running() { return running_; };

  bool timeOver() {
    if (!running_) return false;
    return time_.getDiff(start_) >= wait_;
  }

 private:
  ifc::ITime<type>& time_;

  type start_{0}, wait_{0};
  bool running_ = false;
};

}  // namespace m

#endif  // TIMER_H