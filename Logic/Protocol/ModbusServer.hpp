/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_SERVER_HPP
#define MODBUS_SERVER_HPP

#include <CoroDelay.hpp>
#include <CoroScheduler.hpp>
#include <CoroYield.hpp>
#include <IDataLink.hpp>
#include <ITime.hpp>
#include <Us.hpp>
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <expected>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace m {

/* ===========================================================================
 * Modbus RTU server — compile-time key dispatch, zero-overhead feature gating.
 *
 * Usage:
 *
 *   // 1. Define register keys (address embedded in type).
 *   struct MyRegs {
 *     struct Address  : m::ModbusReg<uint16_t, m::ModbusAccess::HoldingRW,
 * 0x00> {}; struct Baudrate : m::ModbusReg<uint32_t,
 * m::ModbusAccess::HoldingRW, 0x01> {}; using Keys = std::tuple<Address,
 * Baudrate>;
 *   };
 *
 *   // 2. Inherit ModbusHandler<RegInfo, Derived> — implement onRead/onWrite.
 *   struct MyHandler : m::ModbusHandler<MyRegs, MyHandler> {
 *     using Base = m::ModbusHandler<MyRegs, MyHandler>;
 *     MyHandler(uint8_t addr, …) : Base(addr), … {}
 *
 *     uint16_t onRead(m::ModbusKey<MyRegs::Address>) { return …; }
 *     void     onWrite(m::ModbusKey<MyRegs::Baudrate>, uint32_t v) { …; }
 *
 *     // onWrite can return std::optional<ModbusRtuError> for validation.
 *   };
 *
 *   // 3. Create handler(s), inject into ModbusServer.
 *   MyHandler h{0x01, …};
 *   m::ModbusServer server{
 *       data_link, time, {300_Us}, rx_buf, tx_buf,
 *       std::move(h)
 *       // optional LED callbacks (zero overhead if omitted):
 *       , []{ rx_led.toggle(); }, []{ tx_led.toggle(); }
 *   };
 *
 *   // 4. Coroutine loop.
 *   while (1) {
 *     if (!co_await server.coroRun()) {  }
 *     co_await m::coroYield();
 *   }
 *
 * Multiple Modbus addresses:
 *
 *   MyHandler h1{0x01, …};
 *   OtherHandler h2{0x02, …};
 *   m::ModbusServer server{data_link, time, {300_Us}, rx_buf, tx_buf,
 *                          std::move(h1), std::move(h2)};
 *   // Broadcast writes (addr=0) are forwarded to ALL handlers.
 *
 * Compile-time checks:
 *   • Address overlap within same category → static_assert.
 *   • Duplicate key types in Keys tuple → static_assert.
 *   • Unused function codes (e.g. no coils) → zero flash (if constexpr).
 */

// ============================================================================
// Modbus error codes
// ============================================================================

enum class ModbusRtuError : uint8_t {
  IllegalFunction = 1,
  IllegalDataAddress = 2,
  IllegalDataValue = 3,
  SlaveDeviceFailure = 4,
  Acknowledge = 5,
  SlaveDeviceBusy = 6,
  MemoryParityError = 8,
};

// ============================================================================
// ModbusAccess — register kind + r/w
// ============================================================================

enum class ModbusAccess : uint8_t {
  HoldingRO,
  HoldingWO,
  HoldingRW,
  CoilRO,
  CoilRW,
  DiscreteRO,
};

namespace detail {

inline constexpr bool isHolding(ModbusAccess a) {
  return a == ModbusAccess::HoldingRO || a == ModbusAccess::HoldingWO ||
         a == ModbusAccess::HoldingRW;
}
inline constexpr bool isCoil(ModbusAccess a) {
  return a == ModbusAccess::CoilRO || a == ModbusAccess::CoilRW;
}
inline constexpr bool isDiscrete(ModbusAccess a) {
  return a == ModbusAccess::DiscreteRO;
}
inline constexpr bool isReadable(ModbusAccess a) {
  return a == ModbusAccess::HoldingRO || a == ModbusAccess::HoldingRW ||
         a == ModbusAccess::CoilRO || a == ModbusAccess::CoilRW ||
         a == ModbusAccess::DiscreteRO;
}
inline constexpr bool isWritable(ModbusAccess a) {
  return a == ModbusAccess::HoldingWO || a == ModbusAccess::HoldingRW ||
         a == ModbusAccess::CoilRW;
}

template <typename... Keys>
inline constexpr bool hasAnyHolding =
    (isHolding(std::remove_cvref_t<Keys>::access) || ...);

template <typename... Keys>
inline constexpr bool hasAnyCoil =
    (isCoil(std::remove_cvref_t<Keys>::access) || ...);

template <typename... Keys>
inline constexpr bool hasAnyDiscrete =
    (isDiscrete(std::remove_cvref_t<Keys>::access) || ...);

template <typename Tuple>
struct HasAnyHolding;
template <typename... Ks>
struct HasAnyHolding<std::tuple<Ks...>>
    : std::bool_constant<hasAnyHolding<Ks...>> {};

template <typename Tuple>
struct HasAnyCoil;
template <typename... Ks>
struct HasAnyCoil<std::tuple<Ks...>> : std::bool_constant<hasAnyCoil<Ks...>> {};

template <typename Tuple>
struct HasAnyDiscrete;
template <typename... Ks>
struct HasAnyDiscrete<std::tuple<Ks...>>
    : std::bool_constant<hasAnyDiscrete<Ks...>> {};

// ── Compile-time address-overlap detection ────────────────────────────────
template <typename Tuple>
consteval bool allAddressesUnique() {
  constexpr auto N = std::tuple_size_v<Tuple>;
  if constexpr (N <= 1) return true;

  enum class Cat : uint8_t { Holding, Coil, Discrete };

  struct Info {
    uint16_t addr;
    uint16_t count;
    Cat cat;
  };

  constexpr auto infos = []<std::size_t... Is>(std::index_sequence<Is...>) {
    return std::array<Info, N>{{
        {std::tuple_element_t<Is, Tuple>::address,
         std::tuple_element_t<Is, Tuple>::isHolding
             ? std::tuple_element_t<Is, Tuple>::Type::regCount
             : 1U,
         std::tuple_element_t<Is, Tuple>::isHolding ? Cat::Holding
         : std::tuple_element_t<Is, Tuple>::isCoil  ? Cat::Coil
                                                    : Cat::Discrete}...,
    }};
  }(std::make_index_sequence<N>{});

  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = i + 1; j < N; ++j) {
      if (infos[i].cat != infos[j].cat) continue;
      if (infos[i].cat == Cat::Holding) {
        if (infos[i].addr < infos[j].addr + infos[j].count &&
            infos[j].addr < infos[i].addr + infos[i].count)
          return false;
      } else {
        if (infos[i].addr == infos[j].addr) return false;
      }
    }
  }
  return true;
}

// ── Compile-time duplicate-type detection ─────────────────────────────────
template <typename Tuple>
consteval bool allTypesUnique() {
  constexpr auto N = std::tuple_size_v<Tuple>;
  if constexpr (N <= 1) return true;
  return []<std::size_t... Is>(std::index_sequence<Is...>) {
    return ([]<std::size_t I>() {
      return []<std::size_t... Js>(std::index_sequence<Js...>) {
        return ([]<std::size_t J>() {
          if constexpr (I >= J) return true;
          return !std::is_same_v<std::tuple_element_t<I, Tuple>,
                                 std::tuple_element_t<J, Tuple>>;
        }.template operator()<Js>() &&
                ...);
      }(std::make_index_sequence<N>{});
    }.template operator()<Is>() &&
            ...);
  }(std::make_index_sequence<N>{});
}

struct NoOpCb {
  void operator()() const {}
};

}  // namespace detail

template <typename T>
struct ModbusType {
  using ValueType = T;
  static constexpr size_t paddedSize = (sizeof(T) + 1U) & ~1U;
  static constexpr uint16_t regCount = static_cast<uint16_t>(paddedSize / 2U);
  static_assert(paddedSize >= sizeof(T));
};

// ============================================================================
// ModbusReg<T, Access, Addr> — holding register definition
// ============================================================================

template <typename T, ModbusAccess Access, uint16_t Addr>
  requires(detail::isHolding(Access))
struct ModbusReg {
  using ValueType = T;
  using Type = ModbusType<T>;
  static constexpr auto access = Access;
  static constexpr uint16_t address = Addr;
  static constexpr bool isHolding = true;
  static constexpr bool isCoil = false;
  static constexpr bool isDiscrete = false;
};

// ============================================================================
// ModbusCoil<Access, Addr> — coil / discrete input definition
// ============================================================================

template <ModbusAccess Access, uint16_t Addr>
  requires(detail::isCoil(Access) || detail::isDiscrete(Access))
struct ModbusCoil {
  using ValueType = bool;
  static constexpr auto access = Access;
  static constexpr uint16_t address = Addr;
  static constexpr bool isHolding = false;
  static constexpr bool isCoil = detail::isCoil(Access);
  static constexpr bool isDiscrete = detail::isDiscrete(Access);
};

// ============================================================================
// CModbusKey — any register/coil key
// ============================================================================

template <typename T>
concept CModbusKey = requires {
  typename T::ValueType;
  { T::access } -> std::same_as<const ModbusAccess&>;
  { T::address } -> std::same_as<const uint16_t&>;
  { T::isHolding } -> std::convertible_to<bool>;
  { T::isCoil } -> std::convertible_to<bool>;
  { T::isDiscrete } -> std::convertible_to<bool>;
};

// ============================================================================
// CModbusRegInfo — the register info struct
// ============================================================================

template <typename T>
concept CModbusRegInfo = requires {
  typename T::Keys;
  []<typename... Ks>(std::tuple<Ks...>*) {
    ([]<CModbusKey> {}.template operator()<Ks>(), ...);
  }(static_cast<typename T::Keys*>(nullptr));
};

// ============================================================================
// ModbusKey<K> — tag for dispatch
// ============================================================================

template <typename K>
struct ModbusKey {
  using KeyType = K;
};

// ============================================================================
// ModbusHandler<RegInfo, Derived> — CRTP dispatch (onRead / onWrite)
//
// Usage:
//
//   struct MyRegs { … };
//   struct MyHandler : m::ModbusHandler<MyRegs, MyHandler> {
//     using Base = m::ModbusHandler<MyRegs, MyHandler>;
//     MyHandler(uint8_t addr, …) : Base(addr), … {}
//     uint16_t onRead(m::ModbusKey<MyRegs::Address>) { … }
//   };
// ============================================================================

template <CModbusRegInfo RegInfo, typename Derived>
class ModbusHandler {
  static constexpr bool Has_Holding =
      detail::HasAnyHolding<typename RegInfo::Keys>::value;
  static constexpr bool Has_Coil =
      detail::HasAnyCoil<typename RegInfo::Keys>::value;
  static constexpr bool Has_Discrete =
      detail::HasAnyDiscrete<typename RegInfo::Keys>::value;

  static_assert(detail::allAddressesUnique<typename RegInfo::Keys>(),
                "Modbus register addresses overlap or conflict");
  static_assert(detail::allTypesUnique<typename RegInfo::Keys>(),
                "Duplicate Modbus register key type in Keys tuple");

 public:
  using Error = ModbusRtuError;

  explicit ModbusHandler(uint8_t address) : address_(address) {}

  void setAddress(uint8_t a) { address_ = a; }
  uint8_t getAddress() const { return address_; }

  // ── Dispatch entry points (called by ModbusServer) ─────────────────────

  /// Handle a unicast command (addr matched).
  /// Returns expected resp_size or error.
  std::expected<uint32_t, Error> dispatch(uint8_t cmd, std::span<uint8_t> req,
                                          std::span<uint8_t> resp) {
    std::optional<Error> err = Error::IllegalFunction;
    uint32_t resp_size = 0;

    switch (cmd) {
      case 0x03:
        if constexpr (Has_Holding) {
          err = readHR(req, resp);
          if (!err) resp_size = static_cast<uint32_t>(resp[0]) + 1U;
        }
        break;
      case 0x06:
        if constexpr (Has_Holding) {
          err = writeSingleHR(req, resp);
          if (!err) resp_size = 4;
        }
        break;
      case 0x10:
        if constexpr (Has_Holding) {
          err = writeMultipleHR(req, resp);
          if (!err) resp_size = 4;
        }
        break;
      case 0x01:
        if constexpr (Has_Coil) {
          err = readCoils(req, resp);
          if (!err) resp_size = static_cast<uint32_t>(resp[0]) + 1U;
        }
        break;
      case 0x05:
        if constexpr (Has_Coil) {
          err = writeSingleCoil(req, resp);
          if (!err) resp_size = 4;
        }
        break;
      case 0x0F:
        if constexpr (Has_Coil) {
          err = writeMultipleCoils(req, resp);
          if (!err) resp_size = 4;
        }
        break;
      case 0x02:
        if constexpr (Has_Discrete) {
          err = readDiscrete(req, resp);
          if (!err) resp_size = static_cast<uint32_t>(resp[0]) + 1U;
        }
        break;
      case 0x04:
        if constexpr (Has_Holding) {
          err = readHR(req, resp);
          if (!err) resp_size = static_cast<uint32_t>(resp[0]) + 1U;
        }
        break;
      default:
        break;
    }
    if (err) return std::unexpected(*err);
    return resp_size;
  }

  /// Handle a broadcast command (addr == 0).  Only writes are processed.
  void broadcastWrite(uint8_t cmd, std::span<uint8_t> req,
                      std::span<uint8_t> resp) {
    if constexpr (Has_Holding) {
      if (cmd == 0x06)
        writeSingleHR(req, resp);
      else if (cmd == 0x10)
        writeMultipleHR(req, resp);
    }
    if constexpr (Has_Coil) {
      if (cmd == 0x05)
        writeSingleCoil(req, resp);
      else if (cmd == 0x0F)
        writeMultipleCoils(req, resp);
    }
  }

 private:
  Derived& self() { return static_cast<Derived&>(*this); }
  uint8_t address_;

  // ═══════════════════════════════════════════════════════════════════════
  // Holding registers (0x03, 0x06, 0x10, 0x04)
  // ═══════════════════════════════════════════════════════════════════════

  std::optional<Error> readHR(std::span<uint8_t> rx_buf,
                              std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) return Error::IllegalDataValue;
    const uint16_t start = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t num = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    if (num < 1 || num > 0x00'7D) return Error::IllegalDataValue;
    if (static_cast<uint32_t>(start) + num > 0xFF'FF)
      return Error::IllegalDataAddress;

    const uint32_t bc = static_cast<uint32_t>(num) * 2U;
    if (bc + 1U > tx_buf.size()) return Error::SlaveDeviceFailure;
    tx_buf[0] = static_cast<uint8_t>(bc);
    auto regs = tx_buf.subspan(1, bc);

    bool matched = false;
    std::optional<Error> err;
    std::apply(
        [&](auto... keys) {
          (tryReadHRKey(start, num, regs, keys, err, matched), ...);
        },
        typename RegInfo::Keys{});
    if (!matched) return Error::IllegalDataAddress;
    if (err) return err;
    swapBytesInSpan(regs);
    return std::nullopt;
  }

  template <typename Key>
  void tryReadHRKey(uint16_t addr, uint16_t num, std::span<uint8_t> regs,
                    Key /*tag*/, std::optional<Error>& err, bool& matched) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isHolding) return;
    if (matched) return;
    if (K::address != addr) return;
    matched = true;
    if (num != K::Type::regCount) {
      err = Error::IllegalDataValue;
      return;
    }
    if constexpr (detail::isReadable(K::access)) {
      auto v = self().onRead(ModbusKey<K>{});
      checkReadReturnType<K>(v);
      std::memset(regs.data(), 0, K::Type::paddedSize);
      std::memcpy(regs.data(), &v, sizeof(v));
    } else {
      err = Error::IllegalFunction;
    }
  }

  std::optional<Error> writeSingleHR(std::span<uint8_t> rx_buf,
                                     std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) return Error::IllegalDataValue;
    const uint16_t addr = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t value = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];

    bool matched = false;
    std::optional<Error> err;
    std::apply(
        [&](auto... keys) {
          (tryWriteSingleHRKey(addr, value, keys, err, matched), ...);
        },
        typename RegInfo::Keys{});
    if (!matched) return Error::IllegalDataAddress;
    if (err) return err;
    std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
    return std::nullopt;
  }

  template <typename Key>
  void tryWriteSingleHRKey(uint16_t addr, uint16_t value, Key /*tag*/,
                           std::optional<Error>& err, bool& matched) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isHolding) return;
    if (matched) return;
    if (K::address != addr) return;
    matched = true;
    if (K::Type::regCount != 1) {
      err = Error::IllegalDataValue;
      return;
    }
    if constexpr (detail::isWritable(K::access)) {
      typename K::ValueType v{};
      std::memcpy(&v, &value, sizeof(v));
      err = callOnWrite<K>(ModbusKey<K>{}, v);
    } else {
      err = Error::IllegalFunction;
    }
  }

  std::optional<Error> writeMultipleHR(std::span<uint8_t> rx_buf,
                                       std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) return Error::IllegalDataValue;
    const uint16_t start = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t num = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    const uint8_t bc = rx_buf[4];
    if (num < 1 || num > 0x00'7B || bc != num * 2U)
      return Error::IllegalDataValue;
    if (static_cast<uint32_t>(start) + num > 0xFF'FF)
      return Error::IllegalDataAddress;
    if (rx_buf.size() != static_cast<std::size_t>(bc) + 5U)
      return Error::IllegalDataValue;

    auto regs = rx_buf.subspan(5, num * 2U);
    swapBytesInSpan(regs);

    bool matched = false;
    std::optional<Error> err;
    std::apply(
        [&](auto... keys) {
          (tryWriteMultiHRKey(start, num, regs, keys, err, matched), ...);
        },
        typename RegInfo::Keys{});
    if (!matched) return Error::IllegalDataAddress;
    if (err) return err;
    std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
    return std::nullopt;
  }

  template <typename Key>
  void tryWriteMultiHRKey(uint16_t addr, uint16_t num, std::span<uint8_t> regs,
                          Key /*tag*/, std::optional<Error>& err,
                          bool& matched) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isHolding) return;
    if (matched) return;
    if (K::address != addr) return;
    matched = true;
    if (num != K::Type::regCount) {
      err = Error::IllegalDataValue;
      return;
    }
    if constexpr (detail::isWritable(K::access)) {
      typename K::ValueType v{};
      if (regs.size() < sizeof(v)) {
        err = Error::IllegalDataValue;
        return;
      }
      std::memcpy(&v, regs.data(), sizeof(v));
      err = callOnWrite<K>(ModbusKey<K>{}, v);
    } else {
      err = Error::IllegalFunction;
    }
  }

  // ═══════════════════════════════════════════════════════════════════════
  // Coils (0x01, 0x05, 0x0F)
  // ═══════════════════════════════════════════════════════════════════════

  std::optional<Error> readCoils(std::span<uint8_t> rx_buf,
                                 std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) return Error::IllegalDataValue;
    const uint16_t start = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t num = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    if (num < 1 || num > 0x07D0) return Error::IllegalDataValue;
    if (static_cast<uint32_t>(start) + num > 0xFFFF)
      return Error::IllegalDataAddress;

    const uint32_t bc = (num + 7U) / 8U;
    if (bc + 1U > tx_buf.size()) return Error::SlaveDeviceFailure;
    tx_buf[0] = static_cast<uint8_t>(bc);
    std::memset(&tx_buf[1], 0, bc);

    bool any_match = false;
    std::apply(
        [&](auto... keys) {
          ((tryReadCoilKey(start, num, tx_buf, keys, any_match)), ...);
        },
        typename RegInfo::Keys{});
    if (!any_match) return Error::IllegalDataAddress;
    return std::nullopt;
  }

  template <typename Key>
  void tryReadCoilKey(uint16_t start, uint16_t num, std::span<uint8_t> tx_buf,
                      Key /*tag*/, bool& any_match) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isCoil) return;
    if (K::address < start || K::address >= start + num) return;
    if constexpr (detail::isReadable(K::access)) {
      bool v = self().onRead(ModbusKey<K>{});
      any_match = true;
      const uint16_t off = K::address - start;
      if (v) tx_buf[1 + off / 8U] |= static_cast<uint8_t>(1U << (off % 8U));
    }
  }

  std::optional<Error> writeSingleCoil(std::span<uint8_t> rx_buf,
                                       std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) return Error::IllegalDataValue;
    const uint16_t addr = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t raw = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    if (raw != 0x00'00U && raw != 0xFF'00U) return Error::IllegalDataValue;
    const bool value = (raw == 0xFF'00U);

    bool matched = false;
    std::optional<Error> err;
    std::apply(
        [&](auto... keys) {
          (tryWriteSingleCoilKey(addr, value, keys, err, matched), ...);
        },
        typename RegInfo::Keys{});
    if (!matched) return Error::IllegalDataAddress;
    if (err) return err;
    std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
    return std::nullopt;
  }

  template <typename Key>
  void tryWriteSingleCoilKey(uint16_t addr, bool value, Key /*tag*/,
                             std::optional<Error>& err, bool& matched) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isCoil) return;
    if (matched) return;
    if (K::address != addr) return;
    matched = true;
    if constexpr (detail::isWritable(K::access))
      err = callOnWrite<K>(ModbusKey<K>{}, value);
    else
      err = Error::IllegalFunction;
  }

  std::optional<Error> writeMultipleCoils(std::span<uint8_t> rx_buf,
                                          std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) return Error::IllegalDataValue;
    const uint16_t start = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t num = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    const uint8_t bc = rx_buf[4];
    if (num < 1 || num > 0x07'B0 || bc != (num + 7U) / 8U)
      return Error::IllegalDataValue;
    if (static_cast<uint32_t>(start) + num > 0xFFFF)
      return Error::IllegalDataAddress;
    if (rx_buf.size() != static_cast<std::size_t>(bc) + 5U)
      return Error::IllegalDataValue;

    auto bits = rx_buf.subspan(5, bc);
    std::optional<Error> first_err;
    std::apply(
        [&](auto... keys) {
          ((tryWriteMultiCoilKey(start, num, bits, keys, first_err)), ...);
        },
        typename RegInfo::Keys{});
    if (first_err) return first_err;
    std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
    return std::nullopt;
  }

  template <typename Key>
  void tryWriteMultiCoilKey(uint16_t start, uint16_t num,
                            std::span<const uint8_t> bits, Key /*tag*/,
                            std::optional<Error>& first_err) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isCoil) return;
    if (K::address < start || K::address >= start + num) return;
    if constexpr (detail::isWritable(K::access)) {
      const uint16_t off = K::address - start;
      const bool v = (bits[off / 8U] >> (off % 8U)) & 1U;
      auto err = callOnWrite<K>(ModbusKey<K>{}, v);
      if (err && !first_err) first_err = err;
    }
  }

  // ═══════════════════════════════════════════════════════════════════════
  // Discrete inputs (0x02)
  // ═══════════════════════════════════════════════════════════════════════

  std::optional<Error> readDiscrete(std::span<uint8_t> rx_buf,
                                    std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) return Error::IllegalDataValue;
    const uint16_t start = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t num = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    if (num < 1 || num > 0x07D0) return Error::IllegalDataValue;
    if (static_cast<uint32_t>(start) + num > 0xFFFF)
      return Error::IllegalDataAddress;

    const uint32_t bc = (num + 7U) / 8U;
    if (bc + 1U > tx_buf.size()) return Error::SlaveDeviceFailure;
    tx_buf[0] = static_cast<uint8_t>(bc);
    std::memset(&tx_buf[1], 0, bc);

    bool any_match = false;
    std::apply(
        [&](auto... keys) {
          ((tryReadDiscreteKey(start, num, tx_buf, keys, any_match)), ...);
        },
        typename RegInfo::Keys{});
    if (!any_match) return Error::IllegalDataAddress;
    return std::nullopt;
  }

  template <typename Key>
  void tryReadDiscreteKey(uint16_t start, uint16_t num,
                          std::span<uint8_t> tx_buf, Key /*tag*/,
                          bool& any_match) {
    using K = std::remove_cvref_t<Key>;
    if constexpr (!K::isDiscrete) return;
    if (K::address < start || K::address >= start + num) return;
    if constexpr (detail::isReadable(K::access)) {
      bool v = self().onRead(ModbusKey<K>{});
      any_match = true;
      const uint16_t off = K::address - start;
      if (v) tx_buf[1 + off / 8U] |= static_cast<uint8_t>(1U << (off % 8U));
    }
  }

  // ═══════════════════════════════════════════════════════════════════════
  // Helpers
  // ═══════════════════════════════════════════════════════════════════════

  template <typename K, typename V>
  static constexpr void checkReadReturnType(const V&) {
    if constexpr (K::isHolding)
      static_assert(
          std::is_same_v<std::remove_cvref_t<V>, typename K::ValueType>,
          "onRead return type must match ModbusReg<T,...>");
    else
      static_assert(std::is_same_v<std::remove_cvref_t<V>, bool>,
                    "onRead for coil/discrete must return bool");
  }

  template <typename K, typename V>
  std::optional<Error> callOnWrite(ModbusKey<K> key, V&& v) {
    if constexpr (std::is_void_v<decltype(self().onWrite(
                      key, std::forward<V>(v)))>) {
      self().onWrite(key, std::forward<V>(v));
      return std::nullopt;
    } else {
      return self().onWrite(key, std::forward<V>(v));
    }
  }

  static void swapBytesInSpan(std::span<uint8_t> s) {
    for (std::size_t i = 0; i + 1U < s.size(); i += 2U)
      std::swap(s[i], s[i + 1U]);
  }
};

// ============================================================================
// ModbusServer<TimeUsT, Handlers..., OnRx, OnTx> — transport + multi-address
//
// Usage:
//
//   struct MyHandler1 : m::ModbusHandler<Regs1, MyHandler1> { … };
//   m::ModbusServer server{
//       data_link, time, {300_Us}, rx_buf, tx_buf,
//       std::move(h1)
//       // optional LED callbacks (zero overhead if omitted):
//       , [&]{ rx_led.toggle(); }, [&]{ tx_led.toggle(); }
//   };
// ============================================================================

template <m::ifc::CTime TimeUsT, typename OnRx = detail::NoOpCb,
          typename OnTx = detail::NoOpCb, typename... Handlers>
  requires m::ifc::CUs<typename TimeUsT::Unit> && (sizeof...(Handlers) >= 1)
class ModbusServer {
 public:
  using Error = ModbusRtuError;

  struct Timings {
    decltype(std::declval<TimeUsT&>().now()) tx_response_delay;
  };

  ModbusServer(m::ifc::IDataLink& data_link, TimeUsT& time, Timings timings,
               std::span<uint8_t> rx_buf, std::span<uint8_t> tx_buf,
               OnRx on_rx = {}, OnTx on_tx = {}, Handlers... handlers)
      : data_link_(data_link),
        time_(time),
        timings_(timings),
        rx_buf_(rx_buf),
        tx_buf_(tx_buf),
        on_rx_(std::move(on_rx)),
        on_tx_(std::move(on_tx)),
        handlers_(std::move(handlers)...) {}

  // ── Transport ──────────────────────────────────────────────────────────

  m::Task<bool> coroRun() {
    if (data_link_.error()) {
      if (!data_link_.stopReceive()) co_return false;
      if (!data_link_.stopTransmit()) co_return false;
    }
    if (running_) {
      if (data_link_.startReceive(rx_buf_))
        co_await m::coroYield();
      else
        co_return false;
    } else {
      co_return true;
    }

    auto packet = data_link_.getPacket();
    while (!packet) {
      packet = data_link_.getPacket();
      co_await m::coroYield();
    }

    tx_packet_size_ = process(packet.value(), tx_buf_);
    if (!tx_packet_size_) co_return true;

    on_rx_();
    co_await m::coroDelay(time_, timings_.tx_response_delay);

    if (auto size = tx_packet_size_.value(); size) {
      if (!data_link_.startTransmit(tx_buf_.first(size))) co_return false;
      on_tx_();
    }

    auto tx_done = data_link_.transmitDone();
    while (!tx_done) {
      tx_done = data_link_.transmitDone();
      co_await m::coroYield();
    }
    co_return tx_done.value();
  }

  bool start() { return running_ ? false : (running_ = true); }
  bool stop() { return !running_ ? false : (running_ = false); }

 private:
  m::ifc::IDataLink& data_link_;
  TimeUsT& time_;
  Timings timings_;
  std::span<uint8_t> rx_buf_;
  std::span<uint8_t> tx_buf_;
  [[no_unique_address]] OnRx on_rx_;
  [[no_unique_address]] OnTx on_tx_;
  std::tuple<Handlers...> handlers_;
  std::optional<uint32_t> tx_packet_size_;
  bool running_ = true;

  // ═══════════════════════════════════════════════════════════════════════
  // process() — CRC → address → find handler → dispatch → response
  // ═══════════════════════════════════════════════════════════════════════

  std::optional<uint32_t> process(std::span<uint8_t> rx_buf,
                                  std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 4 || rx_buf.size() > 256) return std::nullopt;

    const auto no_crc = rx_buf.first(rx_buf.size() - 2);
    if (crc16(no_crc) !=
        (static_cast<uint16_t>(rx_buf[rx_buf.size() - 2]) |
         (static_cast<uint16_t>(rx_buf[rx_buf.size() - 1]) << 8)))
      return std::nullopt;

    const uint8_t addr = rx_buf[0];
    const uint8_t cmd = rx_buf[1];
    const auto req = rx_buf.subspan(2, rx_buf.size() - 4);
    auto resp = tx_buf.subspan(2, tx_buf.size() - 4);

    // ── Broadcast (address 0) — apply writes to ALL handlers ──────────
    if (addr == 0) {
      std::apply([&](auto&... h) { ((h.broadcastWrite(cmd, req, resp)), ...); },
                 handlers_);
      return std::nullopt;
    }

    // ── Unicast — find handler by address ─────────────────────────────
    std::optional<Error> err;
    uint32_t resp_size = 0;
    bool found = false;

    std::apply(
        [&](auto&... h) {
          auto tryHandler = [&](auto& handler) {
            if (found) return;
            if (handler.getAddress() != addr) return;
            found = true;
            auto result = handler.dispatch(cmd, req, resp);
            if (!result) {
              err = result.error();
            } else {
              resp_size = *result;
            }
          };
          (tryHandler(h), ...);
        },
        handlers_);

    if (!found) return std::nullopt;

    // ── Build response ────────────────────────────────────────────────
    tx_buf[0] = addr;
    tx_buf[1] = cmd;
    uint32_t total = 2;
    if (err) {
      tx_buf[1] = static_cast<uint8_t>(cmd | 0x80U);
      tx_buf[2] = static_cast<uint8_t>(*err);
      total += 1;
    } else {
      total += resp_size;
    }
    auto crc = crc16(tx_buf.first(total));
    tx_buf[total] = static_cast<uint8_t>(crc);
    tx_buf[total + 1] = static_cast<uint8_t>(crc >> 8);
    return total + 2;
  }

  // ═══════════════════════════════════════════════════════════════════════
  // CRC16
  // ═══════════════════════════════════════════════════════════════════════

  static uint16_t crc16(std::span<const uint8_t> data) {
    static constexpr uint16_t table[2] = {0x00'00, 0xA0'01};
    uint16_t crc = 0xFF'FF;
    for (uint8_t byte : data) {
      crc ^= byte;
      for (uint8_t bit = 0; bit < 8U; ++bit) {
        const uint16_t xorv = crc & 0x01U;
        crc >>= 1U;
        crc ^= table[xorv];
      }
    }
    return crc;
  }
};

}  // namespace m

#endif  // MODBUS_SERVER_HPP
