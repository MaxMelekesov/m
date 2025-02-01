/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef HASHFAQ6_H
#define HASHFAQ6_H

#include <IHash.hpp>
#include <TSerDes.hpp>
#include <cstdint>
#include <span>

namespace m {
class HashFAQ6 final : public ifc::IHash<4> {
 public:
  using Hash = m::ifc::IHash<4>::type;

  bool check(std::span<uint8_t const> data, Hash& hash) override {
    auto calculated = calc(data);
    auto res = hash == calculated;
    return res;
  }

  Hash calc(std::span<uint8_t const> data) override {
    Hash hash;
    auto temp = faq6(data);
    m::serialize(hash, temp);
    return hash;
  }

 private:
  static uint32_t faq6(std::span<uint8_t const> data) {
    uint32_t hash = 0;

    for (uint32_t i = 0; i < data.size(); i++) {
      hash += data[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
  }
};
}  // namespace m

#endif  // HASHFAQ6_H