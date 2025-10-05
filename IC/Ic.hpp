/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */
#ifndef IC_HPP
#define IC_HPP

#include <StaticMap.hpp>
#include <TupleContains.hpp>
#include <concepts>
#include <optional>
#include <tuple>

namespace m::ic {

template <typename T>
concept CIcInfo = requires {
  typename T::Regs;
  typename T::Map;
  []<typename... Args>(std::tuple<Args...>*) {
  }(static_cast<typename T::Regs*>(nullptr));
  requires m::CStaticMap<typename T::Map>;
};

template <typename T, typename Reg>
concept CIc = requires(T t, Reg reg) {
  { t.template writeImpl<Reg>(reg) } -> std::same_as<bool>;
  { t.template readImpl<Reg>() } -> std::same_as<std::optional<Reg>>;
};

template <typename Derived, CIcInfo IcInfo>
class Ic {
 public:
  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  bool write(Reg reg) {
    static_assert(CIc<Derived, Reg>, "Derived must implement CIc interface");

    return static_cast<Derived*>(this)->template writeImpl<Reg>(reg);
  }

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  std::optional<Reg> read() {
    static_assert(CIc<Derived, Reg>, "Derived must implement CIc interface");

    return static_cast<Derived*>(this)->template readImpl<Reg>();
  }
};

}  // namespace m::ic

#endif  // IC_HPP