/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef JEDEC_SPI_FLASH_HPP
#define JEDEC_SPI_FLASH_HPP

#include <Bps.hpp>
#include <CoroYield.hpp>
#include <IFlashMemory.hpp>
#include <IIO_Async.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace m::ic {

template <typename IoT, typename TimeT, typename CsPinT,
          std::size_t Default_Total_Size_Bytes_V = 16U * 1024U * 1024U,
          std::size_t Erase_Block_Bytes_V = 4U * 1024U,
          std::size_t Write_Block_Bytes_V = 256U,
          std::size_t Address_Bytes_V = 3U, uint8_t Erase_Command_V = 0x20>
  requires m::ifc::CIO_Async<IoT> && m::ifc::CBps<typename IoT::Unit> &&
           m::ifc::CTimeOf<TimeT, typename TimeT::Unit> &&
           m::ifc::CMs<typename TimeT::Unit> && m::ifc::mcu::CPin<CsPinT>
class JedecSpiFlash : public m::ifc::IFlashMemory {
 public:
  static_assert(Default_Total_Size_Bytes_V > 0U,
                "Default flash size must be > 0");
  static_assert(Erase_Block_Bytes_V > 0U, "Erase block size must be > 0");
  static_assert(Write_Block_Bytes_V > 0U, "Write block size must be > 0");
  static_assert((Erase_Block_Bytes_V % Write_Block_Bytes_V) == 0U,
                "Erase block must be divisible by write block");
  static_assert((Address_Bytes_V >= 3U) && (Address_Bytes_V <= 4U),
                "Address bytes must be 3 or 4");

  static constexpr std::size_t Default_Total_Size_Bytes =
      Default_Total_Size_Bytes_V;
  static constexpr std::size_t Erase_Block_Bytes = Erase_Block_Bytes_V;
  static constexpr std::size_t Write_Block_Bytes = Write_Block_Bytes_V;
  static constexpr std::size_t Address_Bytes = Address_Bytes_V;

  JedecSpiFlash(IoT& io, CsPinT& cs_pin, TimeT& time)
      : io_(io), cs_pin_(cs_pin), time_(time) {}

  std::size_t size() override { return total_size_bytes_; }

  std::size_t eraseBlockSize() override { return Erase_Block_Bytes; }

  std::size_t writeBlockSize() override { return Write_Block_Bytes; }

  m::Task<bool> read(std::size_t addr, std::span<uint8_t> data) override {
    if (data.empty()) {
      co_return true;
    }
    if (!isRangeValid(addr, data.size())) {
      co_return false;
    }

    auto cmd = makeAddrCommand(Command::ReadData, addr);

    csSelect();
    const bool cmd_ok = co_await tx(cmd);
    const bool data_ok = cmd_ok && (co_await rx(data));
    csDeselect();

    co_return data_ok;
  }

  m::Task<bool> eraseBlock(std::size_t addr) override {
    if ((addr & (Erase_Block_Bytes - 1U)) != 0U) {
      co_return false;
    }
    if (!isRangeValid(addr, Erase_Block_Bytes)) {
      co_return false;
    }

    if (!(co_await setWriteEnable(true))) {
      co_return false;
    }

    auto cmd = makeAddrCommand(static_cast<Command>(Erase_Command_V), addr);

    csSelect();
    const bool cmd_ok = co_await tx(cmd);
    csDeselect();

    if (!cmd_ok) {
      co_return false;
    }

    co_return co_await waitWhileBusy(TimeUnitT{350});
  }

  m::Task<bool> writeBlock(std::size_t addr,
                           std::span<uint8_t const> data) override {
    if (data.size() != Write_Block_Bytes) {
      co_return false;
    }
    if (!isRangeValid(addr, data.size())) {
      co_return false;
    }

    const std::size_t page_offset = addr & (Write_Block_Bytes - 1U);
    if ((page_offset + data.size()) > Write_Block_Bytes) {
      co_return false;
    }

    if (!(co_await setWriteEnable(true))) {
      co_return false;
    }

    auto cmd = makeAddrCommand(Command::PageProgram, addr);

    csSelect();
    const bool cmd_ok = co_await tx(cmd);
    const bool data_ok = cmd_ok && (co_await tx(data));
    csDeselect();

    if (!data_ok) {
      co_return false;
    }

    co_return co_await waitWhileBusy(TimeUnitT{20});
  }

  m::Task<std::optional<std::array<uint8_t, 3>>> readJedecId() {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Command::ReadJedecId)};
    std::array<uint8_t, 3> jedec{};

    csSelect();
    const bool cmd_ok = co_await tx(cmd);
    const bool id_ok = cmd_ok && (co_await rx(jedec));
    csDeselect();

    if (!id_ok) {
      co_return std::nullopt;
    }

    co_return jedec;
  }

  m::Task<bool> probeSizeByJedec() {
    const auto jedec_opt = co_await readJedecId();
    if (!jedec_opt.has_value()) {
      co_return false;
    }

    const uint8_t cap = jedec_opt.value()[2];
    if ((cap < 8U) || (cap > 31U)) {
      co_return false;
    }

    total_size_bytes_ = static_cast<std::size_t>(1UL) << cap;
    co_return true;
  }

 private:
  using TimeUnitT = typename TimeT::Unit;

  enum class Command : uint8_t {
    PageProgram = 0x02,
    ReadData = 0x03,
    WriteDisable = 0x04,
    ReadStatus1 = 0x05,
    WriteEnable = 0x06,
    Erase4k = 0x20,
    ReadJedecId = 0x9F,
  };

  static constexpr uint8_t Busy_Mask = 0x01U;

  IoT& io_;
  CsPinT& cs_pin_;
  TimeT& time_;
  std::size_t total_size_bytes_{Default_Total_Size_Bytes};

  bool isRangeValid(std::size_t addr, std::size_t size_bytes) const {
    if (size_bytes == 0U) {
      return true;
    }
    if (addr >= total_size_bytes_) {
      return false;
    }

    return size_bytes <= (total_size_bytes_ - addr);
  }

  TimeUnitT transferTimeout(std::size_t bytes) {
    const auto baud = io_.getBaudrate().value();
    if (baud == 0U) {
      return TimeUnitT{100};
    }

    const uint64_t payload_bytes = static_cast<uint64_t>(bytes);
    const uint64_t ms =
        (payload_bytes * 1'000ULL + baud - 1ULL) / static_cast<uint64_t>(baud);
    return TimeUnitT{static_cast<typename TimeUnitT::type>(ms + 2ULL)};
  }

  std::array<uint8_t, 1U + Address_Bytes> makeAddrCommand(Command cmd,
                                                          std::size_t addr) {
    std::array<uint8_t, 1U + Address_Bytes> buf{};
    buf[0] = static_cast<uint8_t>(cmd);

    if constexpr (Address_Bytes == 3U) {
      buf[1] = static_cast<uint8_t>((addr >> 16U) & 0xFFU);
      buf[2] = static_cast<uint8_t>((addr >> 8U) & 0xFFU);
      buf[3] = static_cast<uint8_t>(addr & 0xFFU);
    } else {
      buf[1] = static_cast<uint8_t>((addr >> 24U) & 0xFFU);
      buf[2] = static_cast<uint8_t>((addr >> 16U) & 0xFFU);
      buf[3] = static_cast<uint8_t>((addr >> 8U) & 0xFFU);
      buf[4] = static_cast<uint8_t>(addr & 0xFFU);
    }

    return buf;
  }

  void csSelect() { cs_pin_.write(true); }

  void csDeselect() { cs_pin_.write(false); }

  m::Task<bool> tx(std::span<uint8_t const> data) {
    if (data.empty()) {
      co_return true;
    }

    if (!io_.startWrite(data)) {
      co_return false;
    }

    const auto start = time_.now();
    const auto timeout = transferTimeout(data.size());
    while (!io_.isWriteDone()) {
      if (time_.diff(start) > timeout) {
        io_.abortWrite();
        co_return false;
      }
      co_await m::coroYield();
    }

    co_return !io_.error();
  }

  m::Task<bool> rx(std::span<uint8_t> data) {
    if (data.empty()) {
      co_return true;
    }

    if (!io_.startRead(data)) {
      co_return false;
    }

    const auto start = time_.now();
    const auto timeout = transferTimeout(data.size()) + TimeUnitT{5};
    while (!io_.isReadDone()) {
      if (time_.diff(start) > timeout) {
        io_.abortRead();
        co_return false;
      }
      co_await m::coroYield();
    }

    co_return !io_.error();
  }

  m::Task<bool> setWriteEnable(bool enable) {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(
        enable ? Command::WriteEnable : Command::WriteDisable)};

    csSelect();
    const bool ok = co_await tx(cmd);
    csDeselect();

    co_return ok;
  }

  m::Task<std::optional<uint8_t>> readStatus1() {
    std::array<uint8_t, 1> cmd{static_cast<uint8_t>(Command::ReadStatus1)};
    std::array<uint8_t, 1> resp{};

    csSelect();
    const bool ok = (co_await tx(cmd)) && (co_await rx(resp));
    csDeselect();

    if (!ok) {
      co_return std::nullopt;
    }

    co_return resp[0];
  }

  m::Task<bool> waitWhileBusy(TimeUnitT timeout) {
    const auto start = time_.now();
    while (true) {
      const auto status_opt = co_await readStatus1();
      if (!status_opt.has_value()) {
        co_return false;
      }

      if ((status_opt.value() & Busy_Mask) == 0U) {
        co_return true;
      }

      if (time_.diff(start) > timeout) {
        co_return false;
      }

      co_await m::coroYield();
    }
  }
};

template <typename IoT, typename TimeT, typename CsPinT>
  requires m::ifc::CIO_Async<IoT> && m::ifc::CBps<typename IoT::Unit> &&
           m::ifc::CTimeOf<TimeT, typename TimeT::Unit> &&
           m::ifc::CMs<typename TimeT::Unit> && m::ifc::mcu::CPin<CsPinT>
JedecSpiFlash(IoT& io, CsPinT& cs_pin, TimeT& time)
    -> JedecSpiFlash<IoT, TimeT, CsPinT>;

}  // namespace m::ic

#endif  // JEDEC_SPI_FLASH_HPP
