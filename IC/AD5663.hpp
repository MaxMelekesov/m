/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef AD5663_HPP
#define AD5663_HPP

#include <Bps.hpp>
#include <IIO_Async.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ic.hpp>
#include <Reg.hpp>
#include <StaticMap.hpp>
#include <Timeout.hpp>
#include <Us.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ic {

struct Ad5663 {
  struct WriteInputDacA {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct WriteInputDacB {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct UpdateDacA {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct UpdateDacB {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct WriteInputUpdateAllDacA {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct WriteInputUpdateAllDacB {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct WriteAndUpdateDacA {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };
  struct WriteAndUpdateDacB {
    struct Code : public m::BitField<Code, 16> {};

    m::Reg<uint16_t, Code> value;
  };

  using Regs =
      std::tuple<WriteInputDacA, WriteInputDacB, UpdateDacA, UpdateDacB,
                 WriteInputUpdateAllDacA, WriteInputUpdateAllDacB,
                 WriteAndUpdateDacA, WriteAndUpdateDacB>;

  struct Map
      : public m::StaticMap<
            uint8_t, m::Pair<WriteInputDacA, 0x00>,
            m::Pair<WriteInputDacB, 0x01>, m::Pair<UpdateDacA, 0x08>,
            m::Pair<UpdateDacB, 0x09>, m::Pair<WriteInputUpdateAllDacA, 0x10>,
            m::Pair<WriteInputUpdateAllDacB, 0x11>,
            m::Pair<WriteAndUpdateDacA, 0x18>,
            m::Pair<WriteAndUpdateDacB, 0x19>> {};
};

/// AD5663 — synchronous SPI driver (register-style via m::ic::Ic).
///
/// Write-only chip: the only bus access is through the CRTP base Ic —
/// `dac.write(reg)` for a register type from `Ad5663::Regs`; the frame's
/// address byte ((command << 3) | DAC address) comes from `Ad5663::Map`.
/// The AD5663 has no readback, so there is no readImpl.
///
/// Multiple chips may share one SPI bus: each instance owns its SYNC (CS)
/// pin; SCLK/MOSI — and LDAC/CLR, when used — are common to the bus.
///
/// @code
///   Spi bus;                       // m::ifc::CIO_Async (DMA), shared
///   m::ifc::mcu::IPin& sync1;      // per-chip SYNC
///
///   Ad5663Ic dac1{time, bus, sync1, add_timeout};   // chip #1 (A, B)
///
///   // A: write and update in one frame (command 011)
///   Ad5663::WriteAndUpdateDacA a;
///   a.value.setRaw(0x8000);
///   dac1.write(a);
///
///   // A and B synchronously via registers (command 010 update-all):
///   Ad5663::WriteInputDacB b;
///   b.value.setRaw(0x4000);
///   dac1.write(b);                 // B -> input register (output unchanged)
///   Ad5663::WriteInputUpdateAllDacA all;
///   all.value.setRaw(0x8000);
///   dac1.write(all);               // input A + update of both DACs at once
/// @endcode
template <m::ifc::CTime Time, m::ifc::CIO_Async Io>
  requires m::ifc::CUs<typename Time::Unit> && m::ifc::CBps<typename Io::Unit>
class Ad5663Ic : public Ic<Ad5663Ic<Time, Io>, Ad5663> {
 public:
  using TimeUnit = typename Time::Unit;

  Ad5663Ic(Time& time, Io& io, m::ifc::mcu::IPin& cs, TimeUnit add_timeout)
      : time_(time), io_(io), cs_(cs), add_timeout_(add_timeout) {}

 private:
  Time& time_;
  Io& io_;
  m::ifc::mcu::IPin& cs_;
  TimeUnit add_timeout_;

  std::array<uint8_t, 3> write_buf_;

  template <typename Reg>
  bool writeImpl(Reg reg) {
    write_buf_[0] = Ad5663::Map::template value<Reg>();
    const uint16_t raw = reg.value.getRaw();
    write_buf_[1] = static_cast<uint8_t>((raw >> 8U) & 0xFFU);
    write_buf_[2] = static_cast<uint8_t>(raw & 0xFFU);

    auto span = std::span<const uint8_t>(write_buf_).first(3);

    cs_.write(false);
    const bool ok = writeSpan(span);
    cs_.write(true);
    return ok;
  }

  bool writeSpan(std::span<const uint8_t> span) {
    if (!io_.startWrite(span)) {
      return false;
    }

    if (!m::execWithTimeout(
            time_, [&]() { return io_.isWriteDone(); },
            transferTimeout(span.size()) + add_timeout_)) {
      return false;
    }

    return true;
  }

  TimeUnit transferTimeout(std::size_t bytes) {
    using CalcT = std::common_type_t<std::size_t, typename Io::Unit::type,
                                     typename TimeUnit::type>;
    constexpr CalcT Us_In_Second = 1'000'000;

    const auto baud = static_cast<CalcT>(io_.getBaudrate().value());
    if (baud == 0) {
      return TimeUnit{0};
    }

    const auto bytes_calc = static_cast<CalcT>(bytes);
    const CalcT tx_time = (bytes_calc * Us_In_Second + baud - 1) / baud;
    return TimeUnit{static_cast<typename TimeUnit::type>(tx_time)};
  }

  friend class Ic<Ad5663Ic<Time, Io>, Ad5663>;
};

template <m::ifc::CTime Time, m::ifc::CIO_Async Io>
  requires m::ifc::CUs<typename Time::Unit> && m::ifc::CBps<typename Io::Unit>
Ad5663Ic(Time& time, Io& io, m::ifc::mcu::IPin& cs,
         typename Time::Unit add_timeout) -> Ad5663Ic<Time, Io>;

}  // namespace m::ic

#endif  // AD5663_HPP
