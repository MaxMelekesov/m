/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CORO_DELAY_HPP
#define CORO_DELAY_HPP

#include <CoroScheduler.hpp>
#include <CoroUntil.hpp>
#include <ITime.hpp>

namespace m {

template <m::ifc::CTime TimeT>
inline auto coroDelay(TimeT& time,
                      decltype(std::declval<TimeT&>().getTick()) delay)
    -> m::Task<void> {
  auto start = time.getTick();
  co_await m::coroUntil([&] { return time.getDiff(start) >= delay; });
}

}  // namespace m

#endif  // CORO_DELAY_HPP