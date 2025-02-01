/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIO_ASYNC_LOG_H
#define IIO_ASYNC_LOG_H

#include <IIO_Async.hpp>
#include <ILog.hpp>
#include <array>
#include <string_view>

namespace m {

template <std::size_t Line_Length = 63, std::size_t Lines = 100>
class IIO_AsyncLog : public m::ifc::ILog {
 public:
  IIO_AsyncLog(m::ifc::IIO_Async& io) : io_(io) {}

  void add(std::string_view text) override {
    if (count_ < Lines) {
      if (text.size() > Line_Length) {
        text = text.substr(0, Line_Length);
      }
      std::copy(text.begin(), text.end(), buffer_[write_index_].begin());
      buffer_[write_index_][text.size()] = '\0';
      write_index_ = (write_index_ + 1) % Lines;
      ++count_;
    }
  }

  void handle() {
    if (count_ > 0 && io_.writeDone()) {
      auto& line = buffer_[read_index_];
      if (io_.writeAsync(std::span<uint8_t const>(
              reinterpret_cast<const uint8_t*>(line.data()),
              strlen(line.data(), line.size()))) == true) {
        read_index_ = (read_index_ + 1) % Lines;
        --count_;
      } else {
        io_.abortWrite();
      }
    }
  }

 private:
  m::ifc::IIO_Async& io_;

  std::array<std::array<char, Line_Length + 1>, Lines> buffer_;
  std::size_t write_index_ = 0;
  std::size_t read_index_ = 0;
  std::size_t count_ = 0;

  std::size_t strlen(const char* str, std::size_t max_len) {
    std::size_t len = 0;
    while (len < max_len && str[len] != '\0') {
      ++len;
    }
    return len;
  }
};

}  // namespace m

#endif  // IIO_ASYNC_LOG_H