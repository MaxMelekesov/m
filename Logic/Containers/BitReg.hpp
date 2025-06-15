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
 * ```cpp
 *
 * // Example with DummyField for unused bits
 * struct ConfigField : public m::BitField<4, ConfigField, 15> {};
 * struct DataField : public m::BitField<8, DataField, 0> {};
 *
 * // 32-bit register with unused bits
 * // Layout: Config(4) + Dummy(4) + Data(8) + Dummy(16) = 32 bits
 * using ControlRegister = m::Register<std::uint32_t,
 *                                   ConfigField,       // Bits 0-3
 *                                   m::DummyField<4>,  // Bits 4-7 (unused)
 *                                   DataField,         // Bits 8-15
 *                                   m::DummyField<16>  // Bits 16-31 (unused)
 *                                   >;
 *
 * ControlRegister ctrl;
 * ctrl.set<ConfigField>(10);  // Only tagged fields are accessible
 * ctrl.set<DataField>(255);   // Dummy fields are automatically skipped
 * ```
 */

namespace m {

template <typename T>
concept CRegisterType = std::is_integral_v<T> && std::is_unsigned_v<T>;

template <std::size_t Size, typename Derived, auto DefaultValue = 0>
  requires(Size > 0) && (Size <= 64)
struct BitField {
  using TagType = Derived;
  static constexpr std::size_t size = Size;
  static constexpr auto default_value = DefaultValue;
  static constexpr auto max_value = (1ULL << Size) - 1;

  static_assert(static_cast<std::uint64_t>(DefaultValue) <= max_value,
                "Default value exceeds bit field capacity");
};

template <typename T>
concept CBitFieldTag =
    std::is_base_of_v<BitField<T::size, T, T::default_value>, T>;

template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct DummyField {
  struct DummyTag {};
  using TagType = DummyTag;
  static constexpr std::size_t size = Size;
  static constexpr int default_value = 0;
};

namespace {

template <typename Tag, typename... Fields>
concept CIsRegisterTag = (std::is_same_v<Tag, typename Fields::TagType> || ...);

template <typename Tag, typename FirstField, typename... RestFields>
constexpr auto getFieldByTag() {
  if constexpr (std::is_same_v<Tag, typename FirstField::TagType>) {
    return FirstField{};
  } else if constexpr (sizeof...(RestFields) > 0) {
    return getFieldByTag<Tag, RestFields...>();
  }
}

template <typename Tag, typename FirstField, typename... RestFields>
constexpr std::size_t calculateOffsetByTag() {
  if constexpr (std::is_same_v<Tag, typename FirstField::TagType>) {
    return 0;
  } else if constexpr (sizeof...(RestFields) > 0) {
    return FirstField::size + calculateOffsetByTag<Tag, RestFields...>();
  } else {
    return 0;
  }
}
}  // namespace

template <CRegisterType StorageType, typename... Fields>
  requires(sizeof...(Fields) > 0)
class Register {
 public:
  constexpr Register() : data_(0) { ((setFieldDefault<Fields>()), ...); }

  explicit constexpr Register(StorageType initial_value)
      : data_(initial_value) {}

  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  constexpr auto get() const {
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr std::size_t size = getSize<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;

    return static_cast<decltype(getFieldByTag<Tag, Fields...>().default_value)>(
        (data_ >> offset) & field_mask);
  }

  template <typename Tag, typename Value>
    requires CIsRegisterTag<Tag, Fields...>
  constexpr void set(Value value) {
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr std::size_t size = getSize<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;
    constexpr StorageType register_mask = getMask<Tag>();

    const StorageType masked_value =
        static_cast<StorageType>(value) & field_mask;

    data_ = (data_ & ~register_mask) | (masked_value << offset);
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

  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr std::size_t getOffset() {
    return calculateOffsetByTag<Tag, Fields...>();
  }

  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr std::size_t getSize() {
    return decltype(getFieldByTag<Tag, Fields...>())::size;
  }

  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr StorageType getMask() {
    constexpr std::size_t size = getSize<Tag>();
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;
    return field_mask << offset;
  }

  template <typename Field>
  constexpr void setFieldDefault() {
    if constexpr (!std::is_same_v<typename Field::TagType,
                                  typename DummyField<Field::size>::DummyTag>) {
      if constexpr (Field::default_value != 0) {
        set<typename Field::TagType>(Field::default_value);
      }
    }
  }
};

}  // namespace m

#endif  // BITREG_HPP
