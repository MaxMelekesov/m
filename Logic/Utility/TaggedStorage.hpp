/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef TAGGEDSTORAGE_HPP
#define TAGGEDSTORAGE_HPP

#include <type_traits>
#include <utility>

namespace m {

/**
 * @brief Base class for all tags
 */
struct TagBase {};

template <typename T>
concept CTag = std::is_base_of_v<m::TagBase, T>;

/**
 * @brief Tag template for defining typed settings with default values
 *
 * @tparam Derived The derived tag class (CRTP pattern)
 * @tparam Type The data type of the value associated with this tag
 * @tparam DefaultValue The default value for this tag
 *
 * Example:
 *
 *     struct TemperatureTag : public Tag<TemperatureTag, int, 25> {};
 *     struct EnabledTag : public Tag<EnabledTag, bool, false> {};
 */
template <typename Derived, typename Type, Type DefaultValue>
struct Tag : public TagBase {
  using ValueType = Type;
  static constexpr Type default_value = DefaultValue;
};

/**
 * @brief A compile-time container for storing and retrieving values by their
 * tag types
 *
 * TaggedStorage provides type-safe access to a collection of values, each
 * identified by a unique tag type. This allows for strongly-typed access to
 * settings without string-based lookup or runtime overhead.
 *
 * Key features:
 * - All elements are stored sequentially in memory with defined layout
 * - Can be directly serialized and deserialized as binary data
 * - Supports persistence (saving/loading from storage)
 * - Zero runtime overhead for element access
 *
 * Example usage:
 *
 *     // Define tag types
 *     struct TemperatureTag : public Tag<int, 25> {};
 *     struct EnabledTag : public Tag<bool, false> {};
 *     struct NameTag : public Tag<const char*, "Default"> {};
 *
 *     // Create a storage with these tags
 *     TaggedStorage<TemperatureTag, EnabledTag, NameTag> storage;
 *
 *     // Get values (returns default values initially)
 *     int temp = storage.get<TemperatureTag>();      // temp = 25
 *     bool enabled = storage.get<EnabledTag>();      // enabled = false
 *
 *     // Set values
 *     storage.set<TemperatureTag>(30);
 *     storage.set<EnabledTag>(true);
 *
 *     // Get updated values
 *     temp = storage.get<TemperatureTag>();          // temp = 30
 *     enabled = storage.get<EnabledTag>();           // enabled = true
 *
 *     // Serialization example
 *     void saveToFlash(const TaggedStorage<TemperatureTag, EnabledTag>&
 * storage) {
 *         // Direct binary serialization is possible due to sequential memory
 * layout
 *          flash.write(0, &storage, sizeof(storage));
 *         // or
 *          std::array<uint8_t, sizeof(storage)> buf;
 *          m::serilaize(buf, storage);
 *     }
 *
 *     // Deserialization example
 *     void loadFromFlash(TaggedStorage<TemperatureTag, EnabledTag>& storage) {
 *         flash.read(0, &storage, sizeof(storage));
 *       // or
 *          std::array<uint8_t, sizeof(storage)> buf;
 *          m::deserilaize<decltype(storage)>(buf);
 *     }
 *
 * @tparam Tags The tag types that define the values stored in this container
 */
template <typename... Tags>
struct TaggedStorage;

/**
 * @brief Recursive case for TaggedStorage with multiple tags
 */
template <typename FirstTag, typename... RestTags>
struct TaggedStorage<FirstTag, RestTags...> {
  typename FirstTag::ValueType value;
  TaggedStorage<RestTags...> rest;

  constexpr TaggedStorage() : value(FirstTag::default_value), rest() {}

  template <typename Tag>
  auto get() {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template get<Tag>();
    }
  }

  template <typename Tag>
  auto get() const {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template get<Tag>();
    }
  }

  template <typename Tag, typename Value>
  void set(Value&& new_walue) {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      value = std::forward<Value>(new_walue);
    } else {
      rest.template set<Tag>(std::forward<Value>(new_walue));
    }
  }
};

/**
 * @brief Base case for TaggedStorage with a single tag
 */
template <typename LastTag>
struct TaggedStorage<LastTag> {
  typename LastTag::ValueType value;

  constexpr TaggedStorage() : value(LastTag::default_value) {}

  template <typename Tag>
  auto get() {
    return value;
  }

  template <typename Tag>
  auto get() const {
    return value;
  }

  template <typename Tag, typename Value>
  void set(Value&& new_walue) {
    value = std::forward<Value>(new_walue);
  }
};
}  // namespace m

#endif  // TAGGEDSTORAGE_HPP