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

template <m::ifc::CMs MsT>
class PeriodicTask {
 public:
  PeriodicTask(ifc::ITime<MsT>& time, MsT period, std::function<void()>&& cb)
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
  Timer<MsT> timer_;
  MsT period_;
  const std::function<void()> cb_;

  bool start_ = true;
};

template <m::ifc::CMs MsT>
PeriodicTask(ifc::ITime<MsT>&, MsT, std::function<void()>&&)
    -> PeriodicTask<MsT>;

}  // namespace m

#endif  // PERIODIC_TASK_H