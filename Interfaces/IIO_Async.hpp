/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIO_ASYNC_HPP
#define IIO_ASYNC_HPP

#include <Bps.hpp>
#include <cstdint>
#include <span>

namespace m::ifc {

template <CBps Baudrate>
class IIO_Async {
 public:
  virtual ~IIO_Async() {}

  virtual std::size_t bytesToWrite() = 0;
  virtual bool writeAsync(std::span<uint8_t const> data) = 0;
  virtual bool abortWrite() = 0;
  virtual bool writeDone() = 0;

  virtual std::size_t bytesAvailable() = 0;
  virtual bool readAsync(std::span<uint8_t> data) = 0;
  virtual bool abortRead() = 0;
  virtual bool readDone() = 0;

  virtual Baudrate getBaudrate() = 0;
  virtual bool setBaudrate(Baudrate baud) = 0;

  virtual bool error() = 0;
};

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

      { io.getBaudrate() };
      { io.setBaudrate(io.getBaudrate()) } -> std::same_as<bool>;

      { io.error() } -> std::same_as<bool>;
    };

static_assert(CIO_Async<IIO_Async<Bps<uint32_t>>>,
              "IIO_Async must satisfy CIO_Async concept");

}  // namespace m::ifc

#endif  // IIO_ASYNC_HPP