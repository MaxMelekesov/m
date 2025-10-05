/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TUPLE_CONTAINS_HPP
#define TUPLE_CONTAINS_HPP
#include <tuple>

namespace m {
template <typename T, typename Tuple>
concept tuple_contains = []<typename... Args>(std::tuple<Args...>*) {
  return (std::same_as<T, Args> || ...);
}(static_cast<Tuple*>(nullptr));
}  // namespace m

#endif  // TUPLE_CONTAINS_HPP
