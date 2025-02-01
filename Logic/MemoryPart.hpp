/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MEMORYPART_H
#define MEMORYPART_H

#include <IMemory.hpp>
#include <cstdint>
#include <span>

namespace m {
class MemoryPart : public ifc::IMemory {
 private:
  ifc::IMemory &mem_;
  std::size_t const offset_;
  std::size_t const size_;

 public:
  MemoryPart(ifc::IMemory &mem, std::size_t offset, std::size_t size)
      : mem_(mem), offset_(offset), size_(size) {}

  std::size_t size() override { return size_; }

  bool write(std::size_t addr, std::span<uint8_t const> data) override {
    return mem_.write(offset_ + addr, data);
  }
  bool read(std::size_t addr, std::span<uint8_t> data) override {
    return mem_.read(offset_ + addr, data);
  }
};
}  // namespace m

#endif  // MEMORYPART_H