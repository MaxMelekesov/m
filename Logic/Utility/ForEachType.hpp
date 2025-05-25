/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef FOR_EACH_TYPE_HPP
#define FOR_EACH_TYPE_HPP

#include <tuple>
#include <type_traits>

namespace m {

/* Пример использования:

#include <iostream>
#include <string>
#include "for_each_type.hpp"

int main() {
    auto printer = []<typename T>() {
        std::cout << "Type: " << typeid(T).name() << '\n';
    };

    std::cout << "Processing std::tuple:\n";
    m::for_each_type<std::tuple<int, float, std::string>>(printer);

    std::cout << "\nProcessing TypeList:\n";
    using MyTypes = m::TypeList<bool, char, double>;
    m::for_each_type<MyTypes>(printer);

    return 0;
}
*/

template <typename... Ts>
struct TypeList {};

template <typename TList, typename = void>
struct list_traits {
  static constexpr bool is_supported = false;
};

template <template <typename...> class Tuple, typename... Ts>
struct list_traits<Tuple<Ts...>, std::void_t<std::tuple_size<Tuple<Ts...>>>> {
  static constexpr bool is_supported = true;
  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I>
  using type_at = std::tuple_element_t<I, Tuple<Ts...>>;
};

template <typename... Ts>
struct list_traits<TypeList<Ts...>> {
  static constexpr bool is_supported = true;
  static constexpr std::size_t size = sizeof...(Ts);
  template <std::size_t I>
  using type_at = std::tuple_element_t<I, std::tuple<Ts...>>;
};

template <typename TList, typename F>
constexpr void for_each_type(F&& func) {
  using traits = list_traits<TList>;
  static_assert(traits::is_supported, "TList is not a supported type list");

  constexpr std::size_t N = traits::size;
  [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    (func.template operator()<typename traits::template type_at<Is>>(), ...);
  }(std::make_index_sequence<N>{});
}

}  // namespace m
#endif  // FOR_EACH_TYPE_HPP