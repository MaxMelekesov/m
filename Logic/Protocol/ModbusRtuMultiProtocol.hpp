/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_MULTI_PROTOCOL_HPP
#define MODBUS_RTU_MULTI_PROTOCOL_HPP

#include <DataLinkAsync.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Us.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <tuple>

namespace m {

template <m::ifc::CUs UsT, m::ifc::mcu::CPin PintT, std::size_t AddrCount = 1>
class ModbusRtuMultiProtocol {
 public:
  enum class Commands : uint8_t {
    ReadCoils = 1,
    ReadDiscreteInputs = 2,
    ReadMultipleHoldingRegisters = 3,
    ReadInputRegisters = 4,
    WriteSingleCoil = 5,
    WriteSingleHoldingRegister = 6,
    WriteMultipleCoils = 15,
    WriteMultipleHoldingRegisters = 16,
    ReadServerId = 17,
    ReadFileRecord = 20,
    WriteFileRecord = 21,
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
    UsT tx_response_delay;
  };

  // ReadCoils callback
  using RC_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t coils_num, std::span<uint8_t> coils)>;

  // ReadDiscreteInputs callback
  using RDI_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t inputs_num, std::span<uint8_t> inputs)>;

  // ReadMultipleHoldingRegisters callback
  using RMHR_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t regs_num, std::span<uint8_t> regs)>;

  // ReadInputRegisters callback
  using RIR_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t regs_num, std::span<uint8_t> regs)>;

  // WriteSingleCoil callback
  using WSC_Cb = std::function<std::optional<Error>(uint16_t addr, bool value)>;

  // WriteSingleHoldingRegister callback
  using WSHR_Cb =
      std::function<std::optional<Error>(uint16_t addr, uint16_t value)>;

  // WriteMultipleCoils callback
  using WMC_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t coils_num, std::span<uint8_t> coils)>;

  // WriteMultipleHoldingRegisters callback
  using WMHR_Cb = std::function<std::optional<Error>(
      uint16_t start_addr, uint16_t regs_num, std::span<uint8_t> regs)>;

  ModbusRtuMultiProtocol(m::ifc::IDataLink& data_link, m::ifc::ITime<UsT>& time,
                         Timings timings, std::span<uint8_t> rx_buf,
                         std::span<uint8_t> tx_buf, PintT& rx_led,
                         PintT& tx_led)
      : data_link_(data_link),
        time_(time),
        timings_(timings),
        rx_buf_(rx_buf),
        tx_buf_(tx_buf),
        rx_led_(rx_led),
        tx_led_(tx_led) {
    addr_.fill(0);
  }

  void addReadCoilsCallback(std::size_t index, RC_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].rc_cb = std::move(cb);
  }
  void addReadDiscreteInputsCallback(std::size_t index, RDI_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].rdi_cb = std::move(cb);
  }
  void addReadMultipleHoldingRegistersCallback(std::size_t index,
                                               RMHR_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].rmhr_cb = std::move(cb);
  }
  void addReadInputRegistersCallback(std::size_t index, RIR_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].rir_cb = std::move(cb);
  }
  void addWriteSingleCoilCallback(std::size_t index, WSC_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].wsc_cb = std::move(cb);
  }
  void addWriteSingleHoldingRegisterCallback(std::size_t index, WSHR_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].wshr_cb = std::move(cb);
  }
  void addWriteMultipleCoilsCallback(std::size_t index, WMC_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].wmc_cb = std::move(cb);
  }
  void addWriteMultipleHoldingRegistersCallback(std::size_t index,
                                                WMHR_Cb&& cb) {
    if (index >= AddrCount) return;
    cb_[index].wmhr_cb = std::move(cb);
  }

  bool handle() {
    if (data_link_.error()) {
      state_ = State::Idle;
      if (!data_link_.stopReceive()) {
        return false;
      }
      if (!data_link_.stopTransmit()) {
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
        if (auto value = data_link_.getPacket(); value) {
          tx_packet_size_ = process(value.value(), tx_buf_);
          rx_led_.toggle();
          if (!tx_packet_size_) {
            state_ = State::Idle;
            return handle();
          }

          // TODO: switch delay to non blocking timer
          time_.delay(timings_.tx_response_delay);

          if (auto size = tx_packet_size_.value(); size) {
            if (!data_link_.startTransmit(tx_buf_.first(size))) {
              state_ = State::Idle;
              return false;
            }
            tx_led_.toggle();
          }
          state_ = State::TransmitResponse;

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
    if (!data_link_.stopReceive()) return false;
    if (!data_link_.stopTransmit()) return false;
    state_ = State::Idle;
    running_ = true;
    return true;
  }

  void setAddress(std::array<uint8_t, AddrCount> addr) { addr_ = addr; }

  std::array<uint8_t, AddrCount> getAddress() { return addr_; }

 private:
  m::ifc::IDataLink& data_link_;
  m::ifc::ITime<UsT>& time_;
  Timings timings_;
  std::span<uint8_t> rx_buf_;
  std::span<uint8_t> tx_buf_;
  PintT& rx_led_;
  PintT& tx_led_;

  struct Callbacks {
    RC_Cb rc_cb;
    RDI_Cb rdi_cb;
    RMHR_Cb rmhr_cb;
    RIR_Cb rir_cb;
    WSC_Cb wsc_cb;
    WSHR_Cb wshr_cb;
    WMC_Cb wmc_cb;
    WMHR_Cb wmhr_cb;
  };
  std::array<Callbacks, AddrCount> cb_;

  std::array<uint8_t, AddrCount> addr_;

  std::optional<uint32_t> tx_packet_size_;

  enum class State : uint8_t { Idle, ProcessPacket, TransmitResponse };
  State state_ = State::Idle;

  bool running_ = true;

  std::optional<uint32_t> process(std::span<uint8_t> rx_buf,
                                  std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 4 || rx_buf.size() > 256) {
      return std::nullopt;
    }

    {
      uint16_t lo = rx_buf.last(2)[0];
      uint16_t hi = rx_buf.last(2)[1];
      auto crc_origin = lo + (hi << 8);
      auto crc = crc16(rx_buf.first(rx_buf.size() - 2));

      if (crc != crc_origin) {
        return std::nullopt;
      }
    }

    uint8_t addr = rx_buf[0];  // 0 - 247 valid
    if (addr == 0) addr = addr_[0];

    std::size_t addr_index = 0;
    if (auto it = std::find(addr_.begin(), addr_.end(), addr);
        it != addr_.end()) {
      addr_index = std::distance(addr_.begin(), it);

      uint8_t cmd = rx_buf[1];

      tx_buf[0] = addr;
      tx_buf[1] = cmd;
      uint32_t response_size = 2;

      switch (static_cast<Commands>(cmd)) {
        case Commands::ReadCoils: {
          if (!cb_[addr_index].rc_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto [err, size] = processReadCoils(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += size;
            }
          }
        } break;
        case Commands::ReadDiscreteInputs: {
          if (!cb_[addr_index].rdi_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto [err, size] = processReadDiscreteInputs(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += size;
            }
          }
        } break;
        case Commands::ReadMultipleHoldingRegisters: {
          if (!cb_[addr_index].rmhr_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto [err, size] = processReadMultipleHoldingRegisters(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += size;
            }
          }
        } break;
        case Commands::ReadInputRegisters: {
          if (!cb_[addr_index].rir_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto [err, size] = processReadInputRegisters(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += size;
            }
          }
        } break;
        case Commands::WriteSingleCoil: {
          if (!cb_[addr_index].wsc_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto err = processWriteSingleCoil(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += 4;
            }
          }
        } break;
        case Commands::WriteSingleHoldingRegister: {
          if (!cb_[addr_index].wshr_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto err = processWriteSingleHoldingRegister(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += 4;
            }
          }
        } break;
        case Commands::WriteMultipleCoils: {
          if (!cb_[addr_index].wmc_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto err = processWriteMultipleCoils(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += 4;
            }
          }
        } break;
        case Commands::WriteMultipleHoldingRegisters: {
          if (!cb_[addr_index].wmhr_cb) {
            tx_buf[1] += 0x80;
            tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
            response_size += 1;
          } else {
            if (auto err = processWriteMultipleHoldingRegisters(
                    addr_index, rx_buf.subspan(2, rx_buf.size() - 4),
                    tx_buf.subspan(2, tx_buf.size() - 4));
                err) {
              tx_buf[1] += 0x80;
              tx_buf[2] = static_cast<uint8_t>(err.value());
              response_size += 1;
            } else {
              response_size += 4;
            }
          }
        } break;

        default: {
          tx_buf[1] += 0x80;
          tx_buf[2] = static_cast<uint8_t>(Error::IllegalFunction);
          response_size += 1;
        } break;
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

  std::tuple<std::optional<Error>, uint32_t> processReadCoils(
      std::size_t addr_index, std::span<uint8_t> rx_buf,
      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t coils_num = (rx_buf[2] << 8) + rx_buf[3];

    if (coils_num < 1 || coils_num > 0x07'D0) {
      return {Error::IllegalDataValue, 0};
    }

    {
      uint32_t range = (int32_t)start_address + (int32_t)coils_num;
      if (range > 0xFF'FF) {
        return {Error::IllegalDataAddress, 0};
      }
    }

    uint32_t byte_count = (coils_num + 7) / 8;
    if (byte_count + 1 > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = byte_count;
    tx_buf = tx_buf.subspan(1);

    std::span<uint8_t> coils = tx_buf.first(byte_count);

    if (auto err = cb_[addr_index].rc_cb(start_address, coils_num, coils);
        err) {
      return {err, 0};
    } else {
      return {std::nullopt, byte_count + 1};
    }
  }

  std::tuple<std::optional<Error>, uint32_t> processReadDiscreteInputs(
      std::size_t addr_index, std::span<uint8_t> rx_buf,
      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t inputs_num = (rx_buf[2] << 8) + rx_buf[3];

    if (inputs_num < 1 || inputs_num > 0x07'D0) {
      return {Error::IllegalDataValue, 0};
    }

    {
      uint32_t range = (int32_t)start_address + (int32_t)inputs_num;
      if (range > 0xFF'FF) {
        return {Error::IllegalDataAddress, 0};
      }
    }

    uint32_t byte_count = (inputs_num + 7) / 8;
    if (byte_count + 1 > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = byte_count;
    tx_buf = tx_buf.subspan(1);

    std::span<uint8_t> inputs = tx_buf.first(byte_count);

    if (auto err = cb_[addr_index].rdi_cb(start_address, inputs_num, inputs);
        err) {
      return {err, 0};
    } else {
      return {std::nullopt, byte_count + 1};
    }
  }

  std::tuple<std::optional<Error>, uint32_t>
  processReadMultipleHoldingRegisters(std::size_t addr_index,
                                      std::span<uint8_t> rx_buf,
                                      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t regs_num = (rx_buf[2] << 8) + rx_buf[3];

    if (regs_num < 1 || regs_num > 0x00'7D) {
      return {Error::IllegalDataValue, 0};
    }

    {
      uint32_t range = (int32_t)start_address + (int32_t)regs_num;
      if (range > 0xFF'FF) {
        return {Error::IllegalDataAddress, 0};
      }
    }

    uint32_t byte_count = regs_num * 2;
    if (byte_count + 1 > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = regs_num * 2;
    tx_buf = tx_buf.subspan(1, 2 * regs_num);

    if (auto err = cb_[addr_index].rmhr_cb(start_address, regs_num, tx_buf);
        err) {
      return {err, 0};
    } else {
      swapBytesInSpan(tx_buf);
      return {std::nullopt, byte_count + 1};
    }
  }

  std::tuple<std::optional<Error>, uint32_t> processReadInputRegisters(
      std::size_t addr_index, std::span<uint8_t> rx_buf,
      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t regs_num = (rx_buf[2] << 8) + rx_buf[3];

    if (regs_num < 1 || regs_num > 0x00'7D) {
      return {Error::IllegalDataValue, 0};
    }

    {
      uint32_t range = (int32_t)start_address + (int32_t)regs_num;
      if (range > 0xFF'FF) {
        return {Error::IllegalDataAddress, 0};
      }
    }

    uint32_t byte_count = regs_num * 2;
    if (byte_count + 1 > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = regs_num * 2;
    tx_buf = tx_buf.subspan(1, 2 * regs_num);

    if (auto err = cb_[addr_index].rir_cb(start_address, regs_num, tx_buf);
        err) {
      return {err, 0};
    } else {
      swapBytesInSpan(tx_buf);
      return {std::nullopt, byte_count + 1};
    }
  }

  std::optional<Error> processWriteSingleCoil(std::size_t addr_index,
                                              std::span<uint8_t> rx_buf,
                                              std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return Error::IllegalDataValue;
    }

    uint16_t addr = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t value = (rx_buf[2] << 8) + rx_buf[3];

    if (auto err = cb_[addr_index].wsc_cb(addr, value); err) {
      return err;
    } else {
      std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
      return std::nullopt;
    }
  }

  std::optional<Error> processWriteSingleHoldingRegister(
      std::size_t addr_index, std::span<uint8_t> rx_buf,
      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return Error::IllegalDataValue;
    }

    uint16_t addr = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t value = (rx_buf[2] << 8) + rx_buf[3];

    if (auto err = cb_[addr_index].wshr_cb(addr, value); err) {
      return err;
    } else {
      std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
      return std::nullopt;
    }
  }

  std::optional<Error> processWriteMultipleCoils(std::size_t addr_index,
                                                 std::span<uint8_t> rx_buf,
                                                 std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) {
      return Error::IllegalDataValue;
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t coils_num = (rx_buf[2] << 8) + rx_buf[3];
    uint8_t byte_count = rx_buf[4];

    if (coils_num < 1 || coils_num > 0x07'B0 ||
        byte_count != (coils_num + 7) / 8) {
      return Error::IllegalDataValue;
    }

    if (rx_buf.size() != byte_count + 5u) {
      return Error::IllegalDataValue;
    }

    std::span<uint8_t> coils = rx_buf.subspan(5, byte_count);

    if (auto err = cb_[addr_index].wmc_cb(start_address, coils_num, coils);
        err) {
      return err;
    } else {
      std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
      return std::nullopt;
    }
  }

  std::optional<Error> processWriteMultipleHoldingRegisters(
      std::size_t addr_index, std::span<uint8_t> rx_buf,
      std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) {
      return Error::IllegalDataValue;
    }

    uint16_t start_address = (rx_buf[0] << 8) + rx_buf[1];
    uint16_t regs_num = (rx_buf[2] << 8) + rx_buf[3];
    uint8_t byte_count = rx_buf[4];

    if (regs_num < 1 || regs_num > 0x00'7B || byte_count != regs_num * 2) {
      return Error::IllegalDataValue;
    }

    if (rx_buf.size() != byte_count + 5u) {
      return Error::IllegalDataValue;
    }

    std::span<uint8_t> regs = rx_buf.subspan(5, regs_num * 2);

    if (auto err = cb_[addr_index].wmhr_cb(start_address, regs_num, regs);
        err) {
      return err;
    } else {
      std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
      return std::nullopt;
    }
  }

  void swapBytesInSpan(std::span<uint8_t> regs) {
    for (size_t i = 0; i + 1 < regs.size(); i += 2) {
      std::swap(regs[i], regs[i + 1]);
    }
  }

  uint16_t crc16(std::span<uint8_t> data) {
    static const uint16_t table[2] = {0x00'00, 0xA0'01};
    uint16_t crc = 0xFF'FF;
    uint16_t xorv = 0;

    for (auto i = 0u; i < data.size(); ++i) {
      crc ^= data[i];

      for (uint8_t bit = 0; bit < 8; bit++) {
        xorv = crc & 0x01;
        crc >>= 1;
        crc ^= table[xorv];
      }
    }

    return crc;
  }
};
}  // namespace m

#endif  // MODBUS_RTU_MULTI_PROTOCOL_HPP