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
  static constexpr auto max_value = (1ULL << Size) - 1;
};

template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct DummyField : public BitField<Size, DummyField<Size>> {};

template <typename T>
concept CBitField = requires {
  T::size;
  T::max_value;
} && std::is_base_of_v<BitField<T::size, T>, T>;

template <typename T>
concept CRegStorage = std::is_integral_v<T> && std::is_unsigned_v<T>;

template <CRegStorage Storage, CBitField... Fields>
  requires(sizeof...(Fields) > 0)
class RegBitMap {
 public:
  using StorageType = Storage;

  static constexpr std::size_t total_bits = (Fields::size + ...);
  static_assert(total_bits == sizeof(Storage) * 8,
                "Total field bits must match storage size");

  static constexpr Storage non_dummy_mask_ = []() constexpr {
    Storage mask = 0;
    std::size_t offset = 0;
    ((mask |= (!std::is_base_of_v<DummyField<Fields::size>, Fields>
                   ? (((Storage(1) << Fields::size) - 1) << offset)
                   : 0),
      offset += Fields::size),
     ...);
    return mask;
  }();

  template <CBitField Field>
  static constexpr Storage getField(Storage data) {
    constexpr auto info = getFieldInfo<Field>();
    return (data >> info.first) & info.second;
  }

  template <CBitField Field, typename Value>
  static constexpr Storage setField(Storage data, Value value) {
    constexpr auto info = getFieldInfo<Field>();
    constexpr Storage reg_mask = info.second << info.first;
    return (data & ~reg_mask) |
           ((static_cast<Storage>(value) & info.second) << info.first);
  }

 private:
  template <CBitField Field>
  static consteval auto getFieldInfo() {
    constexpr std::size_t offset = []() consteval {
      std::size_t off = 0;
      ((off += (std::is_same_v<Field, Fields> ? 0 : Fields::size)), ...);
      return off;
    }();
    constexpr Storage mask = (Storage(1) << Field::size) - 1;

    struct FieldInfo {
      std::size_t first;
      Storage second;
    };
    return FieldInfo{offset, mask};
  }
};

template <typename T>
concept CRegBitMap = requires {
  typename T::StorageType;
  T::non_dummy_mask_;
  T::total_bits;
} && CRegStorage<typename T::StorageType>;

// Concept for register info type
template <typename T>
concept CRegInfo = requires { typename T::BitFieldsOrder; } &&
                   CRegBitMap<typename T::BitFieldsOrder>;

template <CRegInfo InfoT>
class Reg {
  using BitFieldsOrder = typename InfoT::BitFieldsOrder;
  using StorageType = typename BitFieldsOrder::StorageType;

 public:
  constexpr Reg() = default;
  explicit constexpr Reg(StorageType value)
      : data_(value & BitFieldsOrder::non_dummy_mask_) {}

  template <CBitField Field>
  constexpr StorageType get() const {
    return BitFieldsOrder::template getField<Field>(data_);
  }

  template <CBitField Field>
  constexpr void set(auto value) {
    data_ = BitFieldsOrder::template setField<Field>(data_, value);
  }

  constexpr StorageType getRaw() const {
    return data_ & BitFieldsOrder::non_dummy_mask_;
  }
  constexpr void setRaw(StorageType value) {
    data_ = value & BitFieldsOrder::non_dummy_mask_;
  }

 private:
  StorageType data_{0};
};
}  // namespace m

#endif  // BITREG_HPP
