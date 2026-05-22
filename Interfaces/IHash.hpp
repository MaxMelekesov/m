/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IHASH_HPP
#define IHASH_HPP

#include <array>
#include <cstdint>
#include <span>

namespace m::ifc {
template <uint32_t Hash_Bytes = 4>
class IHash {
 public:
  using Storage = std::array<uint8_t, Hash_Bytes>;
  virtual ~IHash() {}
  virtual bool check(std::span<uint8_t const> data, Storage& hash) = 0;

  virtual Storage calc(std::span<uint8_t const> data) = 0;

  constexpr uint32_t size() const { return Hash_Bytes; }
};

template <typename T>
concept CHash = requires(T hash, std::span<uint8_t const> data,
                         typename T::Storage& hash_val) {
  typename T::Storage;
  { hash.check(data, hash_val) } -> std::same_as<bool>;
  { hash.calc(data) } -> std::same_as<typename T::Storage>;
  { hash.size() } -> std::same_as<uint32_t>;
};

template <typename T, uint32_t Hash_Bytes>
concept CHashOf =
    CHash<T> &&
    std::same_as<typename T::Storage, std::array<uint8_t, Hash_Bytes>>;

static_assert(CHash<IHash<4>>, "IHash must satisfy CHash concept");
static_assert(CHashOf<IHash<4>, 4>, "IHash<4> must satisfy CHashOf<4> concept");
}  // namespace m::ifc

#endif  // IHASH_HPP