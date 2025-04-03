/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#include <ILog.hpp>
#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string_view>

namespace m {

template <std::size_t MaxStringSize = 64, std::size_t LogSize = 100>
class SimpleLogger final : public m::ifc::ILog {
 public:
  SimpleLogger() = default;

  constexpr void add(std::string_view message) override {
    const auto length = std::min(message.length(), MaxStringSize);
    logs_[lastIndex_] = message.substr(0, length);

    lastIndex_ = (lastIndex_ + 1) % LogSize;

    if (count_ == LogSize) {
      firstIndex_ = (firstIndex_ + 1) % LogSize;
    } else {
      ++count_;
    }
  }

  [[nodiscard]] constexpr std::optional<std::string_view> getFirst() {
    if (count_ == 0) {
      return std::nullopt;
    }

    const auto index = firstIndex_;
    firstIndex_ = (firstIndex_ + 1) % LogSize;
    --count_;

    return logs_[index];
  }

  [[nodiscard]] constexpr std::size_t size() const { return count_; }

  [[nodiscard]] constexpr bool empty() const { return count_ == 0; }

  [[nodiscard]] constexpr bool full() const { return count_ == LogSize; }

  void clear() override {
    firstIndex_ = 0;
    lastIndex_ = 0;
    count_ = 0;
  }

 private:
  std::array<std::string_view, LogSize> logs_{};
  std::size_t firstIndex_{0};
  std::size_t lastIndex_{0};
  std::size_t count_{0};
};

}  // namespace m