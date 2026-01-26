/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PERIODIC_TASK_H
#define PERIODIC_TASK_H

#include <Ms.hpp>
#include <Timer.hpp>
#include <functional>

namespace m {

template <m::ifc::CMs TimeUnit>
class PeriodicTask {
 public:
  using type = TimeUnit;

  PeriodicTask(ifc::ITime<type>& time, type period, std::function<void()>&& cb)
      : timer_(time), period_(period), cb_(std::move(cb)) {}

  bool running() const { return timer_.running(); }

  void resume() { start_ = true; }

  void pause() {
    start_ = false;
    timer_.stop();
  }

  void handle() {
    if (!start_) return;

    if (timer_.timeOver()) {
      cb_();
      timer_.restart(period_);
    } else {
      if (!timer_.running()) {
        timer_.restart(period_);
      }
    }
  }

 private:
  Timer<type> timer_;
  type period_;
  const std::function<void()> cb_;

  bool start_ = true;
};

}  // namespace m

#endif  // PERIODIC_TASK_H