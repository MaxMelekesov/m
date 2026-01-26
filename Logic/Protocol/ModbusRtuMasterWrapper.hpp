/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_MASTER_WRAPPER_H
#define MODBUS_RTU_MASTER_WRAPPER_H

#include <ModbusRtuMaster.hpp>

namespace m {
template <m::ifc::CUs UsType, m::ifc::CBps BpsType>
class ModbusRtuMasterWrapper {
 private:
  using Unit = m::ModbusRtuMaster<UsType, BpsType>::Unit;
  using Error = m::ModbusRtuMaster<UsType, BpsType>::Error;

 public:
  ModbusRtuMasterWrapper(m::ModbusRtuMaster<UsType, BpsType>& modbus)
      : modbus_(modbus) {}

  bool readMhr(uint8_t addr, uint16_t reg_addr, uint16_t regs_num,
               std::span<uint8_t> data) {
    Unit unit{addr, reg_addr, regs_num};
    auto size = modbus_.readMhrResponseSize(unit);
    auto span = std::span<uint8_t>{response_buf_.data(), size};
    if (!modbus_.readMhr(unit, span)) {
      return false;
    }
    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      return false;
    } else {
      if (!modbus_.changeResponseEndian(resp.value())) return false;
      std::copy(resp.value().begin() + 3,
                resp.value().begin() + 3 + regs_num * 2, data.begin());

      return true;
    }

    return false;
  }

  bool writeShr(uint8_t addr, uint16_t reg_addr, uint16_t value) {
    Unit unit{addr, reg_addr, value};
    auto span = std::span<uint8_t>{request_buf_.data(), 8};
    if (!modbus_.writeShr(unit, span)) {
      return false;
    }

    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      return false;
    }

    return true;
  }

  bool writeMhr(uint8_t addr, uint16_t reg_addr, uint16_t regs_num,
                std::span<uint8_t> data) {
    Unit unit{addr, reg_addr, regs_num};
    auto request_size = modbus_.writeMhrRequestSize(unit);
    auto request_span = std::span<uint8_t>{request_buf_.data(), request_size};

    auto response_size = modbus_.writeMhrResponseSize(unit);
    auto response_span =
        std::span<uint8_t>{response_buf_.data(), response_size};

    if (!modbus_.writeMhr(unit, data, request_span, response_span)) {
      return false;
    }
    auto resp = modbus_.getResponse();
    while (!resp) {
      modbus_.handle();
      resp = modbus_.getResponse();
    }

    if (auto err = modbus_.checkResponse(resp.value()); err != Error::None) {
      return false;
    }

    return true;
  }

 private:
  m::ModbusRtuMaster<UsType, BpsType>& modbus_;

  std::array<uint8_t, 256> response_buf_;
  std::array<uint8_t, 256> request_buf_;
};
}  // namespace m

#endif  // MODBUS_RTU_MASTER_WRAPPER_H