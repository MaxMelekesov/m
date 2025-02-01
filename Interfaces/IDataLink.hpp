/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IDATALINK_H
#define IDATALINK_H

#include <cstdint>
#include <optional>
#include <span>

namespace m::ifc {
class IDataLink {
 public:
  virtual ~IDataLink() {}

  virtual bool startReceive(std::span<uint8_t> rx_buf) = 0;
  virtual std::optional<uint32_t> getRxPacketSize() = 0;

  virtual bool startTransmit(std::span<uint8_t> tx_buf) = 0;
  virtual std::optional<bool> transmitDone() = 0;

  virtual bool error() = 0;
  virtual bool reset() = 0;
};
}  // namespace m::ifc

#endif  // IDATALINK_H
