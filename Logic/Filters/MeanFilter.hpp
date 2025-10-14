/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MEAN_FILTER_HPP
#define MEAN_FILTER_HPP

#include <algorithm>
#include <array>
#include <type_traits>

/*
Example usage of MeanFilter:

#include "MeanFilter.hpp"

m::MeanFilter<int, 5> filter;

filter.add(10);
filter.add(20);
filter.add(30);
filter.add(40);
filter.add(50);

int mean = filter.getValue(); // mean will be 30

filter.clear(); // reset the filter
*/

namespace m {
template <typename T, std::size_t N>
class MeanFilter {
 public:
  MeanFilter() : window_size_(N) {}
  MeanFilter(std::size_t size) : window_size_(size) {}

  T add(T value) {
    if (first_run_) {
      first_run_ = false;
      std::fill(window_.begin(), window_.end(), value);
      sum_ = value * window_size_;
      return value;
    }

    sum_ -= window_[index_];
    sum_ += value;
    window_[index_] = value;
    index_ = (index_ + 1) % window_size_;

    if constexpr (std::is_integral<T>::value) {
      return static_cast<T>((sum_ + window_size_ / 2) / window_size_);
    } else {
      return static_cast<T>(sum_ / window_size_);
    }
  }

  bool setWindowSize(std::size_t size) {
    if (size < 1 || size > N) {
      return false;
    }
    window_size_ = size;
    clear();
    return true;
  }

  std::size_t getWindowSize() const { return window_size_; }

  void clear() {
    index_ = 0;
    first_run_ = true;
    sum_ = T{};
  }

 private:
  std::array<T, N> window_;
  std::size_t index_ = 0;
  std::size_t window_size_;
  bool first_run_ = true;
  T sum_ = T{};
};
}  // namespace m

#endif  // MEAN_FILTER_HPP