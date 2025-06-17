/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef REG_MAP_HPP
#define REG_MAP_HPP

#include <concepts>
#include <type_traits>

namespace m {

template <auto Address, typename RegisterType>
struct RegInfo {
  static constexpr auto address = Address;
  using Type = RegisterType;
};

template <typename T>
concept CRegInfo = requires {
  T::address;
  typename T::Type;
};

template <typename AddressType, CRegInfo... RegInfos>
  requires std::is_integral_v<AddressType> && std::is_unsigned_v<AddressType>
class RegMap {
 public:
  template <typename RegisterType>
    requires(std::same_as<RegisterType, typename RegInfos::Type> || ...)
  static constexpr AddressType getAddress() {
    AddressType address{};
    ((address = std::is_same_v<RegisterType, typename RegInfos::Type>
                    ? static_cast<AddressType>(RegInfos::address)
                    : address),
     ...);
    return address;
  }
};

}  // namespace m

#endif  // REG_MAP_HPP