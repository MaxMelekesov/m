/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_MASTER_WRAPPER_HPP
#define MODBUS_RTU_MASTER_WRAPPER_HPP

#include <CoroMutex.hpp>
#include <CoroScheduler.hpp>
#include <ModbusRtuMaster.hpp>

namespace m {
template <CModbusRtuMaster Mdbs>
class ModbusRtuMasterWrapper {
 private:
  using Unit = typename Mdbs::Unit;
  using Error = typename Mdbs::Error;

 public:
  ModbusRtuMasterWrapper(Mdbs& modbus) : modbus_(modbus) {}

  auto readMhr(uint8_t addr, uint16_t reg_addr, uint16_t regs_num,
               std::span<uint8_t> data) -> Task<bool> {
    co_await mutex_.lock();
    Unit unit{addr, reg_addr, regs_num};
    auto size = modbus_.readMhrResponseSize(unit);
    auto span = std::span<uint8_t>{response_buf_.data(), size};
    if (!modbus_.readMhr(unit, span)) {
      co_return false;
    }
    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
      co_await CoroScheduler::yield();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      co_return false;
    } else {
      if (!modbus_.changeResponseEndian(resp.value())) co_return false;
      std::copy(resp.value().begin() + 3,
                resp.value().begin() + 3 + regs_num * 2, data.begin());

      co_return true;
    }

    co_return false;
  }

  auto writeShr(uint8_t addr, uint16_t reg_addr, uint16_t value)
      -> m::Task<bool> {
    co_await mutex_.lock();
    Unit unit{addr, reg_addr, value};
    auto span = std::span<uint8_t>{request_buf_.data(), 8};
    if (!modbus_.writeShr(unit, span)) {
      co_return false;
    }

    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
      co_await CoroScheduler::yield();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      co_return false;
    }

    co_return true;
  }

  auto writeMhr(uint8_t addr, uint16_t reg_addr, uint16_t regs_num,
                std::span<uint8_t> data) -> m::Task<bool> {
    co_await mutex_.lock();
    Unit unit{addr, reg_addr, regs_num};
    auto request_size = modbus_.writeMhrRequestSize(unit);
    auto request_span = std::span<uint8_t>{request_buf_.data(), request_size};

    auto response_size = modbus_.writeMhrResponseSize(unit);
    auto response_span =
        std::span<uint8_t>{response_buf_.data(), response_size};

    if (!modbus_.writeMhr(unit, data, request_span, response_span)) {
      co_return false;
    }
    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
      co_await CoroScheduler::yield();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      co_return false;
    }

    co_return true;
  }

 private:
  Mdbs& modbus_;
  m::CoroMutex mutex_;

  std::array<uint8_t, 256> response_buf_;
  std::array<uint8_t, 256> request_buf_;
};

template <typename T>

concept CModbusRtuMasterWrapper =
    requires(T wrapper, uint8_t addr, uint16_t reg_addr, uint16_t regs_num,
             uint16_t value, std::span<uint8_t> data) {
      {
        wrapper.readMhr(addr, reg_addr, regs_num, data)
      } -> std::same_as<m::Task<bool>>;
      {
        wrapper.writeShr(addr, reg_addr, value)
      } -> std::same_as<m::Task<bool>>;
      {
        wrapper.writeMhr(addr, reg_addr, regs_num, data)
      } -> std::same_as<m::Task<bool>>;
    };

static_assert(
    CModbusRtuMasterWrapper<ModbusRtuMasterWrapper<ModbusRtuMaster<
        ifc::IIO_Async<Bps<uint32_t>>, ifc::ITime<Us<uint32_t>>>>>,
    "ModbusRtuMasterWrapper must satisfy CModbusRtuMasterWrapper concept");
}  // namespace m

#endif  // MODBUS_RTU_MASTER_WRAPPER_HPP