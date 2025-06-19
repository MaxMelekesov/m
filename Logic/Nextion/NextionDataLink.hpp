/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef NEXTION_DATA_LINK_HPP
#define NEXTION_DATA_LINK_HPP

#include <IDataLink.hpp>
#include <IIO_Async.hpp>
#include <Timer.hpp>
#include <Us.hpp>
#include <cstdint>
#include <cstring>
#include <optional>
#include <ranges>

namespace m::nxt {

template <m::ifc::CUs UsType>
class NextionDataLink {
 public:
  NextionDataLink(m::ifc::ITime<UsType> &time, m::ifc::IIO_Async &io)
      : io_(io), tx_timeout_timer_{time}, rx_timeout_timer_(time) {}

  bool startReceive(std::span<uint8_t> rx_buf) {
    if (!io_.readDone()) return false;
    rx_buf_ = rx_buf;
    if (!io_.abortRead()) {
      return false;
    }
    if (!io_.readAsync(rx_buf_)) {
      return false;
    }

    head_ = 0;

    return true;
  }

  std::optional<m::ifc::RingSpan> getPacket() {
    auto tail = io_.bytesAvailable();
    if (tail == scan_pos_) {
      // Нет новых байт, проверяем таймаут
      if (rx_timeout_timer_.timeOver() && head_ != scan_pos_) {
        // Возвращаем всё, что накопилось (включая возможные терминирующие
        // байты)
        c::RingSpan span;
        if (head_ < scan_pos_) {
          span.first = rx_buf_.subspan(head_, scan_pos_ - head_);
          span.second = {};
        } else {
          span.first = rx_buf_.subspan(head_, rx_buf_.size() - head_);
          span.second = rx_buf_.subspan(0, scan_pos_);
        }
        head_ = scan_pos_;
        terminator_counter_ = 0;
        return span;
      }
      return std::nullopt;
    }

    // Сбросить/перезапустить таймер при появлении новых байт
    rx_timeout_timer_.restart(
        UsType{5 * 1'000'000 / io_.getBaudrate() + 3'000});

    // Создаём view на новые данные (от scan_pos_ до tail)
    c::RingSpan segments;
    if (scan_pos_ < tail) {
      segments.first = rx_buf_.subspan(scan_pos_, tail - scan_pos_);
      segments.second = {};
    } else {
      segments.first = rx_buf_.subspan(scan_pos_, rx_buf_.size() - scan_pos_);
      segments.second = rx_buf_.subspan(0, tail);
    }

    auto view =
        std::ranges::views::join(std::array{segments.first, segments.second});

    auto it = std::ranges::find_if(
        view, [this](uint8_t b) { return isPacketTerminator(b); });

    if (it == view.end()) {
      // Терминатор не найден, обновляем scan_pos_ до tail
      scan_pos_ = tail;
      return std::nullopt;
    }

    // Терминатор найден — возвращаем пакет вместе с терминирующими байтами
    std::size_t processed = std::distance(view.begin(), it);
    std::size_t packet_end =
        (scan_pos_ + processed + 1) %
        rx_buf_.size();  // +1 чтобы включить байт-терминатор

    c::RingSpan span;
    if (head_ < packet_end) {
      span.first = rx_buf_.subspan(head_, packet_end - head_);
      span.second = {};
    } else {
      span.first = rx_buf_.subspan(head_, rx_buf_.size() - head_);
      span.second = rx_buf_.subspan(0, packet_end);
    }

    // Обновляем head_ и scan_pos_ (после возвращённого пакета)
    head_ = packet_end;
    scan_pos_ = head_;
    terminator_counter_ = 0;
    return span;
  }

  bool startTransmit(std::span<const uint8_t> tx_buf) {
    if (!io_.abortWrite()) {
      return false;
    }
    if (!io_.writeAsync(tx_buf)) {
      return false;
    }

    tx_timeout_timer_.restart(
        UsType{tx_buf.size() * 1'000'000 / io_.getBaudrate() + 3'000});

    return true;
  }

  std::optional<bool> transmitDone() {
    if (io_.writeDone()) {
      if (!io_.abortWrite()) {
        return false;
      }
      return true;
    } else {
      if (tx_timeout_timer_.timeOver()) {
        if (!io_.abortWrite()) {
        }
        return false;
      }
    }
    return std::nullopt;
  }

  bool error() { return io_.error(); }

  bool stopReceive() {
    if (!io_.abortRead()) {
      return false;
    }
    return true;
  }

  bool stopTransmit() {
    if (!io_.abortWrite()) {
      return false;
    }
    return true;
  }

 private:
  ifc::IIO_Async &io_;

  Timer<UsType> tx_timeout_timer_;
  Timer<UsType> rx_timeout_timer_;

  std::span<uint8_t> rx_buf_;
  std::size_t head_ = 0;
  std::size_t scan_pos_ = 0;

  uint8_t terminator_counter_ = 0;
  bool isPacketTerminator(uint8_t byte) {
    if (byte == 0xFF) {
      terminator_counter_++;
      if (terminator_counter_ == 3) {
        terminator_counter_ = 0;
        return true;
      }
    } else {
      terminator_counter_ = 0;
    }
    return false;
  }
};

static_assert(m::ifc::CRingDataLink<NextionDataLink<Us<uint32_t>>>,
              "NextionDataLink does not satisfy CDataLink concept");

}  // namespace m::nxt

#endif  // NEXTION_DATA_LINK_HPP