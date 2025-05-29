/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MEDIAN_FILTER_HPP
#define MEDIAN_FILTER_HPP

#include <algorithm>
#include <array>

/*
Example usage of MedianFilter:

#include "MedianFilter.hpp"

m::MedianFilter<int, 5> filter;

filter.add(10);
filter.add(20);
filter.add(30);
filter.add(40);
filter.add(50);

int median = filter.getValue(); // median will be 30

filter.clear(); // reset the filter
*/

namespace m {
template <typename T, std::size_t N>
class MedianFilter {
 public:
  void add(T t) {
    if (first_run_) {
      first_run_ = false;
      for (auto& v : window_) v = t;
    } else {
      window_[index_] = t;
      index_ = (index_ + 1) % window_.size();
    }
  }

  void clear() {
    index_ = 0;
    first_run_ = true;
  }

  [[nodiscard]] T getValue() {
    std::copy(window_.begin(), window_.end(), copy_.begin());
    std::sort(copy_.begin(), copy_.end());
    return copy_[copy_.size() / 2];
  }

 private:
  std::array<T, N> window_;
  std::array<T, N> copy_;
  std::size_t index_ = 0;
  bool first_run_ = false;
};
}  // namespace m

#endif  // MEDIAN_FILTER_HPP