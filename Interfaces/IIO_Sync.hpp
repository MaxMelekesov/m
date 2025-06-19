/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIO_SYNC_HPP
#define IIO_SYNC_HPP

#include <BPs.hpp>
#include <cstdint>
#include <span>

namespace m::ifc {

template <CBps Baudrate>
class IIO_Sync {
 public:
  virtual ~IIO_Sync() {}

  virtual bool write(std::span<uint8_t const> data) = 0;
  virtual bool read(std::span<volatile uint8_t> data) = 0;

  virtual Baudrate getBaudrate() = 0;
  virtual bool setBaudrate(Baudrate baud) = 0;

  virtual bool error() = 0;
};

template <typename T>
concept CIO_Sync = requires(T io, std::span<volatile uint8_t> rx_buf,
                            std::span<const uint8_t> tx_buf) {
  { io.write(tx_buf) } -> std::same_as<bool>;
  { io.read(rx_buf) } -> std::same_as<bool>;

  requires CBps<std::remove_cvref_t<decltype(io.getBaudrate())>>;
  requires requires(
      typename std::remove_cvref_t<decltype(io.getBaudrate())> baud) {
    { io.setBaudrate(baud) } -> std::same_as<bool>;
  };

  { io.error() } -> std::same_as<bool>;
};

static_assert(CIO_Sync<IIO_Sync<Bps<uint32_t>>>,
              "IIO_Sync must satisfy CIO_Sync concept");

}  // namespace m::ifc

#endif  // IIO_SYNC_HPP