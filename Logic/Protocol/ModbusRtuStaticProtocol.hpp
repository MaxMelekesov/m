/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MODBUS_RTU_STATIC_PROTOCOL_HPP
#define MODBUS_RTU_STATIC_PROTOCOL_HPP

#include <CoroDelay.hpp>
#include <CoroScheduler.hpp>
#include <CoroYield.hpp>
#include <IDataLink.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Us.hpp>
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace m {

enum class ModbusRtuError : uint8_t {
  IllegalFunction = 1,
  IllegalDataAddress = 2,
  IllegalDataValue = 3,
  SlaveDeviceFailure = 4,
  Acknowledge = 5,
  SlaveDeviceBusy = 6,
  MemoryParityError = 8,
};

template <typename HandlerT>
struct ModbusAddressNode {
  using Handler = HandlerT;
  uint8_t address;

  HandlerT handler;
};

template <typename HandlerT>
auto makeModbusAddressNode(uint8_t address, HandlerT&& handler)
    -> ModbusAddressNode<std::decay_t<HandlerT>> {
  return ModbusAddressNode<std::decay_t<HandlerT>>{
      address, std::forward<HandlerT>(handler)};
}

template <m::ifc::CTime TimeUsT, m::ifc::mcu::CPin PintT, typename... Nodes>
  requires(m::ifc::CUs<typename TimeUsT::Unit> && (sizeof...(Nodes) > 0))
class ModbusRtuStaticProtocol {
 private:
  template <typename Node>
  static consteval bool isModbusNode() {
    return requires {
      typename Node::Handler;
      { std::declval<Node&>().address } -> std::convertible_to<uint8_t>;
    };
  }

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

  using Error = ModbusRtuError;

  struct Timings {
    decltype(std::declval<TimeUsT&>().now()) tx_response_delay;
  };

  static_assert(sizeof...(Nodes) == 0 || (isModbusNode<Nodes>() && ...),
                "Nodes must be ModbusAddressNode-like types");

  explicit ModbusRtuStaticProtocol(m::ifc::IDataLink& data_link, TimeUsT& time,
                                   Timings timings, std::span<uint8_t> rx_buf,
                                   std::span<uint8_t> tx_buf, PintT& rx_led,
                                   PintT& tx_led, Nodes... nodes)
      : data_link_(data_link),
        time_(time),
        timings_(timings),
        rx_buf_(rx_buf),
        tx_buf_(tx_buf),
        rx_led_(rx_led),
        tx_led_(tx_led),
        nodes_(std::move(nodes)...) {}

  m::Task<bool> coroRun() {
    if (data_link_.error()) {
      state_ = State::Idle;
      if (!data_link_.stopReceive()) {
        co_return false;
      }
      if (!data_link_.stopTransmit()) {
        co_return false;
      }
    }

    if (running_) {
      if (data_link_.startReceive(rx_buf_)) {
        co_await m::coroYield();
      } else {
        co_return false;
      }
    } else {
      co_return true;
    }

    auto packet = data_link_.getPacket();
    while (!packet) {
      packet = data_link_.getPacket();
      co_await m::coroYield();
    }

    tx_packet_size_ = process(packet.value(), tx_buf_);
    if (!tx_packet_size_) {
      co_return true;
    }

    rx_led_.toggle();

    co_await m::coroDelay(time_, timings_.tx_response_delay);

    if (auto size = tx_packet_size_.value(); size) {
      if (!data_link_.startTransmit(tx_buf_.first(size))) {
        co_return false;
      }
      tx_led_.toggle();
    }

    auto tx_done = data_link_.transmitDone();
    while (!tx_done) {
      tx_done = data_link_.transmitDone();
      co_await m::coroYield();
    }

    co_return tx_done.value();
  }

  template <std::size_t I>
  void setNodeAddress(uint8_t addr) {
    std::get<I>(nodes_).address = addr;
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

 private:
  template <typename HandlerT>
  static constexpr bool ReadCoilsHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t coils_num,
               std::span<uint8_t> coils) {
        {
          handler.readCoils(start_addr, coils_num, coils)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool ReadDiscreteInputsHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t inputs_num,
               std::span<uint8_t> inputs) {
        {
          handler.readDiscreteInputs(start_addr, inputs_num, inputs)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool ReadHoldingRegistersHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t regs_num,
               std::span<uint8_t> regs) {
        {
          handler.readHoldingRegisters(start_addr, regs_num, regs)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool ReadInputRegistersHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t regs_num,
               std::span<uint8_t> regs) {
        {
          handler.readInputRegisters(start_addr, regs_num, regs)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool WriteSingleCoilHandlerV =
      requires(HandlerT& handler, uint16_t addr, bool value) {
        {
          handler.writeSingleCoil(addr, value)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool WriteSingleHoldingRegisterHandlerV =
      requires(HandlerT& handler, uint16_t addr, uint16_t value) {
        {
          handler.writeSingleHoldingRegister(addr, value)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool WriteMultipleCoilsHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t coils_num,
               std::span<uint8_t> coils) {
        {
          handler.writeMultipleCoils(start_addr, coils_num, coils)
        } -> std::same_as<std::optional<Error>>;
      };

  template <typename HandlerT>
  static constexpr bool WriteMultipleHoldingRegistersHandlerV =
      requires(HandlerT& handler, uint16_t start_addr, uint16_t regs_num,
               std::span<uint8_t> regs) {
        {
          handler.writeMultipleHoldingRegisters(start_addr, regs_num, regs)
        } -> std::same_as<std::optional<Error>>;
      };

  static constexpr bool SupportsReadCoilsV =
      (ReadCoilsHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsReadDiscreteInputsV =
      (ReadDiscreteInputsHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsReadHoldingRegistersV =
      (ReadHoldingRegistersHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsReadInputRegistersV =
      (ReadInputRegistersHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsWriteSingleCoilV =
      (WriteSingleCoilHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsWriteSingleHoldingRegisterV =
      (WriteSingleHoldingRegisterHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsWriteMultipleCoilsV =
      (WriteMultipleCoilsHandlerV<typename Nodes::Handler> || ...);
  static constexpr bool SupportsWriteMultipleHoldingRegistersV =
      (WriteMultipleHoldingRegistersHandlerV<typename Nodes::Handler> || ...);

  struct ProcessResult {
    std::optional<Error> error;
    uint32_t response_payload_size = 0;
  };

  m::ifc::IDataLink& data_link_;
  TimeUsT& time_;
  Timings timings_;
  std::span<uint8_t> rx_buf_;
  std::span<uint8_t> tx_buf_;
  PintT& rx_led_;
  PintT& tx_led_;
  std::tuple<Nodes...> nodes_;

  std::optional<uint32_t> tx_packet_size_;

  enum class State : uint8_t { Idle, ProcessPacket, TransmitResponse };
  State state_ = State::Idle;

  bool running_ = true;

  std::optional<uint32_t> process(std::span<uint8_t> rx_buf,
                                  std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 4 || rx_buf.size() > 256) {
      return std::nullopt;
    }

    const auto request_no_crc = rx_buf.first(rx_buf.size() - 2);
    const uint16_t crc_origin =
        static_cast<uint16_t>(rx_buf[rx_buf.size() - 2]) |
        (static_cast<uint16_t>(rx_buf[rx_buf.size() - 1]) << 8);
    if (crc16(request_no_crc) != crc_origin) {
      return std::nullopt;
    }

    const uint8_t addr = rx_buf[0];
    const uint8_t cmd = rx_buf[1];
    const auto request_payload = rx_buf.subspan(2, rx_buf.size() - 4);
    auto response_payload = tx_buf.subspan(2, tx_buf.size() - 4);

    const bool is_broadcast = (addr == 0);

    if (is_broadcast) {
      const auto cmd_enum = static_cast<Commands>(cmd);

      if (cmd_enum != Commands::WriteSingleCoil &&
          cmd_enum != Commands::WriteSingleHoldingRegister &&
          cmd_enum != Commands::WriteMultipleCoils &&
          cmd_enum != Commands::WriteMultipleHoldingRegisters) {
        return std::nullopt;
      }

      dispatchBroadcast([&](auto& node) {
        using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
        switch (cmd_enum) {
          case Commands::WriteSingleCoil:
            if constexpr (WriteSingleCoilHandlerV<HandlerT>) {
              processWriteSingleCoil(node.handler, request_payload,
                                     response_payload);
            }
            break;
          case Commands::WriteSingleHoldingRegister:
            if constexpr (WriteSingleHoldingRegisterHandlerV<HandlerT>) {
              processWriteSingleHoldingRegister(node.handler, request_payload,
                                                response_payload);
            }
            break;
          case Commands::WriteMultipleCoils:
            if constexpr (WriteMultipleCoilsHandlerV<HandlerT>) {
              processWriteMultipleCoils(node.handler, request_payload,
                                        response_payload);
            }
            break;
          case Commands::WriteMultipleHoldingRegisters:
            if constexpr (WriteMultipleHoldingRegistersHandlerV<HandlerT>) {
              processWriteMultipleHoldingRegisters(
                  node.handler, request_payload, response_payload);
            }
            break;
          default:
            break;
        }
      });

      return std::nullopt;
    }

    if (!containsAddress(addr)) {
      return std::nullopt;
    }

    ProcessResult result{Error::IllegalFunction, 0};

    switch (static_cast<Commands>(cmd)) {
      case Commands::ReadCoils:
        if constexpr (SupportsReadCoilsV) {
          result = dispatchReadCoils(addr, request_payload, response_payload);
        }
        break;
      case Commands::ReadDiscreteInputs:
        if constexpr (SupportsReadDiscreteInputsV) {
          result = dispatchReadDiscreteInputs(addr, request_payload,
                                              response_payload);
        }
        break;
      case Commands::ReadMultipleHoldingRegisters:
        if constexpr (SupportsReadHoldingRegistersV) {
          result = dispatchReadHoldingRegisters(addr, request_payload,
                                                response_payload);
        }
        break;
      case Commands::ReadInputRegisters:
        if constexpr (SupportsReadInputRegistersV) {
          result = dispatchReadInputRegisters(addr, request_payload,
                                              response_payload);
        }
        break;
      case Commands::WriteSingleCoil:
        if constexpr (SupportsWriteSingleCoilV) {
          result =
              dispatchWriteSingleCoil(addr, request_payload, response_payload);
        }
        break;
      case Commands::WriteSingleHoldingRegister:
        if constexpr (SupportsWriteSingleHoldingRegisterV) {
          result = dispatchWriteSingleHoldingRegister(addr, request_payload,
                                                      response_payload);
        }
        break;
      case Commands::WriteMultipleCoils:
        if constexpr (SupportsWriteMultipleCoilsV) {
          result = dispatchWriteMultipleCoils(addr, request_payload,
                                              response_payload);
        }
        break;
      case Commands::WriteMultipleHoldingRegisters:
        if constexpr (SupportsWriteMultipleHoldingRegistersV) {
          result = dispatchWriteMultipleHoldingRegisters(addr, request_payload,
                                                         response_payload);
        }
        break;
      default:
        break;
    }

    tx_buf[0] = addr;
    tx_buf[1] = cmd;

    uint32_t response_size = 2;
    if (result.error) {
      tx_buf[1] = static_cast<uint8_t>(cmd | 0x80U);
      tx_buf[2] = static_cast<uint8_t>(result.error.value());
      response_size += 1;
    } else {
      response_size += result.response_payload_size;
    }

    const auto crc = crc16(tx_buf.first(response_size));
    tx_buf[response_size] = static_cast<uint8_t>(crc);
    ++response_size;
    tx_buf[response_size] = static_cast<uint8_t>(crc >> 8);
    ++response_size;

    return response_size;
  }

  uint8_t firstAddress() const { return std::get<0>(nodes_).address; }

  bool containsAddress(uint8_t addr) {
    bool found = false;
    std::apply(
        [&](auto&... node) {
          ((found = found || (addr == node.address)), ...);
        },
        nodes_);
    return found;
  }

  template <typename Dispatcher>
  ProcessResult dispatchByAddress(uint8_t addr, Dispatcher&& dispatcher) {
    ProcessResult result{Error::IllegalDataAddress, 0};
    bool dispatched = false;

    std::apply(
        [&](auto&... node) {
          ((tryDispatchNode(addr, node, dispatched, result, dispatcher)), ...);
        },
        nodes_);

    return result;
  }

  template <typename NodeT, typename Dispatcher>
  static void tryDispatchNode(uint8_t addr, NodeT& node, bool& dispatched,
                              ProcessResult& result, Dispatcher& dispatcher) {
    if (dispatched) {
      return;
    }
    if (addr != node.address) {
      return;
    }

    dispatched = true;
    result = dispatcher(node);
  }

  template <typename Dispatcher>
  void dispatchBroadcast(Dispatcher&& dispatcher) {
    std::apply([&](auto&... node) { ((dispatcher(node)), ...); }, nodes_);
  }

  ProcessResult dispatchReadCoils(uint8_t addr, std::span<uint8_t> rx_buf,
                                  std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (ReadCoilsHandlerV<HandlerT>) {
        return processReadBits(node.handler, rx_buf, tx_buf,
                               &HandlerT::readCoils);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchReadDiscreteInputs(uint8_t addr,
                                           std::span<uint8_t> rx_buf,
                                           std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (ReadDiscreteInputsHandlerV<HandlerT>) {
        return processReadBits(node.handler, rx_buf, tx_buf,
                               &HandlerT::readDiscreteInputs);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchReadHoldingRegisters(uint8_t addr,
                                             std::span<uint8_t> rx_buf,
                                             std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (ReadHoldingRegistersHandlerV<HandlerT>) {
        return processReadRegisters(node.handler, rx_buf, tx_buf,
                                    &HandlerT::readHoldingRegisters);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchReadInputRegisters(uint8_t addr,
                                           std::span<uint8_t> rx_buf,
                                           std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (ReadInputRegistersHandlerV<HandlerT>) {
        return processReadRegisters(node.handler, rx_buf, tx_buf,
                                    &HandlerT::readInputRegisters);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchWriteSingleCoil(uint8_t addr, std::span<uint8_t> rx_buf,
                                        std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (WriteSingleCoilHandlerV<HandlerT>) {
        return processWriteSingleCoil(node.handler, rx_buf, tx_buf);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchWriteSingleHoldingRegister(uint8_t addr,
                                                   std::span<uint8_t> rx_buf,
                                                   std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (WriteSingleHoldingRegisterHandlerV<HandlerT>) {
        return processWriteSingleHoldingRegister(node.handler, rx_buf, tx_buf);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchWriteMultipleCoils(uint8_t addr,
                                           std::span<uint8_t> rx_buf,
                                           std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (WriteMultipleCoilsHandlerV<HandlerT>) {
        return processWriteMultipleCoils(node.handler, rx_buf, tx_buf);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  ProcessResult dispatchWriteMultipleHoldingRegisters(
      uint8_t addr, std::span<uint8_t> rx_buf, std::span<uint8_t> tx_buf) {
    return dispatchByAddress(addr, [&](auto& node) -> ProcessResult {
      using HandlerT = typename std::remove_cvref_t<decltype(node)>::Handler;
      if constexpr (WriteMultipleHoldingRegistersHandlerV<HandlerT>) {
        return processWriteMultipleHoldingRegisters(node.handler, rx_buf,
                                                    tx_buf);
      } else {
        return {Error::IllegalFunction, 0};
      }
    });
  }

  template <typename HandlerT, typename MethodT>
  ProcessResult processReadBits(HandlerT& handler, std::span<uint8_t> rx_buf,
                                std::span<uint8_t> tx_buf, MethodT method) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t start_address =
        (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t item_count =
        (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];

    if (item_count < 1 || item_count > 0x07D0) {
      return {Error::IllegalDataValue, 0};
    }

    const uint32_t range = static_cast<uint32_t>(start_address) + item_count;
    if (range > 0xFFFFU) {
      return {Error::IllegalDataAddress, 0};
    }

    const uint32_t byte_count = (item_count + 7U) / 8U;
    if (byte_count + 1U > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = static_cast<uint8_t>(byte_count);
    auto bits = tx_buf.subspan(1, byte_count);

    if (auto err = (handler.*method)(start_address, item_count, bits); err) {
      return {err, 0};
    }

    return {std::nullopt, byte_count + 1U};
  }

  template <typename HandlerT, typename MethodT>
  ProcessResult processReadRegisters(HandlerT& handler,
                                     std::span<uint8_t> rx_buf,
                                     std::span<uint8_t> tx_buf,
                                     MethodT method) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t start_address =
        (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t regs_num =
        (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];

    if (regs_num < 1 || regs_num > 0x007D) {
      return {Error::IllegalDataValue, 0};
    }

    const uint32_t range = static_cast<uint32_t>(start_address) + regs_num;
    if (range > 0xFFFFU) {
      return {Error::IllegalDataAddress, 0};
    }

    const uint32_t byte_count = static_cast<uint32_t>(regs_num) * 2U;
    if (byte_count + 1U > tx_buf.size()) {
      return {Error::SlaveDeviceFailure, 0};
    }

    tx_buf[0] = static_cast<uint8_t>(byte_count);
    auto regs = tx_buf.subspan(1, byte_count);

    if (auto err = (handler.*method)(start_address, regs_num, regs); err) {
      return {err, 0};
    }

    swapBytesInSpan(regs);
    return {std::nullopt, byte_count + 1U};
  }

  template <typename HandlerT>
  ProcessResult processWriteSingleCoil(HandlerT& handler,
                                       std::span<uint8_t> rx_buf,
                                       std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t addr = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t value = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];

    if (value != 0x0000U && value != 0xFF00U) {
      return {Error::IllegalDataValue, 0};
    }

    if (auto err = handler.writeSingleCoil(addr, value == 0xFF00U); err) {
      return {err, 0};
    }

    std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
    return {std::nullopt, 4};
  }

  template <typename HandlerT>
  ProcessResult processWriteSingleHoldingRegister(HandlerT& handler,
                                                  std::span<uint8_t> rx_buf,
                                                  std::span<uint8_t> tx_buf) {
    if (rx_buf.size() != 4) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t addr = (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t value = (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];

    if (auto err = handler.writeSingleHoldingRegister(addr, value); err) {
      return {err, 0};
    }

    std::copy(rx_buf.begin(), rx_buf.end(), tx_buf.begin());
    return {std::nullopt, 4};
  }

  template <typename HandlerT>
  ProcessResult processWriteMultipleCoils(HandlerT& handler,
                                          std::span<uint8_t> rx_buf,
                                          std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t start_address =
        (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t coils_num =
        (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    const uint8_t byte_count = rx_buf[4];

    if (coils_num < 1 || coils_num > 0x07B0U ||
        byte_count != ((coils_num + 7U) / 8U)) {
      return {Error::IllegalDataValue, 0};
    }

    const uint32_t coils_range =
        static_cast<uint32_t>(start_address) + coils_num;
    if (coils_range > 0xFFFFU) {
      return {Error::IllegalDataAddress, 0};
    }

    if (rx_buf.size() != static_cast<std::size_t>(byte_count) + 5U) {
      return {Error::IllegalDataValue, 0};
    }

    auto coils = rx_buf.subspan(5, byte_count);
    if (auto err = handler.writeMultipleCoils(start_address, coils_num, coils);
        err) {
      return {err, 0};
    }

    std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
    return {std::nullopt, 4};
  }

  template <typename HandlerT>
  ProcessResult processWriteMultipleHoldingRegisters(
      HandlerT& handler, std::span<uint8_t> rx_buf, std::span<uint8_t> tx_buf) {
    if (rx_buf.size() < 5) {
      return {Error::IllegalDataValue, 0};
    }

    const uint16_t start_address =
        (static_cast<uint16_t>(rx_buf[0]) << 8) | rx_buf[1];
    const uint16_t regs_num =
        (static_cast<uint16_t>(rx_buf[2]) << 8) | rx_buf[3];
    const uint8_t byte_count = rx_buf[4];

    if (regs_num < 1 || regs_num > 0x007BU || byte_count != regs_num * 2U) {
      return {Error::IllegalDataValue, 0};
    }

    const uint32_t regs_range = static_cast<uint32_t>(start_address) + regs_num;
    if (regs_range > 0xFFFFU) {
      return {Error::IllegalDataAddress, 0};
    }

    if (rx_buf.size() != static_cast<std::size_t>(byte_count) + 5U) {
      return {Error::IllegalDataValue, 0};
    }

    auto regs = rx_buf.subspan(5, regs_num * 2U);
    swapBytesInSpan(regs);

    if (auto err = handler.writeMultipleHoldingRegisters(start_address,
                                                         regs_num, regs);
        err) {
      return {err, 0};
    }

    std::copy(rx_buf.begin(), rx_buf.begin() + 4, tx_buf.begin());
    return {std::nullopt, 4};
  }

  static void swapBytesInSpan(std::span<uint8_t> regs) {
    for (std::size_t i = 0; i + 1U < regs.size(); i += 2U) {
      std::swap(regs[i], regs[i + 1U]);
    }
  }

  static uint16_t crc16(std::span<const uint8_t> data) {
    static constexpr uint16_t table[2] = {0x0000, 0xA001};

    uint16_t crc = 0xFFFF;
    for (uint8_t byte : data) {
      crc ^= byte;
      for (uint8_t bit = 0; bit < 8U; ++bit) {
        const uint16_t xorv = crc & 0x01U;
        crc >>= 1U;
        crc ^= table[xorv];
      }
    }

    return crc;
  }
};

template <m::ifc::CTime TimeUsT, m::ifc::mcu::CPin PintT, typename... Nodes>
  requires m::ifc::CUs<typename TimeUsT::Unit>
ModbusRtuStaticProtocol(m::ifc::IDataLink&, TimeUsT&,
                        typename ModbusRtuStaticProtocol<
                            TimeUsT, PintT, std::decay_t<Nodes>...>::Timings,
                        std::span<uint8_t>, std::span<uint8_t>, PintT&, PintT&,
                        Nodes&&...)
    -> ModbusRtuStaticProtocol<TimeUsT, PintT, std::decay_t<Nodes>...>;

// ---------------------------------------------------------------------------
// NoModbusCallback — sentinel for unregistered commands
// ---------------------------------------------------------------------------

struct NoModbusCallback {};

namespace detail {

template <typename T>
inline constexpr bool IsNoCallback =
    std::is_same_v<std::decay_t<T>, NoModbusCallback>;

}  // namespace detail

// ---------------------------------------------------------------------------
// ModbusLambdaHandler
//
// Adapter that wraps per-command lambdas (or any callable) into a handler
// compatible with ModbusRtuStaticProtocol. Commands without a callable are
// excluded at compile time — zero RAM and zero code overhead.
//
// Build with makeModbusLambdaHandler() + chained .withXxx() calls:
//
//   auto h = m::makeModbusLambdaHandler()
//       .withReadHoldingRegisters([&](uint16_t start, uint16_t num,
//                                     std::span<uint8_t> regs)
//                                     -> std::optional<m::ModbusRtuError> {
//           ...
//           return std::nullopt;
//       })
//       .withWriteSingleHoldingRegister([&](uint16_t addr, uint16_t value)
//                                           -> std::optional<m::ModbusRtuError>
//                                           {
//           ...
//           return std::nullopt;
//       })
//       .build();
// ---------------------------------------------------------------------------

template <
    typename RcCbT = NoModbusCallback, typename RdiCbT = NoModbusCallback,
    typename RhrCbT = NoModbusCallback, typename RirCbT = NoModbusCallback,
    typename WscCbT = NoModbusCallback, typename WshrCbT = NoModbusCallback,
    typename WmcCbT = NoModbusCallback, typename WmhrCbT = NoModbusCallback>
class ModbusLambdaHandler {
 public:
  using Error = ModbusRtuError;

  [[no_unique_address]] RcCbT rc_cb;
  [[no_unique_address]] RdiCbT rdi_cb;
  [[no_unique_address]] RhrCbT rhr_cb;
  [[no_unique_address]] RirCbT rir_cb;
  [[no_unique_address]] WscCbT wsc_cb;
  [[no_unique_address]] WshrCbT wshr_cb;
  [[no_unique_address]] WmcCbT wmc_cb;
  [[no_unique_address]] WmhrCbT wmhr_cb;

  std::optional<Error> readCoils(uint16_t start_addr, uint16_t coils_num,
                                 std::span<uint8_t> coils)
    requires(!detail::IsNoCallback<RcCbT>)
  {
    return rc_cb(start_addr, coils_num, coils);
  }

  std::optional<Error> readDiscreteInputs(uint16_t start_addr,
                                          uint16_t inputs_num,
                                          std::span<uint8_t> inputs)
    requires(!detail::IsNoCallback<RdiCbT>)
  {
    return rdi_cb(start_addr, inputs_num, inputs);
  }

  std::optional<Error> readHoldingRegisters(uint16_t start_addr,
                                            uint16_t regs_num,
                                            std::span<uint8_t> regs)
    requires(!detail::IsNoCallback<RhrCbT>)
  {
    return rhr_cb(start_addr, regs_num, regs);
  }

  std::optional<Error> readInputRegisters(uint16_t start_addr,
                                          uint16_t regs_num,
                                          std::span<uint8_t> regs)
    requires(!detail::IsNoCallback<RirCbT>)
  {
    return rir_cb(start_addr, regs_num, regs);
  }

  std::optional<Error> writeSingleCoil(uint16_t addr, bool value)
    requires(!detail::IsNoCallback<WscCbT>)
  {
    return wsc_cb(addr, value);
  }

  std::optional<Error> writeSingleHoldingRegister(uint16_t addr, uint16_t value)
    requires(!detail::IsNoCallback<WshrCbT>)
  {
    return wshr_cb(addr, value);
  }

  std::optional<Error> writeMultipleCoils(uint16_t start_addr,
                                          uint16_t coils_num,
                                          std::span<uint8_t> coils)
    requires(!detail::IsNoCallback<WmcCbT>)
  {
    return wmc_cb(start_addr, coils_num, coils);
  }

  std::optional<Error> writeMultipleHoldingRegisters(uint16_t start_addr,
                                                     uint16_t regs_num,
                                                     std::span<uint8_t> regs)
    requires(!detail::IsNoCallback<WmhrCbT>)
  {
    return wmhr_cb(start_addr, regs_num, regs);
  }
};

// ---------------------------------------------------------------------------
// ModbusLambdaHandlerBuilder — builder for ModbusLambdaHandler
// ---------------------------------------------------------------------------

template <
    typename RcCbT = NoModbusCallback, typename RdiCbT = NoModbusCallback,
    typename RhrCbT = NoModbusCallback, typename RirCbT = NoModbusCallback,
    typename WscCbT = NoModbusCallback, typename WshrCbT = NoModbusCallback,
    typename WmcCbT = NoModbusCallback, typename WmhrCbT = NoModbusCallback>
struct ModbusLambdaHandlerBuilder {
  RcCbT rc_cb;
  RdiCbT rdi_cb;
  RhrCbT rhr_cb;
  RirCbT rir_cb;
  WscCbT wsc_cb;
  WshrCbT wshr_cb;
  WmcCbT wmc_cb;
  WmhrCbT wmhr_cb;

  template <typename F>
  auto withReadCoils(F&& f) const {
    return ModbusLambdaHandlerBuilder<std::decay_t<F>, RdiCbT, RhrCbT, RirCbT,
                                      WscCbT, WshrCbT, WmcCbT, WmhrCbT>{
        std::forward<F>(f),
        rdi_cb,
        rhr_cb,
        rir_cb,
        wsc_cb,
        wshr_cb,
        wmc_cb,
        wmhr_cb};
  }

  template <typename F>
  auto withReadDiscreteInputs(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, std::decay_t<F>, RhrCbT, RirCbT,
                                      WscCbT, WshrCbT, WmcCbT, WmhrCbT>{
        rc_cb,  std::forward<F>(f), rhr_cb, rir_cb, wsc_cb, wshr_cb, wmc_cb,
        wmhr_cb};
  }

  template <typename F>
  auto withReadHoldingRegisters(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, std::decay_t<F>, RirCbT,
                                      WscCbT, WshrCbT, WmcCbT, WmhrCbT>{
        rc_cb,  rdi_cb, std::forward<F>(f), rir_cb, wsc_cb, wshr_cb,
        wmc_cb, wmhr_cb};
  }

  template <typename F>
  auto withReadInputRegisters(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, RhrCbT, std::decay_t<F>,
                                      WscCbT, WshrCbT, WmcCbT, WmhrCbT>{
        rc_cb,  rdi_cb,  rhr_cb, std::forward<F>(f),
        wsc_cb, wshr_cb, wmc_cb, wmhr_cb};
  }

  template <typename F>
  auto withWriteSingleCoil(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, RhrCbT, RirCbT,
                                      std::decay_t<F>, WshrCbT, WmcCbT,
                                      WmhrCbT>{
        rc_cb,   rdi_cb, rhr_cb, rir_cb, std::forward<F>(f),
        wshr_cb, wmc_cb, wmhr_cb};
  }

  template <typename F>
  auto withWriteSingleHoldingRegister(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, RhrCbT, RirCbT, WscCbT,
                                      std::decay_t<F>, WmcCbT, WmhrCbT>{
        rc_cb,  rdi_cb, rhr_cb, rir_cb, wsc_cb, std::forward<F>(f),
        wmc_cb, wmhr_cb};
  }

  template <typename F>
  auto withWriteMultipleCoils(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, RhrCbT, RirCbT, WscCbT,
                                      WshrCbT, std::decay_t<F>, WmhrCbT>{
        rc_cb,  rdi_cb, rhr_cb, rir_cb, wsc_cb, wshr_cb, std::forward<F>(f),
        wmhr_cb};
  }

  template <typename F>
  auto withWriteMultipleHoldingRegisters(F&& f) const {
    return ModbusLambdaHandlerBuilder<RcCbT, RdiCbT, RhrCbT, RirCbT, WscCbT,
                                      WshrCbT, WmcCbT, std::decay_t<F>>{
        rc_cb,  rdi_cb,  rhr_cb, rir_cb,
        wsc_cb, wshr_cb, wmc_cb, std::forward<F>(f)};
  }

  auto build() const {
    return ModbusLambdaHandler<RcCbT, RdiCbT, RhrCbT, RirCbT, WscCbT, WshrCbT,
                               WmcCbT, WmhrCbT>{
        rc_cb, rdi_cb, rhr_cb, rir_cb, wsc_cb, wshr_cb, wmc_cb, wmhr_cb};
  }
};

inline auto makeModbusHandler() { return ModbusLambdaHandlerBuilder<>{}; }

}  // namespace m

#endif  // MODBUS_RTU_STATIC_PROTOCOL_HPP