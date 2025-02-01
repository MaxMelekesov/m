/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIO_SYNC_H
#define IIO_SYNC_H

#include <ITime.h>

#include <span>

namespace m::ifc {

class IIO_Sync {
 public:
  virtual ~IIO_Sync() {}

  virtual bool write(std::span<uint8_t const> data, Ms timeout) = 0;
  virtual bool read(std::span<uint8_t> data, Ms timeout) = 0;

  virtual uint32_t getBaudrate() = 0;
  virtual bool setBaudrate(uint32_t baud) { return false; }
};
}  // namespace m::ifc

#endif  // IIO_SYNC_H