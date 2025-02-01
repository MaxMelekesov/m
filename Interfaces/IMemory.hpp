/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IMEMORY_H
#define IMEMORY_H

#include <cstdint>
#include <span>

namespace m::ifc {
class IMemory {
 public:
  virtual ~IMemory() {}

  virtual std::size_t size() = 0;
  virtual bool write(std::size_t addr, std::span<uint8_t const> data) = 0;
  virtual bool read(std::size_t addr, std::span<uint8_t> data) = 0;
};
}  // namespace m::ifc

#endif  // IMEMORY_H