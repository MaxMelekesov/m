/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIMEOUT_HPP
#define TIMEOUT_HPP

#include <ITime.hpp>
#include <functional>

namespace m {

template <m::ifc::CTime TimeT, typename Fn>
  requires std::invocable<Fn&> && std::same_as<std::invoke_result_t<Fn&>, bool>
bool execWithTimeout(TimeT& time, Fn&& code, typename TimeT::Unit timeout) {
  auto start = time.now();
  while (!std::invoke(code)) {
    if (time.diff(start) > timeout) return false;
  }
  return true;
}

}  // namespace m

#endif  // TIMEOUT_HPP