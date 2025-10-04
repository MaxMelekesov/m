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

template <typename T>
concept CMemory =
    requires(T mem, std::size_t addr, std::span<uint8_t const> wdata,
             std::span<uint8_t> rdata) {
      { mem.size() } -> std::same_as<std::size_t>;
      { mem.write(addr, wdata) } -> std::same_as<bool>;
      { mem.read(addr, rdata) } -> std::same_as<bool>;
    };

static_assert(CMemory<IMemory>, "IMemory must satisfy CMemory concept");
}  // namespace m::ifc

#endif  // IMEMORY_H