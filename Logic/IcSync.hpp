/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef IC_SYNC_HPP
#define IC_SYNC_HPP

#include <StaticMap.hpp>
#include <TupleContains.hpp>
#include <concepts>
#include <optional>

namespace m {

template <typename T>
concept CIcInfo = requires {
  typename T::Regs;
  typename T::Map;
  std::tuple_size_v<typename T::Regs>;
};

template <typename T, typename Reg>
concept CIcSync = requires(T t, Reg reg) {
  { t.template writeImpl<Reg>(reg) } -> std::same_as<bool>;
  { t.template readImpl<Reg>() } -> std::same_as<std::optional<Reg>>;
};

template <typename Derived, CIcInfo IcInfo>
class IcSync {
 public:
  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  bool write(Reg reg) {
    static_assert(CIcSync<Derived, Reg>,
                  "Derived must implement CIcSync interface");

    return static_cast<Derived*>(this)->template writeImpl<Reg>(reg);
  }

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  std::optional<Reg> read() {
    static_assert(CIcSync<Derived, Reg>,
                  "Derived must implement CIcSync interface");

    return static_cast<Derived*>(this)->template readImpl<Reg>();
  }
};

}  // namespace m

#endif  // IC_SYNC_HPP