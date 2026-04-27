/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef TAGGED_STORAGE_HPP
#define TAGGED_STORAGE_HPP

#include <type_traits>
#include <utility>

/* Example usage:
 *
 * ```cpp
 * // Define tag types
 * struct TemperatureTag : public m::Tag<int, 25> {};
 * struct EnabledTag : public m::Tag<bool, false> {};
 *
 * // Create storage for tags
 * m::TaggedStorage<TemperatureTag, EnabledTag> storage;
 *
 * // Access and modify values
 * int temp = storage.get<TemperatureTag>();      // temp = 25 (default)
 * bool enabled = storage.get<EnabledTag>();      // enabled = false (default)
 *
 * storage.set<TemperatureTag>(30);
 * storage.set<EnabledTag>(true);
 *
 * temp = storage.get<TemperatureTag>();          // temp = 30
 * enabled = storage.get<EnabledTag>();           // enabled = true
 * ```
 */

namespace m {

template <typename Type, Type DefaultValue>
struct Tag {
  using ValueType = Type;
  static constexpr Type default_value = DefaultValue;
};

template <typename T>
concept CTag =
    std::is_base_of_v<Tag<typename T::ValueType, T::default_value>, T>;

template <typename Tag, typename... Tags>
concept CIsStorageTag = (std::is_same_v<Tag, Tags> || ...);

template <typename Tag, typename Value>
concept CIsTagValueType = std::is_convertible_v<Value, typename Tag::ValueType>;

template <CTag... Tags>
struct TaggedStorage {
  using type = std::tuple<Tags...>;
};

template <CTag FirstTag, CTag... RestTags>
struct TaggedStorage<FirstTag, RestTags...> {
  using type = std::tuple<FirstTag, RestTags...>;

  typename FirstTag::ValueType value;
  [[no_unique_address]] TaggedStorage<RestTags...> rest;

  constexpr TaggedStorage() : value(FirstTag::default_value), rest() {}

  template <CTag Tag>
    requires CIsStorageTag<Tag, FirstTag, RestTags...>
  auto get() {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template get<Tag>();
    }
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, FirstTag, RestTags...>
  auto get() const {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template get<Tag>();
    }
  }

  template <CTag Tag, typename Value>
    requires CIsStorageTag<Tag, FirstTag, RestTags...> &&
             CIsTagValueType<Tag, Value>
  void set(Value&& new_walue) {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      value = std::forward<Value>(new_walue);
    } else {
      rest.template set<Tag>(std::forward<Value>(new_walue));
    }
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, FirstTag, RestTags...>
  auto& getRef() {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template getRef<Tag>();
    }
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, FirstTag, RestTags...>
  const auto& getRef() const {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template getRef<Tag>();
    }
  }
};

template <CTag LastTag>
struct TaggedStorage<LastTag> {
  using type = std::tuple<LastTag>;

  typename LastTag::ValueType value;

  constexpr TaggedStorage() : value(LastTag::default_value) {}

  template <CTag Tag>
    requires CIsStorageTag<Tag, LastTag>
  auto get() {
    return value;
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, LastTag>
  auto get() const {
    return value;
  }

  template <CTag Tag, typename Value>
    requires CIsStorageTag<Tag, LastTag> && CIsTagValueType<Tag, Value>
  void set(Value&& new_walue) {
    value = std::forward<Value>(new_walue);
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, LastTag>
  auto& getRef() {
    return value;
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, LastTag>
  const auto& getRef() const {
    return value;
  }
};
}  // namespace m

#endif  // TAGGED_STORAGE_HPP