/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PT_DELAY_HPP
#define PT_DELAY_HPP

#include <ITime.hpp>
#include <ProtoThread.hpp>

namespace m {

template <m::ifc::CTime TimeT>
class PtDelay : public Proto<PtDelay<TimeT>> {
  using Tick = decltype(std::declval<TimeT&>().now());

  TimeT& time_;
  Tick duration_{};
  Tick start_{};

 public:
  explicit PtDelay(TimeT& time) : time_(time) {}

  PtDelay& operator()(Tick duration) {
    duration_ = duration;
    return *this;
  }

  PtStatus run() {
    PT_BEGIN();
    start_ = time_.now();
    PT_WAIT_UNTIL(time_.diff(start_) >= duration_);
    PT_END();
  }
};

}  // namespace m

#endif  // PT_DELAY_HPP
