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

#include <IIO_Async.hpp>
#include <ITime.hpp>
#include <Timeout.hpp>
#include <TupleContains.hpp>
#include <Us.hpp>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>

namespace m {

template <typename T, typename Reg>
concept CIcSync = requires(T t, Reg reg) {
  { t.template getWriteBuf<Reg>(reg) } -> std::same_as<std::span<uint8_t>>;
  { t.template getReadBuf<Reg>() } -> std::same_as<std::span<volatile uint8_t>>;
  { t.template getReg<Reg>() } -> std::same_as<Reg>;
};

template <typename Derived, m::ifc::CUs TimeUnit, m::ifc::CTime<TimeUnit> Time,
          m::ifc::CIO_Async Io, typename IcInfo>
class IcSync {
 public:
  IcSync(Time& time, Io& io, TimeUnit add_timeout)
      : time_(time), io_(io), add_timeout_(add_timeout) {}

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  bool write(Reg reg) {
    static_assert(CIcSync<Derived, Reg>,
                  "Derived must implement CIcSync interface");
    auto span = static_cast<Derived*>(this)->template getWriteBuf<Reg>(reg);

    if (!io_.writeAsync(span)) return false;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.writeDone(); },
            span.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return false;
    }

    return true;
  }

  template <typename Reg>
    requires m::tuple_contains<Reg, typename IcInfo::Regs>
  std::optional<Reg> read() {
    static_assert(CIcSync<Derived, Reg>,
                  "Derived must implement CIcSync interface");
    auto span = static_cast<Derived*>(this)->template getReadBuf<Reg>();

    if (!io_.readAsync(span)) return std::nullopt;

    if (!timeout_.execWithTimeout(
            [&]() { return io_.readDone(); },
            span.size() * TimeUnit{1'000} / io_.getBaudrate().value() +
                add_timeout_)) {
      return std::nullopt;
    }

    Reg reg = static_cast<Derived*>(this)->template getReg<Reg>();
    return reg;
  }

 private:
  Time& time_;
  Io& io_;
  TimeUnit add_timeout_;

  m::Timeout<TimeUnit> timeout_{time_};
};

}  // namespace m

#endif  // IC_SYNC_HPP