/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SSD1306_H
#define SSD1306_H

#include <CoroScheduler.hpp>
#include <CoroUntil.hpp>
#include <IIO_Async.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace m::ic {

template <m::ifc::CIO_Async IoT>
class SSD1306 {
 public:
  explicit SSD1306(IoT& io) : io_(io) {}

  [[nodiscard]] auto init() -> m::Task<bool> {
    if (!(co_await onOff(false))) co_return false;
    if (!(co_await setClockDiv(0x80))) co_return false;
    if (!(co_await setMux(31))) co_return false;
    if (!(co_await setOffset(0))) co_return false;
    if (!(co_await setStartLine(0))) co_return false;
    if (!(co_await setAddresssingMode(0))) co_return false;
    if (!(co_await setSegmentRemap(true))) co_return false;
    if (!(co_await setScanDirection(true))) co_return false;
    if (!(co_await setComPinsHardware(false, false))) co_return false;
    if (!(co_await setChargePump(true))) co_return false;
    if (!(co_await onOff(true))) co_return false;
    if (!(co_await clear())) co_return false;

    co_return true;
  }

  [[nodiscard]] auto deinit() -> m::Task<bool> {
    if (!(co_await setChargePump(false))) co_return false;
    co_return co_await onOff(false);
  }

  [[nodiscard]] auto setClockDiv(uint8_t value) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0xD5;
    cmd[1] = value;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setMux(uint8_t mux) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    if (mux < 15) mux = 15;
    if (mux > 63) mux = 63;

    cmd[0] = 0xA8;
    cmd[1] = mux;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setOffset(uint8_t offset) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    if (offset > 63) offset = 63;

    cmd[0] = 0xD3;
    cmd[1] = offset;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setStartLine(uint8_t value) -> m::Task<bool> {
    std::array<uint8_t, 1> cmd;

    if (value > 63) value = 63;

    cmd[0] = 0x40 | value;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setSegmentRemap(bool value) -> m::Task<bool> {
    std::array<uint8_t, 1> cmd;

    cmd[0] = (value) ? 0xA1 : 0xA0;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setScanDirection(bool value) -> m::Task<bool> {
    std::array<uint8_t, 1> cmd;

    cmd[0] = (value) ? 0xC8 : 0xC0;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setColumnRange(uint8_t start, uint8_t end)
      -> m::Task<bool> {
    std::array<uint8_t, 3> cmd;

    cmd[0] = 0x21;
    cmd[1] = start;
    cmd[2] = end;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setStartColumn(uint8_t start) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    cmd[0] = start & 0x0F;
    cmd[1] = (start >> 4) | 0x10;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setStartPage(uint8_t start) -> m::Task<bool> {
    std::array<uint8_t, 1> cmd;

    cmd[0] = 0xB0 | (start & 0x07);

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setPageRange(uint8_t start, uint8_t end) -> m::Task<bool> {
    std::array<uint8_t, 3> cmd;

    cmd[0] = 0x22;
    cmd[1] = start;
    cmd[2] = end;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setAddresssingMode(uint8_t value) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    if (value > 2) value = 2;

    cmd[0] = 0x20;
    cmd[1] = value;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setChargePump(bool value) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0x8D;
    cmd[1] = (value) ? 0x14 : 0x10;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto onOff(bool on_off) -> m::Task<bool> {
    std::array<uint8_t, 1> cmd;
    if (on_off)
      cmd[0] = 0xAF;
    else
      cmd[0] = 0xAE;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto setComPinsHardware(bool a5, bool a4) -> m::Task<bool> {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0xDA;
    cmd[1] = 0x02;
    if (a4) cmd[1] |= 1 << 4;
    if (a5) cmd[1] |= 1 << 5;

    co_return co_await sendCmd(cmd);
  }

  [[nodiscard]] auto clear() -> m::Task<bool> {
    if (!(co_await setColumnRange(0, 127))) co_return false;
    if (!(co_await setPageRange(0, 3))) co_return false;
    const uint8_t part_size = 64;

    std::array<uint8_t, part_size> t;
    t.fill(static_cast<uint8_t>(0));
    for (auto i = 0; i < 512 / part_size; ++i) {
      if (!(co_await sendData(t))) co_return false;
    }

    co_return true;
  }

  [[nodiscard]] auto draw(std::span<const uint8_t> data, uint8_t col_start,
                          uint8_t col_end, uint8_t page_start, uint8_t page_end)
      -> m::Task<bool> {
    if (!(co_await setColumnRange(col_start, col_end))) co_return false;
    if (!(co_await setPageRange(page_start, page_end))) co_return false;

    const uint8_t part_size = 64;

    if (data.size() >= 64) {
      for (auto i = 0u; i < data.size() / part_size; ++i) {
        if (!(co_await sendData(data.subspan(i * part_size, part_size)))) {
          co_return false;
        }
      }
    }

    if (data.size() % 64 != 0) {
      if (!(co_await sendData(data.subspan((data.size() / part_size) * 64,
                                           data.size() % 64)))) {
        co_return false;
      }
    }

    co_return true;
  }

 private:
  static constexpr uint8_t addr_ = 0x3C;
  IoT& io_;

  [[nodiscard]] auto sendCmd(std::span<const uint8_t> cmd) -> m::Task<bool> {
    std::array<uint8_t, 10> packet;
    if (cmd.size() > packet.size() - 2) co_return false;

    packet[0] = addr_;
    packet[1] = 0x00;
    std::copy(cmd.begin(), cmd.end(), packet.begin() + 2);

    auto packet_trimmed =
        std::span<const uint8_t>{packet}.first(cmd.size() + 2);

    co_return co_await sendPacket(packet_trimmed);
  }

  [[nodiscard]] auto sendData(std::span<const uint8_t> data) -> m::Task<bool> {
    std::array<uint8_t, 66> packet;
    if (data.size() > packet.size() - 2) co_return false;

    packet[0] = addr_;
    packet[1] = 0x40;
    std::copy(data.begin(), data.end(), packet.begin() + 2);

    auto packet_trimmed =
        std::span<const uint8_t>{packet}.first(data.size() + 2);

    co_return co_await sendPacket(packet_trimmed);
  }

  [[nodiscard]] auto sendPacket(std::span<const uint8_t> packet)
      -> m::Task<bool> {
    if (!io_.writeAsync(packet)) {
      co_return false;
    }

    co_await m::coroUntil([this] { return io_.writeDone(); });

    if (io_.error()) {
      co_return false;
    }

    co_return true;
  }
};

template <m::ifc::CIO_Async IoT>
SSD1306(IoT& io) -> SSD1306<IoT>;

}  // namespace m::ic

#endif  // SSD1306_H
