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
concept tuple_contains = []<std::size_t... I>(std::index_sequence<I...>) {
  return (std::same_as<T, std::tuple_element_t<I, Tuple>> || ...);
}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}  // namespace m

#endif  // TUPLE_CONTAINS_HPP
