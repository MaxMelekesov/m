/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef AD5663_DAC_HPP
#define AD5663_DAC_HPP

#include <AD5663.hpp>
#include <IDac.hpp>
#include <concepts>
#include <cstdint>

namespace m::ic {

/// Anything that can write the AD5663 "write and update" registers —
/// normally m::ic::Ad5663Ic<Time, Io>.
template <typename T>
concept CAd5663Writer =
    requires(T ic, Ad5663::WriteAndUpdateDacA a, Ad5663::WriteAndUpdateDacB b) {
      { ic.write(a) } -> std::same_as<bool>;
      { ic.write(b) } -> std::same_as<bool>;
    };

/**
 * One AD5663 output (DAC A or DAC B) as a m::ifc::IDac.
 *
 * The chip driver is passed by reference, so both channels of one chip are
 * driven through the same Ad5663Ic object (and the same SYNC pin).
 *
 * The chip is 16-bit, so the code is used as is — no scaling. setValue()
 * issues "write and update" (command 011), i.e. the output changes with that
 * frame.
 *
 * Every call is a 24-bit SPI frame and blocks until the DMA transfer is done
 * (m::execWithTimeout), so mind the caller's deadline: at ~5 MBit/s one frame
 * is about 5 us.
 *
 * @code
 *   using Ic = m::ic::Ad5663Ic<TimeUs, Spi>;
 *   Ic chip{time, bus, sync1, add_timeout};    // one object per chip
 *
 *   using Dac = m::ic::Ad5663Dac<Ic>;
 *   Dac a{chip, Dac::Channel::A};
 *   Dac b{chip, Dac::Channel::B};
 *
 *   a.start();
 *   a.setValue(0x8000);            // output A mid-scale
 * @endcode
 */
template <CAd5663Writer IcT>
class Ad5663Dac final : public m::ifc::IDac {
 public:
  enum class Channel : uint8_t { A = 0, B = 1 };

  Ad5663Dac(IcT& ic, Channel channel) : ic_(ic), channel_(channel) {}

  /// Nothing to power up — the chip is always ready to accept codes.
  bool start() override { return true; }
  bool running() override { return true; }

  /// No power-down command in the driver: not supported.
  bool stop() override { return false; }

  bool setValue(uint32_t value) override {
    if (channel_ == Channel::A) {
      Ad5663::WriteAndUpdateDacA reg;
      reg.value.setRaw(value);
      return ic_.write(reg);
    }
    Ad5663::WriteAndUpdateDacB reg;
    reg.value.setRaw(value);
    return ic_.write(reg);
  }

 private:
  IcT& ic_;
  const Channel channel_;
};

}  // namespace m::ic

#endif  // AD5663_DAC_HPP
