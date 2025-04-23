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

#include <TaggedStorage.hpp>

namespace m {

/**
 * @brief A template class for managing application settings with persistence
 *
 * The Settings class provides a type-safe interface for accessing and modifying
 * configuration values, with built-in change tracking and persistence support.
 * It uses the CRTP pattern (Curiously Recurring Template Pattern) to allow
 * derived classes to implement persistence mechanisms while reusing common
 * settings management logic.
 *
 * Key features:
 * - Type-safe access to settings via tag types
 * - Change tracking to optimize persistence operations
 * - Serializable storage with sequential memory layout
 * - Customizable persistence through derived classes
 *
 * @tparam Derived    The derived class that implements persistence methods
 * @tparam SettingsTags  The tag types that define available settings
 *
 * Example usage:
 *
 *     // Define tag types
 *     struct TemperatureTag : public Tag<int, 25> {};
 *     struct EnabledTag : public Tag<bool, false> {};
 *
 *     // Define settings manager with persistence
 *     class DeviceSettings : public Settings<DeviceSettings, TemperatureTag,
 * EnabledTag> { private: Flash& flash_;
 *
 *         // Implement required persistence methods
 *         bool saveImpl(StorageType& storage) {
 *             return flash_.write(0, &storage, sizeof(storage));
 *         }
 *
 *         bool loadImpl(StorageType& storage) {
 *             return flash_.read(0, &storage, sizeof(storage));
 *         }
 *
 *         // Friend declaration needed for CRTP
 *         friend Settings;
 *     };
 *
 *     // Usage example
 *     DeviceSettings settings(flash);
 *     settings.setValue<TemperatureTag>(30);
 *     settings.setValue<EnabledTag>(true);
 *     if(settings.hasChanges()) {
 *         settings.save();
 *     }
 *
 *     int temp = settings.getValue<TemperatureTag>(); // temp = 30
 *     bool enabled = settings.getValue<EnabledTag>(); // enabled = true
 */
template <typename Derived, CTag... SettingsTags>
class Settings {
 public:
  using StorageType = TaggedStorage<SettingsTags...>;

  template <CTag Tag>
    requires CIsStorageTag<Tag, SettingsTags...>
  void setValue(const typename Tag::ValueType& value) {
    if (storage_.template get<Tag>() != value) {
      storage_.template set<Tag>(value);
      has_changes_ = true;
      return static_cast<Derived*>(this)->newChangeImpl();
    }
  }

  template <CTag Tag>
    requires CIsStorageTag<Tag, SettingsTags...>
  auto getValue() const {
    return storage_.template get<Tag>();
  }

  bool hasChanges() const { return has_changes_; }

  bool save() {
    if (static_cast<Derived*>(this)->saveImpl(storage_)) {
      has_changes_ = false;
      return true;
    }
    return false;
  }

  bool load() { return static_cast<Derived*>(this)->loadImpl(storage_); }

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