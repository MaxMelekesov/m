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
 * // Define bit field types using CRTP
 * struct ModeField : public m::BitField<2, ModeField, 0> {};
 * struct EnableField : public m::BitField<1, EnableField, 0> {};
 * struct ValueField : public m::BitField<10, ValueField, 12> {};
 *
 * // Create register with fields (ordered from LSB to MSB)
 * using MyRegister = m::Register<uint16_t, ModeField, m::DummyField<3>,
 *                                ValueField, EnableField>;
 *
 * MyRegister reg;
 *
 * // Access fields by tag
 * reg.set<ModeField>(1);
 * reg.set<ValueField>(13);
 *
 * auto mode = reg.get<ModeField>();    // mode = 1
 * auto value = reg.get<ValueField>();  // value = 13
 * ```
 */

namespace m {

// Concept for integral types that can be used as register storage
template <typename T>
concept CRegisterType = std::is_integral_v<T> && std::is_unsigned_v<T>;

// BitField template using CRTP - represents a bit field with size, tag, and
// default value
template <std::size_t Size, typename Derived, auto DefaultValue = 0>
  requires(Size > 0) && (Size <= 64)
struct BitField {
  using TagType = Derived;
  static constexpr std::size_t size = Size;
  static constexpr auto default_value = DefaultValue;
  static constexpr auto max_value = (1ULL << Size) - 1;

  // Validate that default value fits in the bit field
  static_assert(static_cast<std::uint64_t>(DefaultValue) <= max_value,
                "Default value exceeds bit field capacity");
};

// Concept for bit field tags (now they inherit from BitField)
template <typename T>
concept CBitFieldTag =
    std::is_base_of_v<BitField<T::size, T, T::default_value>, T>;

// DummyField - represents unused bits in a register
template <std::size_t Size>
  requires(Size > 0) && (Size <= 64)
struct DummyField {
  struct DummyTag {};
  using TagType = DummyTag;
  static constexpr std::size_t size = Size;
  static constexpr int default_value = 0;
};

namespace {
// Helper to check if a tag exists in the field list
template <typename Tag, typename... Fields>
concept CIsRegisterTag = (std::is_same_v<Tag, typename Fields::TagType> || ...);

// Helper to get field type by tag
template <typename Tag, typename FirstField, typename... RestFields>
constexpr auto getFieldByTag() {
  if constexpr (std::is_same_v<Tag, typename FirstField::TagType>) {
    return FirstField{};
  } else if constexpr (sizeof...(RestFields) > 0) {
    return getFieldByTag<Tag, RestFields...>();
  }
}

// Helper to calculate bit offset for a field by tag
template <typename Tag, typename FirstField, typename... RestFields>
constexpr std::size_t calculateOffsetByTag() {
  if constexpr (std::is_same_v<Tag, typename FirstField::TagType>) {
    return 0;  // Found the field, offset is 0 from here
  } else if constexpr (sizeof...(RestFields) > 0) {
    return FirstField::size + calculateOffsetByTag<Tag, RestFields...>();
  } else {
    // This should never be reached due to concept requirements
    return 0;
  }
}
}  // anonymous namespace

// Register template - main bit register class
template <CRegisterType StorageType, typename... Fields>
  requires(sizeof...(Fields) > 0)
class Register {
 public:
  // Constructor - initialize with default values
  constexpr Register() : data_(0) {
    // Set default values for all fields
    ((setFieldDefault<Fields>()), ...);
  }

  // Constructor with initial value
  explicit constexpr Register(StorageType initial_value)
      : data_(initial_value) {}

  // Get field value by tag
  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  constexpr auto get() const {
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr std::size_t size = getSize<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;

    return static_cast<decltype(getFieldByTag<Tag, Fields...>().default_value)>(
        (data_ >> offset) & field_mask);
  }

  // Set field value by tag
  template <typename Tag, typename Value>
    requires CIsRegisterTag<Tag, Fields...>
  constexpr void set(Value value) {
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr std::size_t size = getSize<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;
    constexpr StorageType register_mask = getMask<Tag>();

    // Validate value fits in the field
    const StorageType masked_value =
        static_cast<StorageType>(value) & field_mask;

    // Clear field bits and set new value
    data_ = (data_ & ~register_mask) | (masked_value << offset);
  }

  // Get raw register value
  constexpr StorageType raw() const { return data_; }

  // Set raw register value
  constexpr void raw(StorageType value) { data_ = value; }

  // Reset register to default values
  constexpr void reset() {
    data_ = 0;
    ((setFieldDefault<Fields>()), ...);
  }

 private:
  StorageType data_;

  // Calculate total size of all fields
  static constexpr std::size_t total_size = (Fields::size + ...);
  static_assert(total_size <= sizeof(StorageType) * 8,
                "Total field size exceeds storage type capacity");

  // Get field offset by tag
  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr std::size_t getOffset() {
    return calculateOffsetByTag<Tag, Fields...>();
  }

  // Get field size by tag
  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr std::size_t getSize() {
    return decltype(getFieldByTag<Tag, Fields...>())::size;
  }

  // Get field mask by tag
  template <typename Tag>
    requires CIsRegisterTag<Tag, Fields...>
  static constexpr StorageType getMask() {
    constexpr std::size_t size = getSize<Tag>();
    constexpr std::size_t offset = getOffset<Tag>();
    constexpr StorageType field_mask = (StorageType(1) << size) - 1;
    return field_mask << offset;
  }

  // Helper to set default value for a field
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
