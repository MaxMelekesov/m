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

#include <ITime.h>

#include <functional>

namespace m {
class TimerMs {
 private:
  using Ms = ifc::Ms;

 public:
  TimerMs(ifc::ITime& time) : time_(time) {}

  bool restart(Ms ms) {
    stop();
    return start(ms);
  }

  [[nodiscard]] bool start(Ms ms) {
    if (running_) return false;
    start_ = time_.getTickMs();
    wait_ = ms;
    running_ = true;
    return true;
  }

  bool reset() {
    if (!running_) return false;
    start_ = time_.getTickMs();
    return true;
  }

  void stop() { running_ = false; }

  bool running() { return running_; };

  bool timeOver() {
    if (!running_) return false;
    return time_.getDiff(start_) >= wait_;
  }

 private:
  ifc::ITime& time_;

  Ms start_{0}, wait_{0};
  bool running_ = false;
};

class TimerUs {
 private:
  using Us = ifc::Us;

 public:
  TimerUs(ifc::ITime& time) : time_(time) {}

  bool restart(Us us) {
    stop();
    return start(us);
  }

  [[nodiscard]] bool start(Us us) {
    if (running_) return false;
    start_ = time_.getTickUs();
    wait_ = us;
    running_ = true;
    return true;
  }

  bool reset() {
    if (!running_) return false;
    start_ = time_.getTickUs();
    return true;
  }

  void stop() { running_ = false; }

  bool running() { return running_; };

  bool timeOver() {
    if (!running_) return false;
    return time_.getDiff(start_) >= wait_;
  }

 private:
  ifc::ITime& time_;

  Us start_{0}, wait_{0};
  bool running_ = false;
};
}  // namespace m

#endif  // TIMER_H