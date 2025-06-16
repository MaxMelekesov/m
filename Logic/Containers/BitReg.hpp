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
#include <cstdint>
#include <type_traits>

/* Example usage:

 *
 * // Example with DummyField for unused bits
 * struct ConfigField : public m::BitField<4, ConfigField, 15> {};
 * struct DataField : public m::BitField<8, DataField, 0> {};
 *
 * // 32-bit register with unused bits
 * // Layout: Config(4) + Dummy(4) + Data(8) + Dummy(16) = 32 bits
 * using ControlRegister = m::BitReg<std::uint32_t,
 *                                   ConfigField,       // Bits 0-3
 *                                   m::DummyField<4>,  // Bits 4-7 (unused)
 *                                   DataField,         // Bits 8-15
 *                                   m::DummyField<16>  // Bits 16-31 (unused)
 *                                   >;
 *
 * ControlRegister ctrl;
 * ctrl.set<ConfigField>(10);
 * ctrl.set<DataField>(255);
 */

namespace m {

template <std::size_t Size, typename Derived, auto DefaultValue = 0>
  requires(Size > 0) && (Size <= 64)
struct BitField {
  static constexpr std::size_t size = Size;
  static constexpr auto default_value = DefaultValue;
  static constexpr auto max_value = (1ULL << Size);

  static_assert(static_cast<std::uint64_t>(DefaultValue) < max_value,
                "Default value exceeds bit field capacity");
};

template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct DummyField : public BitField<Size, DummyField<Size>, 0> {};

template <typename T>
concept CBitField = requires {
  T::size;
  T::default_value;
  T::max_value;
} && std::is_base_of_v<BitField<T::size, T, T::default_value>, T>;

template <typename Field, typename... Fields>
concept CField = (std::is_same_v<Field, Fields> || ...);

template <typename T>
concept CRegStorage = std::is_integral_v<T> && std::is_unsigned_v<T>;

template <CRegStorage StorageType, CBitField... Fields>
  requires(sizeof...(Fields) > 0)
class BitReg {
 public:
  constexpr BitReg() : data_(0) { ((setFieldDefault<Fields>()), ...); }

  explicit constexpr BitReg(StorageType initial_value) : data_(initial_value) {}

  template <CBitField Field>
    requires CField<Field, Fields...>
  constexpr auto get() const {
    constexpr auto field_info = getFieldInfo<Field, Fields...>();

    return static_cast<decltype(field_info.default_value)>(
        (data_ >> field_info.offset) & field_info.field_mask);
  }

  template <CBitField Field, typename Value>
    requires CField<Field, Fields...>
  constexpr void set(Value value) {
    constexpr auto field_info = getFieldInfo<Field, Fields...>();
    const StorageType masked_value = static_cast<StorageType>(value);

    data_ = (data_ & ~field_info.register_mask) |
            (masked_value << field_info.offset);
  }

  constexpr StorageType raw() const { return data_; }

  constexpr void raw(StorageType value) { data_ = value; }

  constexpr void reset() {
    data_ = 0;
    ((setFieldDefault<Fields>()), ...);
  }

 private:
  StorageType data_;

  static constexpr std::size_t total_size = (Fields::size + ...);
  static_assert(total_size <= sizeof(StorageType) * 8,
                "Total field size exceeds storage type capacity");

  template <std::size_t Offset, std::size_t Size, auto DefaultValue>
  struct FieldInfo {
    static constexpr std::size_t offset = Offset;
    static constexpr std::size_t size = Size;
    static constexpr auto default_value = DefaultValue;
    static constexpr StorageType field_mask = (StorageType(1) << Size) - 1;
    static constexpr StorageType register_mask = field_mask << Offset;
  };

  template <CBitField Field, CBitField FirstField, CBitField... RestFields>
  static constexpr auto getFieldInfo() {
    if constexpr (std::is_same_v<Field, FirstField>) {
      return FieldInfo<0, FirstField::size, FirstField::default_value>{};
    } else if constexpr (sizeof...(RestFields) > 0) {
      auto next_info = getFieldInfo<Field, RestFields...>();
      return FieldInfo<FirstField::size + next_info.offset, next_info.size,
                       next_info.default_value>{};
    }
  }

  template <CBitField Field>
  constexpr void setFieldDefault() {
    if constexpr (!std::is_same_v<Field, DummyField<Field::size>>) {
      if constexpr (Field::default_value != 0) {
        set<Field>(Field::default_value);
      }
    }
  }
};

}  // namespace m

#endif  // BITREG_HPP
