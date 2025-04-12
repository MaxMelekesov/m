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

#include <type_traits>
#include <utility>

namespace m {

struct SettingsTagBase {};

template <typename T>
concept CSettingTag = std::is_base_of_v<SettingsTagBase, T>;

template <typename Derived, typename Type, Type DefaultValue>
struct SettingTag : public SettingsTagBase {
  using ValueType = Type;
  static constexpr Type defaultValue = DefaultValue;
};

template <typename... SettingsTags>
struct SettingsStorage;

template <typename FirstTag, typename... RestTags>
struct SettingsStorage<FirstTag, RestTags...> {
  typename FirstTag::ValueType value;
  SettingsStorage<RestTags...> rest;

  constexpr SettingsStorage() : value(FirstTag::defaultValue), rest() {}

  template <CSettingTag Tag, typename Value>
  void setValue(Value &&newValue) {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      value = std::forward<Value>(newValue);
    } else {
      rest.template setValue<Tag>(std::forward<Value>(newValue));
    }
  }

  template <CSettingTag Tag>
  auto getValue() const {
    if constexpr (std::is_same_v<Tag, FirstTag>) {
      return value;
    } else {
      return rest.template getValue<Tag>();
    }
  }
};

template <typename LastTag>
struct SettingsStorage<LastTag> {
  typename LastTag::ValueType value;

  constexpr SettingsStorage() : value(LastTag::defaultValue) {}

  template <CSettingTag Tag, typename Value>
  void setValue(Value &&newValue) {
    if constexpr (std::is_same_v<Tag, LastTag>) {
      value = std::forward<Value>(newValue);
    }
  }

  template <CSettingTag Tag>
  auto getValue() const {
    if constexpr (std::is_same_v<Tag, LastTag>) {
      return value;
    }
  }
};

template <typename Derived, typename... SettingsTags>
class Settings {
 public:
  using StorageType = SettingsStorage<SettingsTags...>;

  template <CSettingTag TagType, typename Value>
  void setValue(Value &&value) {
    if (storage_.template getValue<TagType>() != value) {
      storage_.template setValue<TagType>(std::forward<Value>(value));
      has_changes_ = true;
    }
  }

  template <CSettingTag TagType>
  const auto getValue() const {
    return storage_.template getValue<TagType>();
  }

  bool hasChanges() const { return has_changes_; }

  bool save() {
    if (static_cast<Derived *>(this)->saveImpl(storage_)) {
      has_changes_ = false;
      return true;
    }
    return false;
  }

  bool load() { return static_cast<Derived *>(this)->loadImpl(storage_); }

  void resetToDefaults() {
    storage_ = StorageType{};
    has_changes_ = true;
  }

 private:
  StorageType storage_;
  bool has_changes_ = false;
};

}  // namespace m

#endif  // SETTINGS_HPP