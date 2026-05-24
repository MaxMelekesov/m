/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ISTREAMCIPHER_HPP
#define ISTREAMCIPHER_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ifc {

class IStreamCipher {
 public:
  virtual ~IStreamCipher() = default;

  virtual std::size_t keySize() = 0;
  virtual std::size_t nonceSize() = 0;

  virtual void init(std::span<const uint8_t> key, std::span<const uint8_t> nonce) = 0;

  virtual void process(std::span<uint8_t> data) = 0;

  /// Zero all internal state (key, nonce, counter).
  virtual void wipe() = 0;
};

template <typename T>
concept CStreamCipher =
    requires(T cipher, std::span<const uint8_t> key, std::span<const uint8_t> nonce,
             std::span<uint8_t> data) {
      { cipher.keySize() } -> std::same_as<std::size_t>;
      { cipher.nonceSize() } -> std::same_as<std::size_t>;
      { cipher.init(key, nonce) } -> std::same_as<void>;
      { cipher.process(data) } -> std::same_as<void>;
      { cipher.wipe() } -> std::same_as<void>;
    };

static_assert(CStreamCipher<IStreamCipher>,
              "IStreamCipher must satisfy CStreamCipher concept");

}  // namespace m::ifc

#endif  // ISTREAMCIPHER_HPP
