/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef DATA_LINK_ASYNC_HPP
#define DATA_LINK_ASYNC_HPP

#include <Bps.hpp>
#include <IDataLink.hpp>
#include <IIO_Async.hpp>
#include <Timer.hpp>
#include <Us.hpp>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace m {

// Packets separated by time
template <ifc::CUs UsT, ifc::CBps BpsT>
class DataLinkAsync : public ifc::IDataLink {
 public:
  struct Timings {
    UsT packet_rx_time_between_bytes{0};
  };

  DataLinkAsync(ifc::ITime<UsT>& time, ifc::IIO_Async<BpsT>& io,
                Timings timings)
      : io_(io), rx_between_bytes_timer_{time}, tx_timeout_timer_{time} {
    rx_between_bytes_timer_.restart(timings.packet_rx_time_between_bytes);
  }

  bool startReceive(std::span<uint8_t> rx_buf) override {
    if (!io_.isReadDone()) return false;
    rx_buf_ = rx_buf;
    if (!io_.abortRead()) {
      return false;
    }
    if (!io_.startRead(rx_buf_)) {
      return false;
    }

    bytes_start_count_ = 0;

    return true;
  }

  bool stopReceive() override { return io_.abortRead(); }

  std::optional<std::span<uint8_t>> getPacket() override {
    auto bytes = io_.bytesReaded();
    if (bytes == rx_buf_.size()) {
      if (io_.isReadDone()) {
        return rx_buf_;
      } else {
        return std::nullopt;
      }
    } else {
      if (bytes != 0) {
        if (bytes == bytes_start_count_) {
          if (rx_between_bytes_timer_.timeOver()) {
            if (!io_.abortRead()) {
              return std::nullopt;
            } else {
              return rx_buf_.first(bytes);
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

  bool startTransmit(std::span<const uint8_t> tx_buf) override {
    if (!io_.abortWrite()) {
      return false;
    }
    if (!io_.startWrite(tx_buf)) {
      return false;
    }

    tx_timeout_timer_.restart(
        UsT{static_cast<UsT::type>(tx_buf.size() * 1'000'000 /
                                   io_.getBaudrate().value())} +
        UsT{3'000});

    return true;
  }

  bool stopTransmit() override { return io_.abortWrite(); }

  std::optional<bool> transmitDone() override {
    if (io_.isWriteDone()) {
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

 private:
  ifc::IIO_Async<BpsT>& io_;

  Timer<UsT> rx_between_bytes_timer_;
  Timer<UsT> tx_timeout_timer_;

  uint32_t bytes_start_count_ = 0;
  std::span<uint8_t> rx_buf_;
};

template <ifc::CUs UsT, ifc::CBps BpsT>
DataLinkAsync(ifc::ITime<UsT>&, ifc::IIO_Async<BpsT>&,
              typename DataLinkAsync<UsT, BpsT>::Timings)
    -> DataLinkAsync<UsT, BpsT>;

}  // namespace m

#endif  // DATA_LINK_ASYNC_HPP