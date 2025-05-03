/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CDATA_LINK_V2_H
#define CDATA_LINK_V2_H

#include <cstdint>
#include <optional>
#include <span>

namespace m::c {

struct RingSpan {
  std::span<uint8_t> first;
  std::span<uint8_t> second;
  std::optional<std::span<uint8_t>> copyTo(std::span<uint8_t> buf) const {
    if (buf.size() < first.size() + second.size()) {
      return std::nullopt;
    }
    std::copy(first.begin(), first.end(), buf.begin());
    std::copy(second.begin(), second.end(), buf.begin() + first.size());
    return std::span<uint8_t>(buf.data(), first.size() + second.size());
  }
};

/*
CRingDataLink is used to receive and parse packets from a ring buffer.
*/
template <typename T>
concept CRingDataLink =
    requires(T dl, std::span<uint8_t> rx_buf, std::span<const uint8_t> tx_buf) {
      { dl.startReceive(rx_buf) } -> std::same_as<bool>;
      { dl.getPacket() } -> std::convertible_to<std::optional<RingSpan>>;
      { dl.startTransmit(tx_buf) } -> std::same_as<bool>;
      { dl.transmitDone() } -> std::convertible_to<std::optional<bool>>;
      { dl.stopReceive() } -> std::same_as<bool>;
      { dl.stopTransmit() } -> std::same_as<bool>;
      { dl.error() } -> std::same_as<bool>;
    };

/*
CDataLink is used to receive and parse packets from a serial buffer.
*/
template <typename T>
concept CDataLink =
    requires(T dl, std::span<uint8_t> rx_buf, std::span<const uint8_t> tx_buf) {
      { dl.startReceive(rx_buf) } -> std::same_as<bool>;
      {
        dl.getPacket()
      } -> std::convertible_to<std::optional<std::span<uint8_t>>>;
      { dl.startTransmit(tx_buf) } -> std::same_as<bool>;
      { dl.transmitDone() } -> std::convertible_to<std::optional<bool>>;
      { dl.stopReceive() } -> std::same_as<bool>;
      { dl.stopTransmit() } -> std::same_as<bool>;
      { dl.error() } -> std::same_as<bool>;
    };

}  // namespace m::c

#endif  // CDATA_LINK_V2_H
