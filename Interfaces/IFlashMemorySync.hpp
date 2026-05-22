/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IFLASHMEMORYSYNC_HPP
#define IFLASHMEMORYSYNC_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ifc {

class IFlashMemorySync {
 public:
  virtual ~IFlashMemorySync() = default;

  virtual std::size_t size() = 0;
  virtual std::size_t eraseBlockSize() = 0;
  virtual std::size_t writeBlockSize() = 0;

  virtual bool eraseBlock(std::size_t addr) = 0;
  virtual bool writeBlock(std::size_t addr, std::span<const uint8_t> data) = 0;
  virtual bool read(std::size_t addr, std::span<uint8_t> data) = 0;
};

template <typename T>
concept CFlashMemorySync =
    requires(T mem, std::size_t addr, std::span<const uint8_t> wdata,
             std::span<uint8_t> rdata) {
      { mem.size() } -> std::same_as<std::size_t>;
      { mem.eraseBlockSize() } -> std::same_as<std::size_t>;
      { mem.writeBlockSize() } -> std::same_as<std::size_t>;
      { mem.eraseBlock(addr) } -> std::same_as<bool>;
      { mem.writeBlock(addr, wdata) } -> std::same_as<bool>;
      { mem.read(addr, rdata) } -> std::same_as<bool>;
    };

static_assert(CFlashMemorySync<IFlashMemorySync>,
              "IFlashMemorySync must satisfy CFlashMemorySync concept");

}  // namespace m::ifc

#endif  // IFLASHMEMORYSYNC_HPP
