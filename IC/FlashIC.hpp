/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FLASHIC_HPP
#define FLASHIC_HPP

#include <IFlashMemory.hpp>
#include <IIO_Sync.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Timeout.hpp>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m {

template <std::size_t Sectors, std::size_t SectorSize, std::size_t PageSize, m::ifc::CMs>
class FlashIC : public m::ifc::IFlashMemory {
 public:
  FlashIC(m::ifc::IIO_Sync<Ms<type>>& spi, m::ifc::mcu::IPin& cs_pin,
          m::ifc::ITime<Ms<type>>& time)
      : spi_(spi), cs_pin_(cs_pin), time_(time), timeout_(time_) {}

  std::size_t size() override { return Sectors * SectorSize; }

  bool erase(std::size_t addr, uint32_t size) override { return false; }
  bool write(std::size_t addr, std::span<uint8_t const> data) override {
    return false;
  }
  bool read(std::size_t addr, std::span<uint8_t> data) override {
    return false;
  }

 private:
  m::ifc::IIO_Sync<Ms<type>>& spi_;
  m::ifc::mcu::IPin& cs_pin_;
  m::ifc::ITime<Ms<type>>& time_;
  m::Timeout<Ms<type>> timeout_;

  enum class Commands : uint8_t {
    Page_Prog = 0x02,
    Read = 0x03,
    Write_Disable = 0x04,
    Read_Status = 0x05,
    Write_Enable = 0x06,
    Erase_Sector = 0x20,

  };

  bool setWriteMode(bool value) {
    std::array<uint8_t, 1> cmd;
    if (value) {
      cmd[0] = static_cast<uint8_t>(Commands::Write_Enable);
    } else {
      cmd[0] = static_cast<uint8_t>(Commands::Write_Disable);
    }

    cs_pin_.write(1);
    auto wr_ok =
        spi_.write(cmd, Ms<type>{cmd.size() * 1'000 / spi_.getBaudrate() + 10});
    cs_pin_.write(0);

    return wr_ok;
  }
};

static_assert(m::ifc::CFlashMemory<FlashIC<16, 4096, 256>>,
              "FlashIC must satisfy CFlashMemory concept");
}  // namespace m

#endif  // FLASHIC_HPP