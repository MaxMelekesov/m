/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IFLASHMEMORY_HPP
#define IFLASHMEMORY_HPP

#include <CoroScheduler.hpp>
#include <cstdint>
#include <span>

namespace m::ifc {
class IFlashMemory {
 public:
  virtual ~IFlashMemory() {}

  virtual std::size_t size() = 0;
  virtual std::size_t eraseBlockSize() = 0;
  virtual std::size_t writeBlockSize() = 0;

  virtual m::Task<bool> eraseBlock(std::size_t addr) = 0;
  virtual m::Task<bool> writeBlock(std::size_t addr,
                                   std::span<uint8_t const> data) = 0;
  virtual m::Task<bool> read(std::size_t addr, std::span<uint8_t> data) = 0;
};

template <typename T>
concept CFlashMemory =
    requires(T mem, std::size_t addr,
             std::span<uint8_t const> wdata, std::span<uint8_t> rdata) {
      { mem.size() } -> std::same_as<std::size_t>;
      { mem.eraseBlockSize() } -> std::same_as<std::size_t>;
      { mem.writeBlockSize() } -> std::same_as<std::size_t>;
      { mem.eraseBlock(addr) } -> std::same_as<m::Task<bool>>;
      { mem.writeBlock(addr, wdata) } -> std::same_as<m::Task<bool>>;
      { mem.read(addr, rdata) } -> std::same_as<m::Task<bool>>;
    };

static_assert(CFlashMemory<IFlashMemory>,
              "IFlashMemory must satisfy CFlashMemory concept");
}  // namespace m::ifc

#endif  // IFLASHMEMORY_HPP