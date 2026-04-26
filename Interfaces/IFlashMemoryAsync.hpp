/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IFLASHMEMORYASYNC_HPP
#define IFLASHMEMORYASYNC_HPP

#include <cstdint>
#include <span>

namespace m::ifc {
class [[deprecated("remove")]] IFlashMemoryAsync {
 public:
  virtual ~IFlashMemoryAsync() {}

  virtual std::size_t sectorSize() = 0;
  virtual std::size_t sectorCount() = 0;

  virtual bool startErase(std::size_t addr, std::size_t sectors) = 0;
  virtual bool eraseDone() = 0;

  virtual bool startWrite(std::size_t addr, std::span<uint8_t const> data) = 0;
  virtual bool isWriteDone() = 0;

  virtual bool startRead(std::size_t addr, std::span<uint8_t> data) = 0;
  virtual bool isReadDone() = 0;

  virtual bool error() = 0;
};

template <typename T>
concept CFlashMemoryAsync =
    requires(T mem, std::size_t addr, std::size_t sz,
             std::span<uint8_t const> wdata, std::span<uint8_t> rdata) {
      { mem.sectorSize() } -> std::same_as<std::size_t>;
      { mem.sectorCount() } -> std::same_as<std::size_t>;
      { mem.startErase(addr, sz) } -> std::same_as<bool>;
      { mem.eraseDone() } -> std::same_as<bool>;
      { mem.startWrite(addr, wdata) } -> std::same_as<bool>;
      { mem.isWriteDone() } -> std::same_as<bool>;
      { mem.startRead(addr, rdata) } -> std::same_as<bool>;
      { mem.isReadDone() } -> std::same_as<bool>;
      { mem.error() } -> std::same_as<bool>;
    };

static_assert(CFlashMemoryAsync<IFlashMemoryAsync>,
              "IFlashMemoryAsync must satisfy CFlashMemoryAsync concept");
}  // namespace m::ifc

#endif  // IFLASHMEMORYASYNC_HPP