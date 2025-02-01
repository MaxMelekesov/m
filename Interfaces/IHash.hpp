/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IHASH_H
#define IHASH_H

#include <array>
#include <cstdint>
#include <span>

namespace m::ifc {
template <uint32_t hash_bytes = 4>
class IHash {
 public:
  using type = std::array<uint8_t, hash_bytes>;
  virtual ~IHash() {}
  virtual bool check(std::span<uint8_t const> data, type& hash) = 0;

  virtual type calc(std::span<uint8_t const> data) = 0;

  constexpr uint32_t size() const { return hash_bytes; }
};
}  // namespace m::ifc

#endif  // IHASH_H