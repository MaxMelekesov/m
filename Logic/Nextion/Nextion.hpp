/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef NEXTION_HPP
#define NEXTION_HPP

#include <CDataLink.hpp>
#include <Fsm_v4.hpp>
#include <NextionDataLink.hpp>
#include <TSerDes.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <variant>

namespace m::nxt {

namespace FN {
struct IdleState : m::State {};
struct ReceiveingState : m::State {};

struct StartEvent : m::Event {};
struct StopEvent : m::Event {};
struct ErrorEvent : m::Event {};
struct PacketReceivedEvent : m::Event {};

}  // namespace FN

enum class ReturnCode : uint8_t {
  Success = 0x01,                    // Command successful
  InvalidComponentId = 0x02,         // Component ID invalid
  InvalidPageId = 0x03,              // Page ID invalid
  InvalidPictureId = 0x04,           // Picture ID invalid
  InvalidFontId = 0x05,              // Font ID invalid
  InvalidFileOperation = 0x06,       // File operation failed
  Crc_Error = 0x09,                  // CRC error
  InvalidBaudRate = 0x11,            // Baud rate setting invalid
  InvalidCurve = 0x12,               // Invalid curve control ID
  InvalidVariableAssignment = 0x1A,  // Variable name/value invalid
  InvalidWaveformChannel = 0x1B,     // Invalid waveform channel
  InvalidWaveformMode = 0x1C,        // Invalid waveform mode
  InvalidWaveformSamples = 0x1D,     // Invalid waveform samples
  InvalidWaveformSampleRate = 0x1E,  // Invalid waveform sample rate
  SerialBufferOverflow = 0x24,       // Serial buffer overflow
  TouchEvent = 0x65,                 // Touch event
  CurrentPageNumber = 0x66,          // Current page number
  TouchCoordinate = 0x67,            // Touch coordinate
  TouchInSleep = 0x68,               // Touch event in sleep mode
  StringData = 0x70,                 // String data enclosed
  NumericData = 0x71,                // Numeric data enclosed
  AutoSleep = 0x86,               // Device automatically enters into sleep mode
  AutoWake = 0x87,                // Device automatically wakes up
  Ready = 0x88,                   // System successful start up
  StartMicroSD = 0x89,            // Start SD card upgrade
  TransparentDataReady = 0xFD,    // Transparent data finished
  TransparentDataFinished = 0xFE  // Transparent data ready
};

enum class EventType : uint8_t {
  Press = 0x01,        // Press event
  Release = 0x02,      // Release event
  ValueChanged = 0x03  // Value changed event
};

template <m::c::CRingDataLink IoType, std::size_t MaxComponents,
          std::size_t BufferSize>
class Nextion;

class Component {
 public:
  constexpr Component(uint8_t page_id, uint8_t component_id,
                      std::string_view name)
      : page_id_(page_id), component_id_(component_id), name_(name) {}

  virtual ~Component() = default;

  [[nodiscard]] constexpr uint8_t getPageId() const { return page_id_; }

  [[nodiscard]] constexpr uint8_t getComponentId() const {
    return component_id_;
  }

  [[nodiscard]] constexpr std::string_view getName() const { return name_; }

 protected:
  using EventValue =
      std::variant<uint32_t, std::span<uint8_t>, std::string_view>;

  virtual void onEvent(EventType event, EventValue value) = 0;

 private:
  uint8_t page_id_;
  uint8_t component_id_;
  std::string_view name_;

  template <m::c::CRingDataLink IoType, std::size_t MaxComponents,
            std::size_t BufferSize>
  friend class Nextion;
};

class Button : public Component {
 public:
  constexpr Button(uint8_t page_id, uint8_t component_id, std::string_view name,
                   std::function<void(EventType)>&& cb)
      : Component(page_id, component_id, name), cb_(std::move(cb)) {}

 private:
  std::function<void(EventType)> cb_;

  void onEvent(EventType event, Component::EventValue value) override {
    // std::holds_alternative<Component::EventValue::uint32_t>(value);
    cb_(event);
  }
};

template <typename T>
concept CNextion = requires(T nxt, const Component& component, uint32_t id,
                            std::string_view text) {
  { nxt.setPicture(component, id) } -> std::same_as<bool>;
  { nxt.setText(component, text) } -> std::same_as<bool>;
};

template <m::c::CRingDataLink IoType, std::size_t MaxComponents = 32,
          std::size_t BufferSize = 256>
class Nextion
    : public m::Fsm_v4<
          Nextion<IoType, MaxComponents, BufferSize>, FN::IdleState,
          m::Transition<FN::IdleState, FN::StartEvent, FN::ReceiveingState>,

          m::Transition<FN::ReceiveingState, FN::StopEvent, FN::IdleState>,
          m::Transition<FN::ReceiveingState, FN::ErrorEvent, FN::IdleState>,
          m::Transition<FN::ReceiveingState, FN::PacketReceivedEvent,
                        FN::ReceiveingState>

          > {
 private:
  using FsmBase = m::Fsm_v4<
      Nextion<IoType, MaxComponents, BufferSize>, FN::IdleState,
      m::Transition<FN::IdleState, FN::StartEvent, FN::ReceiveingState>,
      m::Transition<FN::ReceiveingState, FN::StopEvent, FN::IdleState>,
      m::Transition<FN::ReceiveingState, FN::ErrorEvent, FN::IdleState>,
      m::Transition<FN::ReceiveingState, FN::PacketReceivedEvent,
                    FN::ReceiveingState>>;
  using FsmBase::checkEvents;

 public:
  explicit Nextion(IoType& io) : io_(io), components_{}, component_count_{0} {}

  void handle() { checkEvents(); }

  void start() { start_ = true; }
  void stop() { start_ = false; }

  bool registerComponent(Component& component) {
    if (component_count_ >= MaxComponents) {
      return false;
    }

    components_[component_count_++] = &component;
    return true;
  }

  bool setPicture(const Component& component, uint32_t id) {
    auto length =
        snprintf(tx_buf_, tx_buf_.size(), "%.*s.pic=%u\xFF\xFF\xFF",
                 component.getName().size(), component.getName().data(), id);

    if (length <= 0 || length >= tx_buf_.size()) {
      return false;
    }

    std::span<const uint8_t> span(tx_buf_);
    bool res = sendCommandData(span.first(length));
    return res;
  }

  bool setText(const Component& component, std::string_view text) {
    auto length =
        snprintf(tx_buf_, tx_buf_.size(), "%.*s.txt=\"%.*s\"\xFF\xFF\xFF",
                 component.getName().size(), component.getName().data(),
                 text.size(), text.data());

    if (length <= 0 || length >= tx_buf_.size()) {
      return false;
    }

    std::span<const uint8_t> span(tx_buf_);
    bool res = sendCommandData(span.first(length));
    return res;
  }

 private:
  IoType& io_;
  std::array<Component*, MaxComponents> components_;
  std::size_t component_count_;

  bool start_ = false;

  std::array<uint8_t, BufferSize> rx_buf_;
  std::array<uint8_t, BufferSize> rx_buf_copy_;
  std::span<uint8_t> rx_buf_view_;
  std::array<uint8_t, BufferSize> tx_buf_;

  void parseCommand(std::span<uint8_t> packet) {
    if (packet.size() <
        4) {  // At least return code + component ID + event type + 0xFF
      return;
    }

    for (auto b : packet.last(3)) {
      if (b != 0xFF) return;
    }

    ReturnCode return_code = static_cast<ReturnCode>(packet[0]);

    switch (return_code) {
      case ReturnCode::TouchEvent:
        handleTouchEvent(packet);
        break;
      case ReturnCode::NumericData:
        handleNumericData(packet);
        break;
      case ReturnCode::StringData:
        handleStringData(packet);
        break;
      case ReturnCode::Ready:
        break;
      default:
        break;
    }
  }

  void handleTouchEvent(std::span<uint8_t> packet) {
    if (packet.size() < 7) {  // Return code + page ID + component ID + event
                              // type + value + 3xFF
      return;
    }

    uint8_t page_id = packet[1];
    uint8_t component_id = packet[2];
    EventType event = static_cast<EventType>(packet[3]);

    for (auto c : components_) {
      if (c->getPageId() == page_id && c->getComponentId() == component_id) {
        c->onEvent(event, 0u);
        break;
      }
    }
  }

  void handleNumericData(std::span<uint8_t> packet) {
    if (packet.size() <
        8) {  // Return code + component ID + 4 value bytes + 3xFF
      return;
    }

    uint8_t component_id = packet[1];

    auto [value] = m::deserialize<uint32_t>(packet.subspan(2, 4));

    for (auto c : components_) {
      if (c->getComponentId() == component_id) {
        c->onEvent(EventType::ValueChanged, value);
        break;
      }
    }
  }

  void handleStringData(std::span<uint8_t> packet) {
    if (packet.size() <
        5) {  // Return code + component ID + at least 1 char + 3xFF
      return;
    }

    uint8_t component_id = packet[1];

    std::size_t length =
        packet.size() - 5;  // Return code + component ID + 3xFF

    for (auto c : components_) {
      if (c->getComponentId() == component_id) {
        std::array<char, 64> tempStr{};
        std::size_t copyLength = std::min(length, tempStr.size());

        std::copy(packet.begin() + 2, packet.begin() + 2 + copyLength,
                  tempStr.begin());

        c->onEvent(EventType::ValueChanged,
                   std::string_view(tempStr.data(), copyLength));
        break;
      }
    }
  }

  bool sendCommandData(std::span<const uint8_t> data) {
    if (!io_.startTransmit(data)) {
      return false;
    }

    while (1) {
      if (auto value = io_.transmitDone(); value) {
        return value.value();
      }
    }

    return false;
  }

  // #############################
  //           Idle state
  // #############################
  bool checkEvent(FN::IdleState, FN::StartEvent) { return start_; }
  void handleEvent(FN::IdleState, FN::StartEvent) { io_.startReceive(rx_buf_); }

  // #############################
  //       Receiving state
  // #############################
  bool checkEvent(FN::ReceiveingState, FN::PacketReceivedEvent) {
    if (auto value = io_.getPacket(); value) {
      if (auto view = value->copyTo(rx_buf_copy_); view) {
        rx_buf_view_ = view.value();
        return true;
      }
    }
    return false;
  }
  void handleEvent(FN::ReceiveingState, FN::PacketReceivedEvent) {
    parseCommand(rx_buf_view_);
  }

  bool checkEvent(FN::ReceiveingState, FN::StopEvent) { return !start_; }
  void handleEvent(FN::ReceiveingState, FN::StopEvent) { io_.stopReceive(); }

  bool checkEvent(FN::ReceiveingState, FN::ErrorEvent) { return io_.error(); }
  void handleEvent(FN::ReceiveingState, FN::ErrorEvent) {
    io_.stopReceive();
    io_.stopTransmit();
  }

  friend FsmBase;
};

static_assert(CNextion<Nextion<NextionDataLink<Us<uint32_t>>, 64, 1024>>,
              "Nextion does not satisfy CNextion concept");

}  // namespace m::nxt

#endif  // NEXTION_HPP
