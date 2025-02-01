/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef DATA_LINK_ASYNC_H
#define DATA_LINK_ASYNC_H

#include <IDataLink.hpp>
#include <IIO_Async.hpp>
#include <Timer.hpp>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace m {

// Packets separated by time
template <typename TimeUnit>
class DataLinkAsync : public ifc::IDataLink {
 public:
  using type = TimeUnit;

  struct Timings {
    type packet_rx_time_between_bytes{0};
  };

 public:
  DataLinkAsync(ifc::ITime<type> &time, ifc::IIO_Async &io, Timings timings)
      : io_(io), rx_between_bytes_timer_{time}, tx_timeout_timer_{time} {
    rx_between_bytes_timer_.restart(timings.packet_rx_time_between_bytes);
  }

  bool startReceive(std::span<uint8_t> rx_buf) override {
    if (!io_.readDone()) return false;
    rx_buf_ = rx_buf;
    if (!io_.abortRead()) {
      return false;
    }
    if (!io_.readAsync(rx_buf_)) {
      return false;
    }

    bytes_start_count_ = 0;

    return true;
  }

  std::optional<uint32_t> getRxPacketSize() override {
    auto bytes = io_.bytesAvailable();
    if (bytes == rx_buf_.size()) {
      if (io_.readDone())
        return rx_buf_.size();
      else
        return std::nullopt;
    } else {
      if (bytes != 0) {
        if (bytes == bytes_start_count_) {
          if (rx_between_bytes_timer_.timeOver()) {
            if (!io_.abortRead()) {
              return std::nullopt;
            } else {
              return bytes;
            }
          }
        } else {
          bytes_start_count_ = bytes;
          rx_between_bytes_timer_.reset();
        }
      }
    }

    return std::nullopt;
  }

  bool startTransmit(std::span<uint8_t> tx_buf) override {
    if (!io_.abortWrite()) {
      return false;
    }
    if (!io_.writeAsync(tx_buf)) {
      return false;
    }

    tx_timeout_timer_.restart(
        type{tx_buf.size() * 1'000'000 / io_.getBaudrate() + 3'000});

    return true;
  }

  std::optional<bool> transmitDone() override {
    if (io_.writeDone()) {
      if (!io_.abortWrite()) {
        return false;
      }
      return true;
    } else {
      if (tx_timeout_timer_.timeOver()) {
        if (!io_.abortWrite()) {
        }
        return false;
      }
    }
    return std::nullopt;
  }

  bool error() override { return io_.error(); }

  bool reset() override {
    if (!io_.abortWrite()) {
      return false;
    }
    if (!io_.abortRead()) {
      return false;
    }
    return true;
  }

 private:
  ifc::IIO_Async &io_;

  Timer<type> rx_between_bytes_timer_;
  Timer<type> tx_timeout_timer_;

  uint32_t bytes_start_count_ = 0;
  std::span<uint8_t> rx_buf_;
};

}  // namespace m

#endif  // DATA_LINK_ASYNC_H