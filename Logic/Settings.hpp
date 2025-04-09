/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <tuple>
#include <type_traits>

namespace m {

// Базовый класс для концепции тега
struct TagBase {};

// CRTP реализация для TagValue
template <typename Derived, typename Type, Type DefaultValue>
struct TagValue : public TagBase {
  using TagType = Derived;
  using ValueType = Type;
  static constexpr Type defaultValue = DefaultValue;

  // Гарантируем, что Derived наследуется от TagValue
  constexpr TagValue() {
    static_assert(std::is_base_of_v<TagValue, Derived>,
                  "Tag must inherit from TagValue");
  }
};

// Вспомогательная проверка для TagValue
template <typename T> struct IsTagValue : std::false_type {};

template <typename Derived, typename Type, Type DefaultValue>
struct IsTagValue<TagValue<Derived, Type, DefaultValue>> : std::true_type {};

template <typename T>
inline constexpr bool is_tag_value_v = IsTagValue<T>::value;

// Специализация ValueStorage теперь для конкретных типов тегов
template <typename... Tags> struct ValueStorage {
  std::tuple<typename Tags::ValueType...> values = {Tags::defaultValue...};
};

template <typename Derived, typename... Tags> class Settings {
protected:
  using ValueStorageType = ValueStorage<Tags...>;

private:
  ValueStorageType storage;
  bool has_changes_ = false;

public:
  template <typename Tag, typename ValueType> void setValue(ValueType &&value) {
    static_assert(std::is_base_of_v<TagBase, Tag>,
                  "Tag must be derived from TagBase");
    static_assert(tagExists<Tag>(), "Tag not found in settings pairs");

    constexpr size_t index = getIndex<Tag>();
    using ActualValueType =
        std::tuple_element_t<index, decltype(storage.values)>;

    static_assert(std::is_convertible_v<ValueType, ActualValueType>,
                  "Incompatible value type for this setting");

    auto &currentValue = std::get<index>(storage.values);
    if (currentValue != value) {
      currentValue = std::forward<ValueType>(value);
      has_changes_ = true;
    }
  }

  template <typename Tag> typename Tag::ValueType getValue() const {
    static_assert(tagExists<Tag>(), "Tag not found in settings pairs");

    constexpr size_t index = getIndex<Tag>();
    return std::get<index>(storage.values);
  }

  template <typename Tag, typename ValueType>
  bool hasValue(const ValueType &value) const {
    static_assert(tagExists<Tag>(), "Tag not found in settings pairs");
    return getValue<Tag>() == value;
  }

  bool save() {
    if (static_cast<Derived *>(this)->saveImpl(storage)) {
      has_changes_ = false;
      return true;
    } else {
      return false;
    }
  }

  bool load() { return static_cast<Derived *>(this)->loadImpl(storage); }

  bool hasChanges() const { return has_changes_; }

private:
  template <typename Tag> static constexpr bool tagExists() {
    return (std::is_same_v<Tags, Tag> || ...);
  }

  template <typename Tag> static constexpr size_t getIndex() {
    return getIndexImpl<Tag, 0, Tags...>();
  }

  template <typename Tag, size_t Idx, typename CurrentTag, typename... Rest>
  static constexpr size_t getIndexImpl() {
    if constexpr (std::is_same_v<CurrentTag, Tag>) {
      return Idx;
    } else {
      return getIndexImpl<Tag, Idx + 1, Rest...>();
    }
  }

  template <typename Tag, size_t Idx> static constexpr size_t getIndexImpl() {
    static_assert(sizeof...(Tags) > 0, "Tag not found in settings pairs");
    return Idx;
  }
};
} // namespace m

#endif // SETTINGS_HPP