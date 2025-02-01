/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TSERDES_H
#define TSERDES_H

#include <cstdint>
#include <cstring>
#include <span>
#include <tuple>
#include <utility>

namespace m {

// ##################################################
// Usage Example:
// std::array<uint8_t, 10> buf;
// float f = 123.5f;
// uint32_t u4 = 156;
// uint16_t u2 = 122;
// auto bytes_used = m::serialize(buf, f, u2, u4);
// auto [f_value, u2_value, u4_value] = m::deserialize<float, uint16_t,
// uint32_t>(buf);
// ##################################################

template <typename T>
constexpr inline T __createFromBytes(const uint8_t* data) {
  T result;
  std::memcpy(&result, data, sizeof(T));
  return result;
}

template <typename... Ts, std::size_t... Is>
constexpr inline std::tuple<Ts...> __deserializeImpl(
    const uint8_t* data, std::index_sequence<Is...>) {
  std::size_t offset = 0;
  return std::tuple<Ts...>{(__createFromBytes<Ts>(
      data + std::exchange(offset, offset + sizeof(Ts))))...};
}

template <typename... Ts>
constexpr inline std::tuple<Ts...> deserialize(
    const std::span<uint8_t const>& span) {
  return __deserializeImpl<Ts...>(span.data(),
                                  std::index_sequence_for<Ts...>{});
}

template <typename... Args>
std::size_t serialize(std::span<uint8_t> bytes, Args&&... args) {
  std::size_t offset = 0;
  ((std::memcpy(bytes.data() + offset, &args, sizeof(args)),
    offset += sizeof(args)),
   ...);
  return offset;
}

}  // namespace m

#endif  // TSERDES_H
