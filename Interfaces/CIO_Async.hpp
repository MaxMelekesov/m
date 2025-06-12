/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef CIO_ASYNC_H
#define CIO_ASYNC_H

#include <Bps.hpp>
#include <concepts>
#include <cstdint>
#include <span>

namespace m::c {

template <typename T>
concept CIO_Async =
    requires(T io, std::span<uint8_t> rx_buf, std::span<const uint8_t> tx_buf) {
      { io.bytesToWrite() } -> std::same_as<std::size_t>;
      { io.writeAsync(tx_buf) } -> std::same_as<bool>;
      { io.abortWrite() } -> std::same_as<bool>;
      { io.writeDone() } -> std::same_as<bool>;

      { io.bytesAvailable() } -> std::same_as<std::size_t>;
      { io.readAsync(rx_buf) } -> std::same_as<bool>;
      { io.abortRead() } -> std::same_as<bool>;
      { io.readDone() } -> std::same_as<bool>;

      requires CBps<std::remove_cvref_t<decltype(io.getBaudrate())>>;
      requires requires(
          typename std::remove_cvref_t<decltype(io.getBaudrate())> baud) {
        { io.setBaudrate(baud) } -> std::same_as<bool>;
      };

      { io.error() } -> std::same_as<bool>;
    };

}  // namespace m::c

#endif  // CIO_ASYNC_H
