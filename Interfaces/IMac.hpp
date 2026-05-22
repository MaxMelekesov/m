/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IMAC_HPP
#define IMAC_HPP

#include <concepts>
#include <cstdint>
#include <span>

namespace m::ifc {

template <typename T>
concept CMac = requires(T mac, const uint8_t* key,
                         std::span<const uint8_t> data,
                         uint8_t* tag, const uint8_t* ctag) {
  { mac.init(key) } -> std::same_as<void>;
  { mac.update(data) } -> std::same_as<void>;
  { mac.finalize(tag) } -> std::same_as<void>;
  { mac.verify(ctag) } -> std::same_as<bool>;
};

}  // namespace m::ifc

#endif  // IMAC_HPP
