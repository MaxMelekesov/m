/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef DEBUG_LOGGER_HPP
#define DEBUG_LOGGER_HPP

#include <array>
#include <cstddef>
#include <optional>

namespace m {

template <std::size_t LogSize = 100>
class DebugLogger {
 public:
  static DebugLogger& getInstance() {
    static DebugLogger instance;
    return instance;
  }

  DebugLogger(const DebugLogger&) = delete;
  DebugLogger& operator=(const DebugLogger&) = delete;
  DebugLogger(DebugLogger&&) = delete;
  DebugLogger& operator=(DebugLogger&&) = delete;

  void add(const char* message) {
    logs_[last_index_] = message;
    last_index_ = (last_index_ + 1) % LogSize;
    if (count_ == LogSize) {
      first_index_ = (first_index_ + 1) % LogSize;
    } else {
      ++count_;
    }
  }

  [[nodiscard]] std::optional<const char*> getFirst() {
    if (count_ == 0) {
      return std::nullopt;
    }
    const auto index = first_index_;
    first_index_ = (first_index_ + 1) % LogSize;
    --count_;
    return logs_[index];
  }

  [[nodiscard]] std::size_t size() const { return count_; }
  [[nodiscard]] bool empty() const { return count_ == 0; }
  [[nodiscard]] bool full() const { return count_ == LogSize; }

  void clear() {
    first_index_ = 0;
    last_index_ = 0;
    count_ = 0;
  }

 private:
  DebugLogger() = default;
  std::array<const char*, LogSize> logs_{};
  std::size_t first_index_{0};
  std::size_t last_index_{0};
  std::size_t count_{0};
};

}  // namespace m

#endif  // DEBUG_LOGGER_HPP