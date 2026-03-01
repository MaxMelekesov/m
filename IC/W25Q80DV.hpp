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
#include <Timeout.hpp>
#include <Us.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ic {

class W25Q80DV : public m::ifc::IFlashMemory {
 public:
  static constexpr uint8_t Jedec_Manufacturer_Winbond = 0xEF;
  static constexpr uint8_t Jedec_Manufacturer_Puya = 0x85;
  static constexpr uint8_t Jedec_MemoryType_Winbond = 0x40;
  static constexpr uint8_t Jedec_Capacity_1M = 0x14;
  static constexpr uint8_t Jedec_Capacity_16M = 0x18;

  static constexpr std::size_t Total_Size_Bytes = 16U * 1024U * 1024U;
  static constexpr std::size_t Sector_Size_Bytes = 4U * 1024U;
  static constexpr std::size_t Page_Size_Bytes = 256U;

  W25Q80DV(m::ifc::IIO_Async<Bps<uint32_t>>& spi, m::ifc::mcu::IPin& cs_pin,
           m::ifc::ITime<Us<uint32_t>>& time)
      : spi_(spi), cs_pin_(cs_pin), time_(time) {}

  std::size_t size() override { return total_size_bytes_; }

  bool erase(std::size_t addr, std::size_t erase_size) override {
    if (erase_size == 0U) {
      return true;
    }
    if (!isRangeValid(addr, erase_size)) {
      return false;
    }

    std::size_t start = alignDown(addr, Sector_Size_Bytes);
    std::size_t end = alignUp(addr + erase_size, Sector_Size_Bytes);

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
    if (!isRangeValid(addr, data.size())) {
      return false;
    }

    std::size_t src_offset = 0U;
    while (src_offset < data.size()) {
      std::size_t write_addr = addr + src_offset;
      std::size_t sector_base = alignDown(write_addr, Sector_Size_Bytes);
      std::size_t sector_offset = write_addr - sector_base;
      std::size_t chunk_size =
          std::min(Sector_Size_Bytes - sector_offset, data.size() - src_offset);

      if (!readRaw(sector_base, std::span<uint8_t>{sector_buf_})) {
        return false;
      }

      std::copy_n(data.data() + src_offset, chunk_size,
                  sector_buf_.data() + sector_offset);

      if (!eraseSector4k(sector_base)) {
        return false;
      }
      if (!programSector4k(sector_base, std::span<uint8_t const>{sector_buf_})) {
        return false;
      }

      src_offset += chunk_size;
    }

    return true;
  }

  bool read(std::size_t addr, std::span<uint8_t> data) override {
    if (data.empty()) {
      return true;
    }
    if (!isRangeValid(addr, data.size())) {
      return false;
    }

    return readRaw(addr, data);
  }

  bool probe() {
    std::array<uint8_t, 3> jedec{};
    if (!readJedecId(jedec)) {
      return false;
    }

    if ((jedec[0] == Jedec_Manufacturer_Puya) &&
        (jedec[2] == Jedec_Capacity_16M)) {
      total_size_bytes_ = 16U * 1024U * 1024U;
      return true;
    }

    if ((jedec[0] == Jedec_Manufacturer_Winbond) &&
        (jedec[1] == Jedec_MemoryType_Winbond) &&
        (jedec[2] == Jedec_Capacity_1M)) {
      total_size_bytes_ = 1U * 1024U * 1024U;
      return true;
    }

    return false;
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
    ReadJedecId = 0x9F,
  };

  static constexpr uint8_t Busy_Mask = 0x01;
  static constexpr Us<uint32_t> Page_Program_Timeout{20'000};
  static constexpr Us<uint32_t> Erase_4k_Timeout{500'000};

  m::ifc::IIO_Async<Bps<uint32_t>>& spi_;
  m::ifc::mcu::IPin& cs_pin_;
  m::ifc::ITime<Us<uint32_t>>& time_;
  std::size_t total_size_bytes_ = Total_Size_Bytes;
  std::array<uint8_t, Sector_Size_Bytes> sector_buf_{};

  static std::size_t alignDown(std::size_t value, std::size_t align) {
    return value & ~(align - 1U);
  }

  static std::size_t alignUp(std::size_t value, std::size_t align) {
    return (value + align - 1U) & ~(align - 1U);
  }

  bool isRangeValid(std::size_t addr, std::size_t size_bytes) const {
    if (size_bytes == 0U) {
      return true;
    }
    if (addr >= total_size_bytes_) {
      return false;
    }
    return size_bytes <= (total_size_bytes_ - addr);
  }

  Us<uint32_t> transferTimeout(std::size_t bytes) {
    auto baud = spi_.getBaudrate().value();
    if (baud == 0U) {
      return Us<uint32_t>{20'000};
    }

    uint64_t bits = static_cast<uint64_t>(bytes) * 8ULL;
    uint64_t us = (bits * 1'000'000ULL) / static_cast<uint64_t>(baud);
    return Us<uint32_t>{static_cast<uint32_t>(us + 2'000ULL)};
  }

  static std::array<uint8_t, 4> makeAddrCommand(Command cmd, std::size_t addr) {
    return {
        static_cast<uint8_t>(cmd),
        static_cast<uint8_t>((addr >> 16U) & 0xFFU),
        static_cast<uint8_t>((addr >> 8U) & 0xFFU),
        static_cast<uint8_t>(addr & 0xFFU),
    };
  }

  void csSelect() { cs_pin_.write(true); }

  void csDeselect() { cs_pin_.write(false); }

  bool tx(std::span<uint8_t const> data) {
    if (data.empty()) {
      return true;
    }

    if (!spi_.writeAsync(data)) {
      return false;
    }

    auto done = m::execWithTimeout(
        time_, [&]() { return spi_.writeDone(); }, transferTimeout(data.size()));
    if (!done) {
      spi_.abortWrite();
      return false;
    }

    return !spi_.error();
  }

  bool rx(std::span<uint8_t> data) {
    if (data.empty()) {
      return true;
    }

    if (!spi_.readAsync(data)) {
      return false;
    }

    auto done = m::execWithTimeout(
        time_, [&]() { return spi_.readDone(); },
        transferTimeout(data.size()) + Us<uint32_t>{5'000});
    if (!done) {
      spi_.abortRead();
      return false;
    }

    return !spi_.error();
  }

  bool readJedecId(std::span<uint8_t, 3> jedec) {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Command::ReadJedecId)};

    csSelect();
    bool ok = tx(cmd) && rx(jedec);
    csDeselect();

    return ok;
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
    bool ready = m::execWithTimeout(
        time_,
        [&]() {
          if (!io_ok) {
            return true;
          }

          uint8_t status = 0U;
          if (!readStatus1(status)) {
            io_ok = false;
            return true;
          }

          return (status & Busy_Mask) == 0U;
        },
        timeout);

    return ready && io_ok;
  }

  bool readRaw(std::size_t addr, std::span<uint8_t> data) {
    auto cmd = makeAddrCommand(Command::ReadData, addr);

    csSelect();
    bool ok = tx(cmd) && rx(data);
    csDeselect();

    return ok;
  }

  bool eraseSector4k(std::size_t addr) {
    if ((addr & (Sector_Size_Bytes - 1U)) != 0U) {
      return false;
    }
    if (!isRangeValid(addr, Sector_Size_Bytes)) {
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

    return waitWhileBusy(Erase_4k_Timeout);
  }

  bool programPage(std::size_t addr, std::span<uint8_t const> data) {
    if (data.empty() || data.size() > Page_Size_Bytes) {
      return false;
    }

    std::size_t page_offset = addr & (Page_Size_Bytes - 1U);
    if ((page_offset + data.size()) > Page_Size_Bytes) {
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

    return waitWhileBusy(Page_Program_Timeout);
  }

  bool programSector4k(std::size_t addr, std::span<uint8_t const> data) {
    if ((addr & (Sector_Size_Bytes - 1U)) != 0U) {
      return false;
    }
    if (data.size() != Sector_Size_Bytes) {
      return false;
    }

    for (std::size_t page_idx = 0U; page_idx < (Sector_Size_Bytes / Page_Size_Bytes);
         ++page_idx) {
      std::size_t page_addr = addr + (page_idx * Page_Size_Bytes);
      auto page_data = data.subspan(page_idx * Page_Size_Bytes, Page_Size_Bytes);
      if (!programPage(page_addr, page_data)) {
        return false;
      }
    }

    return true;
  }
};

static_assert(m::ifc::CFlashMemory<W25Q80DV>,
              "W25Q80DV must satisfy CFlashMemory concept");

}  // namespace m::ic

#endif  // W25Q80DV_HPP