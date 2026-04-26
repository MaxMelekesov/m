/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IC_CORO_HPP
#define IC_CORO_HPP

#include <CoroScheduler.hpp>
#include <Ic.hpp>
#include <TupleContains.hpp>
#include <concepts>
#include <optional>

/**
 * @brief CRTP base for coroutine register-level IC access.
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
 * class DevCoro : public m::ic::IcCoro<DevCoro, DevInfo> {
 *  public:
 *   explicit DevCoro(BusAsync& bus) : bus_(bus) {}
 *
 *  private:
 *   template <typename Reg>
 *   m::Task<bool> writeImpl(Reg reg) {
 *     co_return co_await busWriteAsync(DevInfo::Map::template value<Reg>(),
 *                                      reg.value.getRaw());
 *   }
 *
 *   template <typename Reg>
 *   m::Task<std::optional<Reg>> readImpl() {
 *     auto raw = co_await busReadAsync(DevInfo::Map::template value<Reg>());
 *     if (!raw) {
 *       co_return std::nullopt;
 *     }
 *     co_return Reg{raw.value()};
 *   }
 *
 *   BusAsync& bus_;
 *   friend class m::ic::IcCoro<DevCoro, DevInfo>;
 * };
 *
 * m::Task<void> appTask(DevCoro& dev) {
 *   DevInfo::Config cfg{};
 *   bool write_ok = co_await dev.write(cfg);
 *   auto status = co_await dev.read<DevInfo::Status>();
 *   (void)write_ok;
 *   (void)status;
 * }
 * ```
 */

namespace m::ic {

template <typename T, typename Reg>
concept CIcRegReadableCoro = requires(T t, Reg reg) {
  { t.template readImpl<Reg>() } -> std::same_as<m::Task<std::optional<Reg>>>;
};

template <typename T, typename Reg>
concept CIcRegWritableCoro = requires(T t, Reg reg) {
  { t.template writeImpl<Reg>(reg) } -> std::same_as<m::Task<bool>>;
};

template <typename Derived, CIcInfo IcInfo>
class IcCoro {
 public:
  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  m::Task<bool> write(Reg reg) {
    static_assert(CIcRegWritableCoro<Derived, Reg>,
                  "Derived must implement coroutine CIc interface");

    co_return co_await static_cast<Derived*>(this)->template writeImpl<Reg>(
        reg);
  }

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  m::Task<std::optional<Reg>> read() {
    static_assert(CIcRegReadableCoro<Derived, Reg>,
                  "Derived must implement coroutine CIc interface");

    co_return co_await static_cast<Derived*>(this)->template readImpl<Reg>();
  }
};

}  // namespace m::ic

#endif  // IC_CORO_HPP
