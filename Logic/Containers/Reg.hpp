/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef BITREG_HPP
#define BITREG_HPP

#include <cstddef>
#include <type_traits>

namespace m {

template <std::size_t Size, typename Derived>
  requires(Size > 0) && (Size <= 64)
struct BitField {
  static constexpr std::size_t size = Size;
  static constexpr auto max_value = Size == 64 ? ~0ULL : (1ULL << Size) - 1;
};

template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct UnusedField : public BitField<Size, UnusedField<Size>> {};

template <typename T>
concept CBitField = requires {
  T::size;
  T::max_value;
} && std::is_base_of_v<BitField<T::size, T>, T>;

template <typename T>
concept CRegStorage = std::is_integral_v<T> && std::is_unsigned_v<T>;

template <CRegStorage Storage, CBitField... Fields>
  requires(sizeof...(Fields) > 0)
class BitReg {
 public:
  using StorageType = Storage;

  static constexpr std::size_t storage_bits = sizeof(StorageType) * 8;

  static consteval std::size_t sumFieldBits() {
    return (Fields::size + ... + 0);
  }

  static_assert(
      sumFieldBits() == storage_bits,
      "BitReg: Total size of all fields must match storage type bit width");

  constexpr BitReg() : data_(0) {}

  explicit constexpr BitReg(Storage value) : data_(value & non_dummy_mask_) {}

  template <CBitField Field>
    requires(std::is_same_v<Field, Fields> || ...)
  constexpr auto get() const {
    constexpr auto field_info = getFieldInfo<Field>();
    return (data_ >> field_info.offset) & field_info.field_mask;
  }

  template <CBitField Field, typename Value>
    requires(std::is_same_v<Field, Fields> || ...)
  constexpr void set(Value value) {
    constexpr auto field_info = getFieldInfo<Field>();
    const Storage masked_value =
        static_cast<Storage>(value) & field_info.field_mask;

    data_ = (data_ & ~field_info.register_mask) |
            (masked_value << field_info.offset);
  }

  constexpr Storage getRaw() const { return data_ & non_dummy_mask_; }

  constexpr void setRaw(Storage value) { data_ = value & non_dummy_mask_; }

 private:
  Storage data_;

  static constexpr std::size_t total_size = (Fields::size + ...);
  static_assert(total_size <= sizeof(Storage) * 8,
                "Total field size exceeds storage type capacity");

  template <std::size_t Offset, std::size_t Size>
  struct FieldInfo {
    static constexpr std::size_t offset = Offset;
    static constexpr std::size_t size = Size;
    static constexpr Storage field_mask =
        Size == (sizeof(Storage) * 8) ? ~Storage(0) : (Storage(1) << Size) - 1;
    static constexpr Storage register_mask = field_mask << Offset;
  };

  template <CBitField Field>
  static constexpr auto getFieldInfo() {
    constexpr std::size_t offset = getFieldOffset<Field>();
    return FieldInfo<offset, Field::size>{};
  }

  template <CBitField Field>
  static constexpr std::size_t getFieldOffset() {
    std::size_t offset = 0;
    bool found = false;
    ((found ? (void)0
            : (std::is_same_v<Field, Fields>
                   ? (found = true, void(0))
                   : (offset += Fields::size, void(0)))),
     ...);
    return offset;
  }

  // Compile-time mask for all non-dummy fields
  static constexpr Storage non_dummy_mask_ = []() constexpr {
    Storage mask = 0;
    std::size_t offset = 0;
    ((mask |= (!std::is_base_of_v<UnusedField<Fields::size>, Fields>
                   ? (((Storage(1) << Fields::size) - 1) << offset)
                   : 0),
      offset += Fields::size),
     ...);
    return mask;
  }();
};

}  // namespace m

#endif  // BITREG_HPP
