/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_PROTOCOL_H
#define MODBUS_RTU_PROTOCOL_H

#include <DataLinkAsync.h>
#include <ITime.h>

#include <array>
#include <cstdio>
#include <functional>
#include <optional>
#include <span>
#include <tuple>

namespace m {

template <typename TimeUnit>
class ModbusRtuProtocol {
 public:
  using type = TimeUnit;

  enum class Commands : uint8_t {
    ReadCoils = 0,                  // 1
    ReadDiscreteInputs,             // 2
    ReadMultipleHoldingRegisters,   // 3
    ReadInputRegisters,             // 4
    WriteSingleCoil,                // 5
    WriteSingleHoldingRegister,     // 6
    WriteMultipleCoils,             // 15
    WriteMultipleHoldingRegisters,  // 16
    ReadServerId,                   // 17
    ReadFileRecord,                 // 20
    WriteFileRecord,                // 21
  };

  enum class Error : uint8_t {
    IllegalFunction = 1,
    IllegalDataAddress = 2,
    IllegalDataValue = 3,
    SlaveDeviceFailure = 4,
    Acknowledge = 5,
    SlaveDeviceBusy = 6,
    MemoryParityError = 8,
  };

  struct Timings {
    type tx_response_delay;
  };

  using RMHR_Cb =
      std::function<std::optional<m::ModbusRtuProtocol<type>::Error>(
          uint16_t start_addr, uint16_t regs_num, std::span<uint16_t> regs)>;

  ModbusRtuProtocol(m::ifc::IDataLink &data_link, m::ifc::ITime<type> &time,
                    Timings timings, std::span<uint8_t> rx_buf,
                    std::span<uint8_t> tx_buf, const RMHR_Cb &rmhr_cb)
      : data_link_(data_link),
        time_(time),
        timings_(timings),
        rx_buf_(rx_buf),
        tx_buf_(tx_buf),
        rmhr_cb_(rmhr_cb) {}

  bool handle() {
    if (data_link_.error()) {
      state_ = State::Idle;
      if (!data_link_.reset()) {
        return false;
      }
    }

    switch (state_) {
      case State::Idle: {
        if (running_) {
          if (data_link_.startReceive(rx_buf_)) {
            state_ = State::ProcessPacket;
            return true;
          } else {
            return false;
          }
        }
      } break;
      case State::ProcessPacket: {
        if (auto value = data_link_.getRxPacketSize(); value) {
          if (auto rx_packet_size = value.value()) {
            tx_packet_size_ = process(rx_buf_.first(rx_packet_size), tx_buf_);

            if (!tx_packet_size_) {
              state_ = State::Idle;
              return handle();
            }

            time_.delay(timings_.tx_response_delay);

            if (auto size = tx_packet_size_.value(); size) {
              if (!data_link_.startTransmit(tx_buf_.first(size))) {
                state_ = State::Idle;
                return false;
              }
            }
            state_ = State::TransmitResponse;
          }
        } else {
          return false;
        }
      } break;
      case State::TransmitResponse: {
        if (auto value = data_link_.transmitDone(); value) {
          if (value.value()) {
            state_ = State::Idle;
            return handle();
          } else {
          }
        }
      } break;
    }

    return true;
  }

  bool start() {
    if (running_) return false;
    running_ = true;
    return true;
  }

  bool stop() {
    if (!running_) return false;
    running_ = false;
    return true;
  }

  bool restart() {
    if (!data_link_.reset()) return false;
    state_ = State::Idle;
    running_ = true;
    return true;
  }

  bool setAddress(uint8_t addr) {
    addr_ = addr;
    return true;
  }

  uint8_t getAddress() { return addr_; }

 private:
  m::ifc::IDataLink &data_link_;
  m::ifc::ITime<type> &time_;
  Timings timings_;
  std::span<uint8_t> rx_buf_;
  std::span<uint8_t> tx_buf_;

  const RMHR_Cb &rmhr_cb_;

  uint8_t addr_;

  std::optional<uint32_t> tx_packet_size_;

  enum class State : uint8_t { Idle, ProcessPacket, TransmitResponse };
  State state_ = State::Idle;

  bool running_ = true;

#pragma pack(push, 1)
  struct AddrMem {
    uint32_t addr;
    std::array<uint8_t, 4> hash;
  };
#pragma pack(pop)
  static_assert(sizeof(AddrMem) == 8, "Wrong struct AddrMem sizeof");

  std::optional<uint32_t> process(std::span<uint8_t> rx_buf,
                                  std::span<uint8_t> tx_buf) {
    if (!addr_) return std::nullopt;
    if (rx_buf.size() < 4 || rx_buf.size() > 256) {
      return std::nullopt;
    }

    uint16_t lo = rx_buf.last(2)[0];
    uint16_t hi = rx_buf.last(2)[1];
    auto crc_origin = lo + (hi << 8);
    auto crc = crc16(rx_buf.first(rx_buf.size() - 2));

    if (crc != crc_origin) {
      return std::nullopt;
    }

    uint8_t addr = rx_buf[0];  // 0 - 247 valid
    if ((addr == addr_ || addr == 0) && addr < 248) {
      uint8_t cmd = rx_buf[1];

      tx_buf[0] = addr_;
      tx_buf[1] = cmd;
      uint32_t response_size = 2;

      auto cmd_id = getCmdId(cmd);

      if (cmd_id) {
        switch (cmd_id.value()) {
          case static_cast<uint8_t>(Commands::ReadCoils): {
          } break;
          case static_cast<uint8_t>(Commands::ReadDiscreteInputs): {
          } break;
          case static_cast<uint8_t>(Commands::ReadMultipleHoldingRegisters): {
            if (auto err = processReadMultipleHoldingRegisters(
                    rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            }
          } break;
          case static_cast<uint8_t>(Commands::ReadInputRegisters): {
          } break;
          case static_cast<uint8_t>(Commands::WriteSingleCoil): {
          } break;
          case static_cast<uint8_t>(Commands::WriteSingleHoldingRegister): {
          } break;
          case static_cast<uint8_t>(Commands::WriteMultipleCoils): {
          } break;
          case static_cast<uint8_t>(Commands::WriteMultipleHoldingRegisters): {
          } break;
          case static_cast<uint8_t>(Commands::ReadServerId): {
          } break;
          case static_cast<uint8_t>(Commands::ReadFileRecord): {
          } break;
          case static_cast<uint8_t>(Commands::WriteFileRecord): {
          } break;

          default: {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } break;
        }

        // if (cmd_handlers_.at(cmd_id.value())) {
        //   auto [ok, size] = cmd_handlers_.at(cmd_id.value())(
        //       rx_buf.subspan(2, rx_buf.size() - 4), tx_buf.subspan(2));

        //   response_size += size;
        //   if (!ok) tx_buf[1] += 0x80;
        // } else {
        //   tx_buf[1] += 0x80;
        //   tx_buf[2] = (uint8_t)Error::IllegalFunction;
        //   response_size += 1;
        // }
      } else {
        tx_buf[1] += 0x80;
        tx_buf[2] = (uint8_t)Error::IllegalFunction;
        response_size += 1;
      }

      auto crc = crc16(tx_buf.first(response_size));
      tx_buf[response_size] = crc;
      ++response_size;
      tx_buf[response_size] = crc >> 8;
      ++response_size;

      return response_size;
    }

    return std::nullopt;
  }

  std::optional<uint8_t> getCmdId(uint8_t cmd) {
    std::optional<uint8_t> temp;
    if (cmd >= 1 && cmd <= 6) {
      temp = cmd - 1;
    } else if (cmd >= 15 && cmd <= 17) {
      temp = cmd - 9;
    } else if (cmd >= 20 && cmd <= 21) {
      temp = cmd - 11;
    }
    return temp;
  };

  std::optional<m::ModbusRtuProtocol<type>::Error>
  processReadMultipleHoldingRegisters(std::span<uint8_t> rx_buf,
                                      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return m::ModbusRtuProtocol<type>::Error::IllegalDataValue;
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t regs_num = (rx_buf[2] << 8) + rx_buf[3];

    if (regs_num < 1 || regs_num > 0x00'7D) {
      return m::ModbusRtuProtocol<type>::Error::IllegalDataValue;
    }

    {
      uint32_t range = (int32_t)start_address + (int32_t)regs_num;
      if (range > 0xFF'FF) {
        return m::ModbusRtuProtocol<type>::Error::IllegalDataAddress;
      }
    }

    tx_buf[0] = regs_num * 2;
    tx_buf = tx_buf.subspan(1);

    std::span<uint16_t> regs = std::span<uint16_t>{
        reinterpret_cast<uint16_t *>(tx_buf.data()), tx_buf.size() / 2};

    if (auto err = rmhr_cb_(start_address, regs_num, regs); err) {
      return err;
    } else {
      swapBytesInSpan(regs);
      return std::nullopt;
    }

    return std::nullopt;
  }

  void swapBytesInSpan(std::span<uint16_t> regs) {
    for (uint16_t &reg : regs) {
      reg = byteswap(reg);
    }
  }

  constexpr uint16_t byteswap(uint16_t value) {
    return (value >> 8) | (value << 8);
  }

  uint16_t crc16(std::span<uint8_t> data) {
    static const uint16_t table[2] = {0x0000, 0xA001};
    volatile uint16_t crc = 0xFFFF;
    volatile uint16_t xorv = 0;

    for (auto i = 0u; i < data.size(); ++i) {
      crc ^= data[i];

      for (char bit = 0; bit < 8; bit++) {
        xorv = crc & 0x01;
        crc >>= 1;
        crc ^= table[xorv];
      }
    }

    return crc;
  }
};
}  // namespace m

#endif