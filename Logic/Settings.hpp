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
#include <utility>

namespace m {

// Базовый класс для тегов
struct TagBase {};

// Концепт для проверки, что тип является тегом
template <typename T>
concept Tag = std::is_base_of_v<TagBase, T>;

// Класс для определения тега настройки с типом значения и значением по умолчанию
template <typename Derived, typename Type, Type DefaultValue>
struct TagValue : public TagBase {
  using ValueType = Type;
  static constexpr Type defaultValue = DefaultValue;
};

// Хранилище значений настроек
template <typename Derived, typename... Tags>
class Settings {
 private:
  std::tuple<typename Tags::ValueType...> values_ =
      std::make_tuple(Tags::defaultValue...);
  bool has_changes_ = false;  // Флаг для отслеживания изменений

 public:
  // Установить значение по тегу
  template <Tag TagType, typename Value>
  void setValue(Value&& value) {
    constexpr std::size_t index = getIndex<TagType>();
    auto& current_value = std::get<index>(values_);
    if (current_value != value) {  // Проверяем, изменилось ли значение
      current_value = std::forward<Value>(value);
      has_changes_ = true;  // Устанавливаем флаг изменений
    }
  }

  // Проверить, есть ли изменения
  bool hasChanges() const {
    return has_changes_;
  }

  // Сохранить настройки
  bool save() {
    return static_cast<Derived*>(this)->saveImpl(values_);
  }

  // Загрузить настройки
  bool load() {
    return static_cast<Derived*>(this)->loadImpl(values_);
  }

  // Получить значение по тегу
  template <Tag TagType>
  const auto& getValue() const {
    constexpr std::size_t index = getIndex<TagType>();
    return std::get<index>(values_);
  }

  // Сбросить все значения к значениям по умолчанию
  void resetToDefaults() {
    values_ = std::make_tuple(Tags::defaultValue...);
    has_changes_ = true;  // Устанавливаем флаг изменений
  }

 private:
  // Получить индекс тега в tuple
  template <typename TagType, std::size_t Index = 0>
  static constexpr std::size_t getIndex() {
    if constexpr (std::is_same_v<std::tuple_element_t<Index, std::tuple<Tags...>>, TagType>) {
      return Index;
    } else {
      return getIndex<TagType, Index + 1>();
    }
  }
};

}  // namespace m

#endif  // SETTINGS_HPP