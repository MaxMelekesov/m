/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_MASTER_HPP
#define MODBUS_RTU_MASTER_HPP

#include <IIO_Async.hpp>
#include <ITime.hpp>
#include <TSerDes.hpp>
#include <Timer.hpp>
#include <cstdint>
#include <optional>

namespace m {

template <m::ifc::CIO_Async IoT, m::ifc::CTimeUs TimeUsT>
class ModbusRtuMaster {
 public:
  using UsType = decltype(std::declval<TimeUsT>().now());

  struct Timings {
    UsType rx_delay;
    UsType response_delay;
  };

  ModbusRtuMaster(IoT& io, TimeUsT& time, Timings timings)
      : io_(io),
        time_(time),
        timings_(timings),
        tx_timer_(time),
        start_rx_timer_(time),
        rx_timer_(time) {}

#pragma pack(push, 1)
  struct Unit {
   public:
    Unit(uint8_t addr, uint16_t start_reg, uint16_t count)
        : addr_(addr), start_reg_(start_reg), reg_count_(count) {}

    Unit() {}

   private:
    uint8_t addr_{0u};
    uint8_t cmd_{0u};
    uint16_t start_reg_{0u};
    uint16_t reg_count_{0u};
    uint16_t crc_{0u};

    friend class ModbusRtuMaster;
  };
#pragma pack(pop)
  static_assert(sizeof(Unit) == 8, "Wrong sizeof(ModbusRtuMaster::Unit)");

  uint16_t readMhrResponseSize(Unit& unit) { return unit.reg_count_ * 2u + 5u; }

  bool readMhr(Unit& unit, std::span<uint8_t> response_buf) {
    if (state_ != State::Idle) {
      return false;
    }
    if (response_buf.size() != readMhrResponseSize(unit)) {
      return false;
    }

    unit_ = unit;
    response_buf_ = response_buf;

    std::span<uint8_t> buf_tx{(uint8_t*)&unit_, sizeof(Unit)};
    unit_.cmd_ = 0x03;
    unit_.start_reg_ = (unit_.start_reg_ << 8) | (unit_.start_reg_ >> 8);
    unit_.reg_count_ = (unit_.reg_count_ << 8) | (unit_.reg_count_ >> 8);
    unit_.crc_ = crc16(std::span<uint8_t>{buf_tx.data(), buf_tx.size() - 2});
    if (!io_.startWrite(buf_tx)) {
      return false;
    }

    tx_timer_.restart(
        UsType{buf_tx.size() * 1'000'000 / io_.getBaudrate().value() + 1'000});

    state_ = State::WaitTx;
    response_ = std::nullopt;

    return true;
  }

  uint32_t writeShrResponseSize() { return 8; }

  bool writeShr(Unit& unit, std::span<uint8_t> response_buf) {
    if (state_ != State::Idle) {
      return false;
    }
    if (response_buf.size() != 8) {
      return false;
    }

    unit_ = unit;
    response_buf_ = response_buf;

    std::span<uint8_t> buf_tx{(uint8_t*)&unit_, sizeof(Unit)};
    unit_.cmd_ = 0x06;
    unit_.start_reg_ = (unit_.start_reg_ << 8) | (unit_.start_reg_ >> 8);
    unit_.reg_count_ = (unit_.reg_count_ << 8) | (unit_.reg_count_ >> 8);
    unit_.crc_ = crc16(std::span<uint8_t>{buf_tx.data(), buf_tx.size() - 2});

    if (!io_.startWrite(buf_tx)) {
      return false;
    }

    tx_timer_.restart(
        UsType{buf_tx.size() * 1'000'000 / io_.getBaudrate().value() + 1'000});

    state_ = State::WaitTx;
    response_ = std::nullopt;

    return true;
  }

  uint32_t writeMhrRequestSize(Unit& unit) { return unit.reg_count_ * 2u + 9u; }
  uint32_t writeMhrResponseSize(Unit& unit) { return 8u; }

  bool writeMhr(Unit& unit, std::span<uint8_t> data,
                std::span<uint8_t> request_buf,
                std::span<uint8_t> response_buf) {
    if (state_ != State::Idle) {
      return false;
    }
    if (data.size() != unit.reg_count_ * 2u) {
      return false;
    }
    if (request_buf.size() != writeMhrRequestSize(unit)) {
      return false;
    }
    if (response_buf.size() != writeMhrResponseSize(unit)) {
      return false;
    }

    unit_ = unit;
    response_buf_ = response_buf;
    request_buf_ = request_buf;
    unit_.cmd_ = 0x10;

    auto offset = m::serialize(
        request_buf, unit_.addr_, unit_.cmd_, uint8_t(unit_.start_reg_ >> 8),
        uint8_t(unit_.start_reg_), uint8_t(unit_.reg_count_ >> 8),
        uint8_t(unit_.reg_count_), uint8_t(unit_.reg_count_ * 2u));

    std::copy(data.begin(), data.end(), request_buf.begin() + offset);
    // Modbus send regs value in big endian
    dataToBigEndian(
        std::span<uint8_t>{request_buf.data() + offset, data.size()});

    auto crc =
        crc16(std::span<uint8_t>{request_buf.data(), request_buf.size() - 2});
    request_buf[request_buf.size() - 2] = crc;
    request_buf[request_buf.size() - 1] = crc >> 8;

    if (!io_.startWrite(request_buf)) {
      return false;
    }

    tx_timer_.restart(UsType{
        request_buf.size() * 1'000'000 / io_.getBaudrate().value() + 1'000});

    state_ = State::WaitTx;
    response_ = std::nullopt;

    return true;
  }

  std::optional<std::span<uint8_t>> getResponse() { return response_; }

  enum class Error : uint8_t {
    None = 0,
    IllegalFunction,
    IllegalDataAddress,
    IllegalDataValue,
    SlaveDeviceFailure,
    Acknowledge,
    SlaveDeviceBusy,
    NegativeAcknowledge,
    MemParityError,
    BadCrc,
    BadCmd,
    BadSize,
    Corrupted,
    WriteShrBadResponse
  };
  std::optional<Error> checkResponse(std::span<uint8_t> response) {
    if (response.size() < 5) {
      return Error::Corrupted;
    }

    auto crc = crc16(response.first(response.size() - 2));
    auto [crc_origin] = m::deserialize<uint16_t>(response.last(2));
    if (crc != crc_origin) {
      return Error::BadCrc;
    }

    switch (response[1]) {
      case 0x03:
        if (response.size() != response[2] + 5u) {
          return Error::BadSize;
        }
        break;
      case 0x06: {
        if (response.size() != 8u) {
          return Error::BadSize;
        }
        std::span<uint8_t> temp{(uint8_t*)&unit_, sizeof(Unit)};
        for (auto i = 0u; i < response.size(); ++i) {
          if (response[i] != temp[i]) {
            return Error::Corrupted;
          }
        }
      } break;
      case 0x10: {
        if (response.size() != writeMhrResponseSize(unit_)) {
          return Error::BadSize;
        }
        for (auto i = 0u; i < 6; ++i) {
          if (response[i] != request_buf_[i]) {
            return Error::Corrupted;
          }
        }
      } break;
      case 0x83:
      case 0x86:
      case 0x90:
        if (response[2] >= uint8_t(Error::IllegalFunction) &&
            response[2] <= uint8_t(Error::MemParityError)) {
          return Error(response[2]);
        }
        break;

      default:
        return Error::BadCmd;
        break;
    }

    return Error::None;
  }

  bool changeResponseEndian(std::span<uint8_t> response) {
    if (response.size() >= 7) {
      if (response[1] == 0x03) {
        dataToBigEndian(response.subspan(3, response.size() - 5));
        return true;
      }
    }
    return false;
  }

  void handle() {
    switch (state_) {
      case State::Idle:
        break;
      case State::WaitTx: {
        if (io_.isWriteDone()) {
          start_rx_timer_.restart(timings_.rx_delay);

          state_ = State::StartRx;
        } else {
          if (tx_timer_.timeOver()) {
            io_.abortWrite();
            state_ = State::Idle;
          }
        }
      } break;

      case State::StartRx: {
        if (start_rx_timer_.timeOver()) {
          if (!io_.startRead(response_buf_)) {
            io_.abortRead();
            state_ = State::Idle;
          } else {
            rx_timer_.restart(UsType{response_buf_.size() * 1'000'000 /
                                         io_.getBaudrate().value() +
                                     500 + timings_.response_delay.value()});

            state_ = State::WaitRx;
          }
        }
      } break;

      case State::WaitRx: {
        if (io_.isReadDone()) {
          response_ = response_buf_;
          state_ = State::Idle;
        } else {
          if (rx_timer_.timeOver()) {
            response_ =
                std::span<uint8_t>{response_buf_.data(), io_.bytesReaded()};
            io_.abortRead();
            state_ = State::Idle;
          }
        }
      } break;

      default:
        break;
    }
  }

  static bool dataToBigEndian(std::span<uint8_t> data) {
    if (data.size() % 2 != 0) return false;
    for (std::size_t i = 0; i < data.size() - 1; i += 2) {
      std::swap(data[i], data[i + 1]);
    }
    return true;
  }

 private:
  IoT& io_;
  TimeUsT& time_;
  Timings timings_;
  m::Timer<UsType> tx_timer_;
  m::Timer<UsType> start_rx_timer_;
  m::Timer<UsType> rx_timer_;

  std::span<uint8_t> response_buf_;
  std::optional<std::span<uint8_t>> response_;
  std::span<uint8_t> request_buf_;

  enum class State : uint8_t { Idle, WaitTx, StartRx, WaitRx };
  State state_ = State::Idle;

  Unit unit_;

  uint16_t crc16(std::span<uint8_t> data) {
    static const uint16_t table[2] = {0x0000, 0xA001};
    uint16_t crc = 0xFFFF;
    uint16_t xorv = 0;

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

template <typename T>
concept CModbusRtuMaster = requires(T modbus) {
  typename T::Unit;
  typename T::Error;
  typename T::UsType;

  requires requires(typename T::Unit unit, std::span<uint8_t> buf,
                    std::span<uint8_t> data) {
    { modbus.readMhrResponseSize(unit) } -> std::same_as<uint16_t>;
    { modbus.readMhr(unit, buf) } -> std::same_as<bool>;

    { modbus.writeShrResponseSize() } -> std::same_as<uint32_t>;
    { modbus.writeShr(unit, buf) } -> std::same_as<bool>;

    { modbus.writeMhrRequestSize(unit) } -> std::same_as<uint32_t>;
    { modbus.writeMhrResponseSize(unit) } -> std::same_as<uint32_t>;
    { modbus.writeMhr(unit, data, buf, buf) } -> std::same_as<bool>;

    { modbus.getResponse() } -> std::same_as<std::optional<std::span<uint8_t>>>;
    {
      modbus.checkResponse(buf)
    } -> std::same_as<std::optional<typename T::Error>>;
    { modbus.changeResponseEndian(buf) } -> std::same_as<bool>;

    { modbus.handle() } -> std::same_as<void>;
  };
};

static_assert(CModbusRtuMaster<ModbusRtuMaster<ifc::IIO_Async<Bps<uint32_t>>,
                                               ifc::ITime<Us<uint32_t>>>>,
              "ModbusRtuMaster must satisfy CModbusRtuMaster concept");

template <typename IoT, typename TimeUsT>
ModbusRtuMaster(IoT&, TimeUsT&, typename ModbusRtuMaster<IoT, TimeUsT>::Timings)
    -> ModbusRtuMaster<IoT, TimeUsT>;
}  // namespace m

#endif  // MODBUS_RTU_MASTER_HPP