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
template <uint32_t hash_bytes = 4>
class IHash {
 public:
  static constexpr uint32_t Hash_Bytes = hash_bytes;
  using type = std::array<uint8_t, hash_bytes>;
  virtual ~IHash() {}
  virtual bool check(std::span<uint8_t const> data, type& hash) = 0;

  virtual type calc(std::span<uint8_t const> data) = 0;

  constexpr uint32_t size() const { return hash_bytes; }
};

template <typename T>
concept CHash = requires(T hash, std::span<uint8_t const> data,
                         typename T::type& hash_val) {
  typename T::type;
  { hash.check(data, hash_val) } -> std::same_as<bool>;
  { hash.calc(data) } -> std::same_as<typename T::type>;
  { hash.size() } -> std::same_as<uint32_t>;
};

template <typename T, uint32_t hash_bytes>
concept CHashOf =
    CHash<T> && std::same_as<typename T::type, std::array<uint8_t, hash_bytes>>;

static_assert(CHash<IHash<4>>, "IHash must satisfy CHash concept");
static_assert(CHashOf<IHash<4>, 4>, "IHash<4> must satisfy CHashOf<4> concept");
}  // namespace m::ifc

#endif  // IHASH_HPP