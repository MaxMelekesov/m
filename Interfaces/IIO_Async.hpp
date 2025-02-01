/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIO_ASYNC_H
#define IIO_ASYNC_H

#include <cstdint>
#include <span>

namespace m::ifc {

class IIO_Async {
 public:
  virtual ~IIO_Async() {}

  virtual uint32_t bytesToWrite() = 0;
  virtual bool writeAsync(std::span<uint8_t const> data) = 0;
  virtual bool abortWrite() = 0;
  virtual bool writeDone() = 0;

  virtual uint32_t bytesAvailable() = 0;
  virtual bool readAsync(std::span<uint8_t> data) = 0;
  virtual bool abortRead() = 0;
  virtual bool readDone() = 0;

  virtual uint32_t getBaudrate() = 0;
  virtual bool setBaudrate(uint32_t baud) { return false; }

  virtual bool error() = 0;
};
}  // namespace m::ifc

#endif  // IIO_ASYNC_H