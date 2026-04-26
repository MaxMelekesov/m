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

/**
 * @brief CRTP base for synchronous register-level IC access.
 *
 * Usage:
 *
 * ```cpp
 * struct DevInfo {
 *   struct Status {
 *     m::Reg<uint16_t> value;
 *   };
 *   struct Config {
 *     m::Reg<uint16_t> value;
 *   };
 *
 *   using Regs = std::tuple<Status, Config>;
 *   struct Map : public m::StaticMap<uint8_t, m::Pair<Status, 0x00>,
 *                                    m::Pair<Config, 0x01>> {};
 * };
 *
 * class Dev : public m::ic::Ic<Dev, DevInfo> {
 *  public:
 *   explicit Dev(Bus& bus) : bus_(bus) {}
 *
 *  private:
 *   template <typename Reg>
 *   bool writeImpl(Reg reg) {
 *     return busWrite(DevInfo::Map::template value<Reg>(),
 *                     reg.value.getRaw());
 *   }
 *
 *   template <typename Reg>
 *   std::optional<Reg> readImpl() {
 *     auto raw = busRead(DevInfo::Map::template value<Reg>());
 *     if (!raw) {
 *       return std::nullopt;
 *     }
 *     return Reg{raw.value()};
 *   }
 *
 *   Bus& bus_;
 *   friend class m::ic::Ic<Dev, DevInfo>;
 * };
 *
 * Dev dev{bus};
 * DevInfo::Config cfg{};
 * bool write_ok = dev.write(cfg);
 * auto status = dev.read<DevInfo::Status>();
 * ```
 */

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
concept CIcRegReadable = requires(T t, Reg reg) {
  { t.template readImpl<Reg>() } -> std::same_as<std::optional<Reg>>;
};

template <typename T, typename Reg>
concept CIcRegWritable = requires(T t, Reg reg) {
  { t.template writeImpl<Reg>(reg) } -> std::same_as<bool>;
};

template <typename Derived, CIcInfo IcInfo>
class Ic {
 public:
  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  bool write(Reg reg) {
    static_assert(CIcRegWritable<Derived, Reg>,
                  "Derived must implement CIc interface");

    return static_cast<Derived*>(this)->template writeImpl<Reg>(reg);
  }

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  std::optional<Reg> read() {
    static_assert(CIcRegReadable<Derived, Reg>,
                  "Derived must implement CIc interface");

    return static_cast<Derived*>(this)->template readImpl<Reg>();
  }
};

}  // namespace m::ic

#endif  // IC_HPP