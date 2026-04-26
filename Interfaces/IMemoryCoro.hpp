/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IMEMORY_CORO_HPP
#define IMEMORY_CORO_HPP

#include <CoroScheduler.hpp>
#include <cstdint>
#include <span>

namespace m::ifc {

class IMemoryCoro {
 public:
  virtual ~IMemoryCoro() {}

  virtual std::size_t size() = 0;
  virtual Task<bool> write(std::size_t addr, std::span<uint8_t const> data) = 0;
  virtual Task<bool> read(std::size_t addr, std::span<uint8_t> data) = 0;
};

template <typename T>
concept CMemoryCoro =
    requires(T mem, std::size_t addr, std::span<uint8_t const> wdata,
             std::span<uint8_t> rdata) {
      { mem.size() } -> std::same_as<std::size_t>;
      { mem.write(addr, wdata) } -> std::same_as<m::Task<bool>>;
      { mem.read(addr, rdata) } -> std::same_as<m::Task<bool>>;
    };

static_assert(CMemoryCoro<IMemoryCoro>,
              "IMemoryCoro must satisfy CMemoryCoro concept");

}  // namespace m::ifc

#endif  // IMEMORY_CORO_HPP