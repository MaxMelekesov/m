/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef LOG_ERROR_INDICATOR_HPP
#define LOG_ERROR_INDICATOR_HPP

#include <IPin.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Timer.hpp>
#include <bit>
#include <bitset>

namespace m {

template <typename T>
concept EnumClass =
    std::is_enum_v<T> && !std::is_convertible_v<T, std::underlying_type_t<T>>;

template <typename T>
concept EnumClassWithSize = EnumClass<T> && requires {
  T::None;
  T::Size;
  requires std::is_same_v<decltype(T::Size), T>;
};

template <m::ifc::CMs MsT, EnumClassWithSize ErrorT>
class ErrorLedIndicator {
 public:
  ErrorLedIndicator(ifc::mcu::IPin& led, ifc::ITime<MsT>& time,
                    MsT long_flash = MsT{1000}, MsT short_flash = MsT{200},
                    MsT pause_between_flashes = MsT{1000},
                    MsT pause_between_sequences = MsT{5000})
      : led_(led),
        time_(time),
        timer_(time),
        Long_Flash(long_flash),
        Short_Flash(short_flash),
        Pause_Between_Flashes(pause_between_flashes),
        Pause_Between_Sequences(pause_between_sequences) {}

  void setError(ErrorT error_code) {
    error_code_ = error_code;

    generateFlashSequence();
    resetState();
  }

  bool hasError() const { return has_error_; }

  void clearError() {
    has_error_ = false;
    led_.write(false);
  }

  void handle() {
    if (!has_error_) return;

    switch (state_) {
      case State::Idle: {
        if (timer_.restart(Pause_Between_Sequences)) {
          state_ = State::WaitingBetweenSequences;
        }
        break;
      }
      case State::WaitingBetweenSequences: {
        if (timer_.timeOver()) {
          currentFlashIndex_ = 0;
          state_ = State::FlashOn;
          led_.write(true);
          timer_.restart(isLongFlash(currentFlashIndex_) ? Long_Flash
                                                         : Short_Flash);
        }
        break;
      }
      case State::FlashOn: {
        if (timer_.timeOver()) {
          led_.write(false);
          timer_.restart(Pause_Between_Flashes);
          state_ = State::FlashOff;
        }
        break;
      }
      case State::FlashOff: {
        if (timer_.timeOver()) {
          currentFlashIndex_++;
          if (currentFlashIndex_ >= flashSequenceSize_) {
            state_ = State::Idle;
          } else {
            led_.write(true);
            timer_.restart(isLongFlash(currentFlashIndex_) ? Long_Flash
                                                           : Short_Flash);
            state_ = State::FlashOn;
          }
        }
        break;
      }
    }
  }

 private:
  ifc::mcu::IPin& led_;
  ifc::ITime<MsT>& time_;
  Timer<MsT> timer_;

  bool has_error_ = false;
  ErrorT error_code_ = ErrorT::None;

  static constexpr std::size_t bitsNeeded() {
    auto temp = std::bit_width(static_cast<std::size_t>(ErrorT::Size));
    return temp > 0 ? temp : 1;
  }
  static constexpr std::size_t Max_Flash_Sequence_Size = bitsNeeded();

  std::bitset<Max_Flash_Sequence_Size> flashSequence_;
  std::size_t flashSequenceSize_ = 0;
  std::size_t currentFlashIndex_ = 0;

  enum class State { Idle, WaitingBetweenSequences, FlashOn, FlashOff };
  State state_ = State::Idle;

  const MsT Long_Flash;
  const MsT Short_Flash;
  const MsT Pause_Between_Flashes;
  const MsT Pause_Between_Sequences;

  void resetState() {
    has_error_ = false;
    state_ = State::Idle;
    currentFlashIndex_ = 0;
    led_.write(false);
  }

  [[nodiscard]] constexpr bool isLongFlash(std::size_t index) const {
    if (index < flashSequenceSize_) {
      return flashSequence_[index];
    }
    return false;
  }

  void generateFlashSequence() {
    flashSequence_.reset();

    if (error_code_ == ErrorT::None) {
      flashSequenceSize_ = 1;
      return;
    }

    auto temp = static_cast<std::size_t>(error_code_);
    flashSequenceSize_ = std::bit_width(temp);

    for (std::size_t i = 0; i < flashSequenceSize_; ++i) {
      if ((temp & (1u << i)) != 0) {
        flashSequence_.set(flashSequenceSize_ - i - 1);
      }
    }
  }
};

}  // namespace m

#endif  // LOG_ERROR_INDICATOR_HPP