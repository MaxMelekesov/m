/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISTREAMHASH_HPP
#define ISTREAMHASH_HPP

#include <array>
#include <concepts>
#include <cstdint>
#include <span>

namespace m::ifc {

template <uint32_t Hash_Bytes = 4>
class IStreamHash {
 public:
  using Storage = std::array<uint8_t, Hash_Bytes>;
  virtual ~IStreamHash() = default;

  virtual void update(std::span<const uint8_t> data) = 0;
  virtual Storage finalize() = 0;

  constexpr uint32_t size() const { return Hash_Bytes; }
};

template <typename T>
concept CStreamHash = requires(T hash, std::span<const uint8_t> data,
                               typename T::Storage& hash_val) {
  typename T::Storage;
  { hash.update(data) } -> std::same_as<void>;
  { hash.finalize() } -> std::same_as<typename T::Storage>;
  { hash.size() } -> std::same_as<uint32_t>;
};

template <typename T, uint32_t Hash_Bytes>
concept CStreamHashOf =
    CStreamHash<T> &&
    std::same_as<typename T::Storage, std::array<uint8_t, Hash_Bytes>>;

static_assert(CStreamHash<IStreamHash<4>>,
              "IStreamHash must satisfy CStreamHash concept");
static_assert(CStreamHashOf<IStreamHash<4>, 4>,
              "IStreamHash<4> must satisfy CStreamHashOf<4> concept");

}  // namespace m::ifc

#endif  // ISTREAMHASH_HPP
