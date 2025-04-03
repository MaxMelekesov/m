/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#include <IIO_Async.hpp>
#include <ILog.hpp>
#include <array>
#include <cstring>
#include <format>
#include <iostream>
#include <ranges>
#include <span>
#include <string_view>

namespace m {

template <std::size_t Line_Length = 63, std::size_t Lines = 100>
class IIO_AsyncLog : public m::ifc::ILog {
 public:
  explicit IIO_AsyncLog(m::ifc::IIO_Async& io) : io_(io) {}

  void add(std::string_view text) override {
    if (text.empty()) {
      return;
    }

    if ((write_index_ + 1) % Lines == read_index_) {
      return;
    }

    auto truncated_text = text | std::views::take(Line_Length);
    auto& buffer_line = buffer_[write_index_];
    std::ranges::copy(truncated_text, buffer_line.begin());
    buffer_line[truncated_text.size()] = '\0';
    write_index_ = (write_index_ + 1) % Lines;
  }

  void handle() {
    if (read_index_ == write_index_) {
      return;
    }

    auto& line = buffer_[read_index_];
    auto length = std::strlen(line.data());

    if (!io_.writeDone()) {
      return;
    }

    if (io_.writeAsync(std::span<const uint8_t>(
            reinterpret_cast<const uint8_t*>(line.data()), length))) {
      read_index_ = (read_index_ + 1) % Lines;
    } else {
      io_.abortWrite();
    }
  }

  void clear() override {
    write_index_ = 0;
    read_index_ = 0;
    io_.abortWrite();
  }

 private:
  m::ifc::IIO_Async& io_;

  std::array<std::array<char, Line_Length + 1>, Lines> buffer_;
  std::size_t write_index_ = 0;
  std::size_t read_index_ = 0;
};

}  // namespace m