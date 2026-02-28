/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef W25Q80DV_HPP
#define W25Q80DV_HPP

#include <Bps.hpp>
#include <IFlashMemory.hpp>
#include <IIO_Async.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Us.hpp>
#include <Timeout.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ic {

class W25Q80DV : public m::ifc::IFlashMemory {
 public:
  static constexpr std::size_t Total_Size_Bytes = 1 * 1024 * 1024;
  static constexpr std::size_t Sector_Size_Bytes = 4 * 1024;
  static constexpr std::size_t Page_Size_Bytes = 256;

  W25Q80DV(m::ifc::IIO_Async<Bps<uint32_t>>& spi, m::ifc::mcu::IPin& cs_pin,
       m::ifc::ITime<Us<uint32_t>>& time)
      : spi_(spi), cs_pin_(cs_pin), time_(time) {}

  std::size_t size() override { return Total_Size_Bytes; }

  bool erase(std::size_t addr, std::size_t erase_size) override {
    if (erase_size == 0) {
      return true;
    }
    if (!isAddressValid(addr, erase_size)) {
      return false;
    }

    std::size_t start = addr & ~(Sector_Size_Bytes - 1);
    std::size_t end =
        (addr + erase_size + Sector_Size_Bytes - 1) & ~(Sector_Size_Bytes - 1);

    for (std::size_t sector_addr = start; sector_addr < end;
         sector_addr += Sector_Size_Bytes) {
      if (!eraseSector4k(sector_addr)) {
        return false;
      }
    }

    return true;
  }

  bool write(std::size_t addr, std::span<uint8_t const> data) override {
    if (data.empty()) {
      return true;
    }
    if (!isAddressValid(addr, data.size())) {
      return false;
    }

    std::size_t write_addr = addr;
    auto write_data = data;

    if ((write_addr & (Sector_Size_Bytes - 1)) != 0) {
      auto sector_base = write_addr & ~(Sector_Size_Bytes - 1);
      auto sector_offset = write_addr & (Sector_Size_Bytes - 1);
      auto first_chunk =
          std::min(Sector_Size_Bytes - sector_offset, write_data.size());

      if (!readBytes(sector_base, std::span<uint8_t>{sector_buf_})) {
        return false;
      }

      std::copy_n(write_data.data(), first_chunk,
                  sector_buf_.data() + sector_offset);

      if (!eraseSector4k(sector_base) ||
          !writeSector4k(sector_base, std::span<uint8_t const>{sector_buf_})) {
        return false;
      }

      write_addr += first_chunk;
      write_data = write_data.subspan(first_chunk);
    }

    while (write_data.size() >= Sector_Size_Bytes) {
      if (!eraseSector4k(write_addr) ||
          !writeSector4k(write_addr, write_data.first(Sector_Size_Bytes))) {
        return false;
      }

      write_addr += Sector_Size_Bytes;
      write_data = write_data.subspan(Sector_Size_Bytes);
    }

    if (!write_data.empty()) {
      auto sector_base = write_addr & ~(Sector_Size_Bytes - 1);
      auto sector_offset = write_addr & (Sector_Size_Bytes - 1);

      if (!readBytes(sector_base, std::span<uint8_t>{sector_buf_})) {
        return false;
      }

      std::copy_n(write_data.data(), write_data.size(),
                  sector_buf_.data() + sector_offset);

      if (!eraseSector4k(sector_base) ||
          !writeSector4k(sector_base, std::span<uint8_t const>{sector_buf_})) {
        return false;
      }
    }

    return true;
  }

  bool read(std::size_t addr, std::span<uint8_t> data) override {
    if (data.empty()) {
      return true;
    }
    if (!isAddressValid(addr, data.size())) {
      return false;
    }

    return readBytes(addr, data);
  }

  bool probe() {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Command::ReadJedecId)};
    std::array<uint8_t, 3> jedec{};

    csSelect();
    bool ok = tx(cmd) && rx(jedec);
    csDeselect();

    if (!ok) {
      return false;
    }

    return jedec[0] == 0xEF && jedec[1] == 0x40 && jedec[2] == 0x14;
  }

  bool waitReady(Us<uint32_t> timeout) { return waitWhileBusy(timeout); }

 private:
  enum class Command : uint8_t {
    PageProgram = 0x02,
    ReadData = 0x03,
    WriteDisable = 0x04,
    ReadStatus1 = 0x05,
    WriteEnable = 0x06,
    Erase4k = 0x20,
    Erase32k = 0x52,
    ChipErase60 = 0x60,
    ChipEraseC7 = 0xC7,
    Erase64k = 0xD8,
    ReadJedecId = 0x9F,
  };

  m::ifc::IIO_Async<Bps<uint32_t>>& spi_;
  m::ifc::mcu::IPin& cs_pin_;
  m::ifc::ITime<Us<uint32_t>>& time_;
  std::array<uint8_t, Sector_Size_Bytes> sector_buf_{};

  static constexpr uint8_t Busy_Mask = 0x01;

  static std::array<uint8_t, 4> makeAddrCommand(Command cmd, std::size_t addr) {
    return {
        static_cast<uint8_t>(cmd),
        static_cast<uint8_t>((addr >> 16) & 0xFF),
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>(addr & 0xFF),
    };
  }

  bool isAddressValid(std::size_t addr, std::size_t data_size) const {
    if (data_size == 0) {
      return true;
    }
    if (addr >= Total_Size_Bytes) {
      return false;
    }
    return data_size <= (Total_Size_Bytes - addr);
  }

  Us<uint32_t> transferTimeout(std::size_t bytes) {
    auto baud = spi_.getBaudrate().value();
    if (baud == 0) {
      return Us<uint32_t>{20'000};
    }

    auto transfer_us = static_cast<uint32_t>((bytes * 1'000'000) / baud);
    return Us<uint32_t>{transfer_us + 5'000};
  }

  void csSelect() { cs_pin_.write(1); }

  void csDeselect() { cs_pin_.write(0); }

  bool tx(std::span<uint8_t const> data) {
    if (!spi_.writeAsync(data)) {
      return false;
    }

    auto done = m::execWithTimeout(
        time_, [&]() { return spi_.writeDone(); },
        transferTimeout(data.size()));
    if (!done) {
      spi_.abortWrite();
      return false;
    }

    return !spi_.error();
  }

  bool rx(std::span<uint8_t> data) {
    if (!spi_.readAsync(data)) {
      return false;
    }

    auto done = m::execWithTimeout(
        time_, [&]() { return spi_.readDone(); },
      transferTimeout(data.size()) + Us<uint32_t>{100'000});
    if (!done) {
      spi_.abortRead();
      return false;
    }

    return !spi_.error();
  }

  bool setWriteEnable(bool enable) {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(
        enable ? Command::WriteEnable : Command::WriteDisable)};

    csSelect();
    bool ok = tx(cmd);
    csDeselect();

    return ok;
  }

  bool readStatus1(uint8_t& status) {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Command::ReadStatus1)};
    std::array<uint8_t, 1> resp{};

    csSelect();
    bool ok = tx(cmd) && rx(resp);
    csDeselect();

    if (!ok) {
      return false;
    }

    status = resp[0];
    return true;
  }

  bool waitWhileBusy(Us<uint32_t> timeout) {
    bool io_ok = true;
    auto ready = m::execWithTimeout(
        time_,
        [&]() {
          if (!io_ok) {
            return true;
          }

          uint8_t status = 0;
          if (!readStatus1(status)) {
            io_ok = false;
            return true;
          }

          return (status & Busy_Mask) == 0;
        },
        timeout);

    if (!ready) {
      return false;
    }

    if (!io_ok) {
      return false;
    }

    return true;
  }

  bool readBytes(std::size_t addr, std::span<uint8_t> data) {
    auto cmd = makeAddrCommand(Command::ReadData, addr);

    csSelect();
    bool ok = tx(cmd) && rx(data);
    csDeselect();

    return ok;
  }

  bool programPage(std::size_t addr, std::span<uint8_t const> data) {
    if (data.empty() || data.size() > Page_Size_Bytes) {
      return false;
    }
    auto page_off = addr & (Page_Size_Bytes - 1);
    if ((page_off + data.size()) > Page_Size_Bytes) {
      return false;
    }

    if (!setWriteEnable(true)) {
      return false;
    }

    auto cmd = makeAddrCommand(Command::PageProgram, addr);

    csSelect();
    bool ok = tx(cmd) && tx(data);
    csDeselect();

    if (!ok) {
      return false;
    }

    return waitWhileBusy(Us<uint32_t>{10'000});
  }

  bool writeSector4k(std::size_t addr, std::span<uint8_t const> data) {
    if ((addr & (Sector_Size_Bytes - 1)) != 0 ||
        data.size() != Sector_Size_Bytes) {
      return false;
    }

    for (std::size_t page = 0; page < (Sector_Size_Bytes / Page_Size_Bytes);
         ++page) {
      auto page_addr = addr + page * Page_Size_Bytes;
      auto page_data = data.subspan(page * Page_Size_Bytes, Page_Size_Bytes);
      if (!programPage(page_addr, page_data)) {
        return false;
      }
    }

    return true;
  }

  bool eraseSector4k(std::size_t addr) {
    if ((addr & (Sector_Size_Bytes - 1)) != 0 ||
        !isAddressValid(addr, Sector_Size_Bytes)) {
      return false;
    }

    if (!setWriteEnable(true)) {
      return false;
    }

    auto cmd = makeAddrCommand(Command::Erase4k, addr);

    csSelect();
    bool ok = tx(cmd);
    csDeselect();

    if (!ok) {
      return false;
    }

    return waitWhileBusy(Us<uint32_t>{500'000});
  }
};

static_assert(m::ifc::CFlashMemory<W25Q80DV>,
              "W25Q80DV must satisfy CFlashMemory concept");

}  // namespace m::ic

#endif  // W25Q80DV_HPP