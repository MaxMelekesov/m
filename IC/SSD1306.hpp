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

#include <IIO_Sync.hpp>

namespace m::ic {

template <typename TimeUnit>
class SSD1306 {
 public:
  using type = TimeUnit;

  SSD1306(m::ifc::IIO_Sync<Ms<type>> &io) : io_(io) {
    onOff(0);
    setClockDiv(0x80);
    setMux(31);
    setOffset(0);
    setStartLine(0);
    setAddresssingMode(0);
    setSegmentRemap(1);
    setScanDirection(0);
    setComPinsHardware(0, 0);

    setChargePump(1);
    onOff(1);

    clear();

    //    setStartColumn(0);
    //    setStartPage(0);

    //    setColumnRange(0, 22);
    //    setPageRange(0, 2);
    //
    //    while (1) {
    //      std::array<uint8_t, 1> t;
    //      t.fill(0xFF);
    //      sendData(t);
    //      HAL_Delay(100);
    //    }
  }

  ~SSD1306() {
    setChargePump(0);
    onOff(0);
  }

  bool setClockDiv(uint8_t value) {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0xD5;
    cmd[1] = value;

    return sendCmd(cmd);
  }

  bool setMux(uint8_t mux) {
    std::array<uint8_t, 2> cmd;

    if (mux < 15) mux = 15;
    if (mux > 63) mux = 63;

    cmd[0] = 0xA8;
    cmd[1] = mux;

    return sendCmd(cmd);
  }

  bool setOffset(uint8_t offset) {
    std::array<uint8_t, 2> cmd;

    if (offset > 63) offset = 63;

    cmd[0] = 0xD3;
    cmd[1] = offset;

    return sendCmd(cmd);
  }

  bool setStartLine(uint8_t value) {
    std::array<uint8_t, 1> cmd;

    if (value > 63) value = 63;

    cmd[0] = 0x40 | value;

    return sendCmd(cmd);
  }

  bool setSegmentRemap(bool value) {
    std::array<uint8_t, 1> cmd;

    cmd[0] = (value) ? 0xA1 : 0xA0;

    return sendCmd(cmd);
  }

  bool setScanDirection(bool value) {
    std::array<uint8_t, 1> cmd;

    cmd[0] = (value) ? 0xC8 : 0xC0;

    return sendCmd(cmd);
  }

  bool setColumnRange(uint8_t start, uint8_t end) {
    std::array<uint8_t, 3> cmd;

    cmd[0] = 0x21;
    cmd[1] = start;
    cmd[2] = end;

    return sendCmd(cmd);
  }

  bool setStartColumn(uint8_t start) {
    std::array<uint8_t, 2> cmd;

    cmd[0] = start & 0x0F;
    cmd[1] = (start >> 4) | 0x10;

    return sendCmd(cmd);
  }

  bool setStartPage(uint8_t start) {
    std::array<uint8_t, 1> cmd;

    cmd[0] = 0xB0 | (start & 0x07);

    return sendCmd(cmd);
  }

  bool setPageRange(uint8_t start, uint8_t end) {
    std::array<uint8_t, 3> cmd;

    cmd[0] = 0x22;
    cmd[1] = start;
    cmd[2] = end;

    return sendCmd(cmd);
  }

  bool setAddresssingMode(uint8_t value) {
    std::array<uint8_t, 2> cmd;

    if (value > 2) value = 2;

    cmd[0] = 0x20;
    cmd[1] = value;

    return sendCmd(cmd);
  }

  bool setChargePump(bool value) {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0x8D;
    cmd[1] = (value) ? 0x14 : 0x10;

    return sendCmd(cmd);
  }

  bool onOff(bool onOff) {
    std::array<uint8_t, 1> cmd;
    if (onOff)
      cmd[0] = 0xAF;
    else
      cmd[0] = 0xAE;

    return sendCmd(cmd);
  }

  bool setComPinsHardware(bool a5, bool a4) {
    std::array<uint8_t, 2> cmd;

    cmd[0] = 0xDA;
    cmd[1] = 0x02;
    if (a4) cmd[1] |= 1 << 4;
    if (a5) cmd[1] |= 1 << 5;

    return sendCmd(cmd);
  }

  void clear() {
    setColumnRange(0, 127);
    setPageRange(0, 3);
    const uint8_t part_size = 64;

    std::array<uint8_t, part_size> t;
    t.fill(0);
    for (auto i = 0; i < 512 / part_size; ++i) {
      sendData(t);
    }

    //    std::array<uint8_t, 128> t;
    //    t.fill(0x00);
    //
    //    for (auto i = 0; i < 8; ++i) {
    //      setStartColumn(0);
    //      setStartPage(i);
    //      sendData(t);
    //    }
  }

  bool draw(std::span<const uint8_t> data, uint8_t col_start, uint8_t col_end,
            uint8_t page_start, uint8_t page_end) {
    setColumnRange(col_start, col_end);
    setPageRange(page_start, page_end);

    const uint8_t part_size = 64;

    if (data.size() >= 64) {
      for (auto i = 0u; i < data.size() / part_size; ++i) {
        if (!sendData(data.subspan(i * part_size, part_size))) return false;
      }
    }

    if (data.size() % 64 != 0) {
      if (!sendData(
              data.subspan((data.size() / part_size) * 64, data.size() % 64)))
        return false;
    }

    return true;
  }

 private:
  // const uint8_t  addr_ = 0x3C << 1;
  m::ifc::IIO_Sync<Ms<uint32_t>> &io_;

  bool sendCmd(std::span<uint8_t> cmd) {
    std::array<uint8_t, 8> packet;
    if (cmd.size() > packet.size() - 1) return false;

    const uint8_t cmd_byte = 0;
    packet[0] = cmd_byte;
    std::copy(begin(cmd), end(cmd), begin(packet) + 1);

    std::span<uint8_t> packet_trimmed =
        std::span<uint8_t>{packet}.subspan(0, cmd.size() + 1);

    auto res = io_.write(
        packet_trimmed,
        Ms<type>{packet_trimmed.size() * 1'000 / io_.getBaudrate() + 1});

    return res;
  }

  bool sendData(std::span<const uint8_t> data) {
    std::array<uint8_t, 65> packet;
    if (data.size() > packet.size() - 1) return false;

    const uint8_t data_byte = 0x40;
    packet[0] = data_byte;
    std::copy(begin(data), end(data), begin(packet) + 1);

    std::span<uint8_t> packet_trimmed =
        std::span<uint8_t>{packet}.subspan(0, data.size() + 1);

    auto res = io_.write(
        packet_trimmed,
        Ms<type>{packet_trimmed.size() * 1'000 / io_.getBaudrate() + 1});

    return res;
  }
};

}  // namespace m::ic

#endif  // SSD1306_H
