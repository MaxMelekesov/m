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

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

/* Example usage:
 *
 * // Define bit fields
 * struct ConfigField : public m::BitField<4, ConfigField, 15> {};  // 4 bits,
 * default 15
 * struct DataField : public m::BitField<8, DataField, 0> {}; // 8
 * bits, default 0
 * struct EnableBit : public m::BitField<1, EnableBit, 1> {}; // 1 bit, default
 * 1
 *
 * // Create register using struct inheritance (recommended)
 * struct ControlRegister : public m::BitReg<std::uint32_t,
 *                                           ConfigField,       // Bits 0-3
 *                                           m::DummyField<4>,  // Bits 4-7
 *                                           DataField,         // Bits 8-15
 *                                           EnableBit,         // Bit 16
 *                                           m::DummyField<15>  // Bits 17-31
 *                                           > {};
 *
 * // Alternative: using alias (but struct inheritance is preferred)
 * using StatusRegister = m::BitReg<std::uint8_t,
 *                                  m::BitField<4, struct StatusField, 5>,
 *                                  m::DummyField<4>>;
 *
 * // Usage example:
 * ControlRegister ctrl;                    // Default constructor sets defaults
 * ctrl.set<ConfigField>(10);               // Set config field to 10
 * ctrl.set<DataField>(255);                // Set data field to 255
 * ctrl.set<EnableBit>(0);                  // Disable
 *
 * auto config = ctrl.get<ConfigField>();   // Returns 10
 * auto data = ctrl.get<DataField>();       // Returns 255
 * auto enabled = ctrl.get<EnableBit>();    // Returns 0
 *
 * std::uint32_t raw_value = ctrl.raw();    // Get raw register value
 * ctrl.raw(0x12345678);                    // Set raw register value
 * ctrl.reset();                            // Reset to default values
 */

namespace m {

template <std::size_t Size, typename Derived, auto DefaultValue = 0>
  requires(Size > 0) && (Size <= 64)
struct BitField {
  static constexpr std::size_t size = Size;
  static constexpr auto default_value = DefaultValue;
  static constexpr auto max_value = (1ULL << Size);

  static_assert(static_cast<uint64_t>(DefaultValue) < max_value,
                "Default value exceeds bit field capacity");
};

template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct DummyField : public BitField<Size, DummyField<Size>, 0> {};

template <typename T>
concept CBitField = requires {
  T::size;
  T::default_value;
} && std::derived_from<T, BitField<T::size, T, T::default_value>>;

template <typename Storage, CBitField... Fields>
  requires(sizeof...(Fields) > 0) && std::is_integral_v<Storage> &&
          std::is_unsigned_v<Storage>
class BitReg {
 public:
  using StorageType = Storage;

  constexpr BitReg() : data_(0) { ((setFieldDefault<Fields>()), ...); }

  explicit constexpr BitReg(Storage initial_value) : data_(initial_value) {}

  template <CBitField Field>
    requires(std::same_as<Field, Fields> || ...)
  constexpr auto get() const {
    constexpr auto field_info = getFieldInfo<Field>();
    return static_cast<decltype(field_info.default_value)>(
        (data_ >> field_info.offset) & field_info.field_mask);
  }

  template <CBitField Field, typename Value>
    requires(std::same_as<Field, Fields> || ...)
  constexpr void set(Value value) {
    constexpr auto field_info = getFieldInfo<Field>();
    const Storage masked_value =
        static_cast<Storage>(value) & field_info.field_mask;

    data_ = (data_ & ~field_info.register_mask) |
            (masked_value << field_info.offset);
  }

  constexpr Storage raw() const { return data_; }

  constexpr void raw(Storage value) { data_ = value; }

  constexpr void reset() {
    data_ = 0;
    ((setFieldDefault<Fields>()), ...);
  }

 private:
  Storage data_;

  static constexpr std::size_t total_size = (Fields::size + ...);
  static_assert(total_size <= sizeof(Storage) * 8,
                "Total field size exceeds storage type capacity");

  template <std::size_t Offset, std::size_t Size, auto DefaultValue>
  struct FieldInfo {
    static constexpr std::size_t offset = Offset;
    static constexpr std::size_t size = Size;
    static constexpr auto default_value = DefaultValue;
    static constexpr Storage field_mask = (Storage(1) << Size) - 1;
    static constexpr Storage register_mask = field_mask << Offset;
  };

  template <CBitField Field>
  static constexpr auto getFieldInfo() {
    constexpr std::size_t offset = getFieldOffset<Field>();
    return FieldInfo<offset, Field::size, Field::default_value>{};
  }

  template <CBitField Field>
  static constexpr std::size_t getFieldOffset() {
    std::size_t offset = 0;
    ((offset += (std::is_same_v<Field, Fields> ? 0 : Fields::size)), ...);
    return offset;
  }

  template <CBitField Field>
  constexpr void setFieldDefault() {
    if constexpr (!std::is_base_of_v<DummyField<Field::size>, Field>) {
      if constexpr (Field::default_value != 0) {
        set<Field>(Field::default_value);
      }
    }
  }
};

}  // namespace m

#endif  // BITREG_HPP
