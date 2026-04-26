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

#include <concepts>
#include <cstdint>
#include <span>

namespace m::ifc {

template <typename UnitT>
class IIO_Async {
 public:
  using Unit = UnitT;

  virtual ~IIO_Async() {}

  virtual std::size_t bytesWritten() = 0;
  virtual bool startWrite(std::span<uint8_t const> data) = 0;
  virtual bool abortWrite() = 0;
  virtual bool isWriteDone() = 0;

  virtual std::size_t bytesReaded() = 0;
  virtual bool startRead(std::span<uint8_t> data) = 0;
  virtual bool abortRead() = 0;
  virtual bool isReadDone() = 0;

  virtual UnitT getBaudrate() = 0;
  virtual bool setBaudrate(UnitT baud) = 0;

  virtual bool error() = 0;
};

template <typename T, typename BaudT>
concept CIO_AsyncOf = requires(T& io, std::span<uint8_t> rx_buf,
                               std::span<const uint8_t> tx_buf, BaudT baud) {
  { io.bytesWritten() } -> std::same_as<std::size_t>;
  { io.startWrite(tx_buf) } -> std::same_as<bool>;
  { io.abortWrite() } -> std::same_as<bool>;
  { io.isWriteDone() } -> std::same_as<bool>;

  { io.bytesReaded() } -> std::same_as<std::size_t>;
  { io.startRead(rx_buf) } -> std::same_as<bool>;
  { io.abortRead() } -> std::same_as<bool>;
  { io.isReadDone() } -> std::same_as<bool>;

  { io.getBaudrate() } -> std::same_as<BaudT>;
  { io.setBaudrate(baud) } -> std::same_as<bool>;

  { io.error() } -> std::same_as<bool>;
};

template <typename T>
concept CIO_Async =
    requires { typename T::Unit; } && CIO_AsyncOf<T, typename T::Unit>;

static_assert(CIO_Async<IIO_Async<int>>,
              "IIO_Async must satisfy CIO_Async concept");
static_assert(CIO_AsyncOf<IIO_Async<int>, int>,
              "IIO_Async<int> must satisfy CIO_AsyncOf concept");

}  // namespace m::ifc

#endif  // IIO_ASYNC_HPP