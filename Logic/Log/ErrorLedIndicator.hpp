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
#include <cstdint>

namespace m {

template <typename TimeUnit, uint32_t MaxErrorCode>
class ErrorLedIndicator {
 public:
  ErrorLedIndicator(ifc::mcu::IPin& led, ifc::ITime<Ms<TimeUnit>>& time,
                    Ms<TimeUnit> long_flash = Ms<TimeUnit>{1000},
                    Ms<TimeUnit> short_flash = Ms<TimeUnit>{200},
                    Ms<TimeUnit> pause_between_flashes = Ms<TimeUnit>{1000},
                    Ms<TimeUnit> pause_between_sequences = Ms<TimeUnit>{5000})
      : led_(led),
        time_(time),
        timer_(time),
        Long_Flash(long_flash),
        Short_Flash(short_flash),
        Pause_Between_Flashes(pause_between_flashes),
        Pause_Between_Sequences(pause_between_sequences) {}

  void setError(uint32_t errorCode) {
    if (errorCode > MaxErrorCode) {
      errorCode_ = MaxErrorCode;
    } else {
      errorCode_ = errorCode;
    }
    generateFlashSequence();
    resetState();
  }

  void clearError() {
    hasError_ = false;
    led_.write(false);
  }

  void handle() {
    if (!hasError_) return;

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
  ifc::ITime<Ms<TimeUnit>>& time_;
  Timer<Ms<TimeUnit>> timer_;

  bool hasError_ = false;
  uint32_t errorCode_ = 0;

  static constexpr std::size_t bitsNeeded(uint32_t value) {
    return std::bit_width(value) > 0 ? std::bit_width(value) : 1;
  }
  static constexpr std::size_t Max_Flash_Sequence_Size =
      bitsNeeded(MaxErrorCode);

  std::bitset<Max_Flash_Sequence_Size> flashSequence_;
  std::size_t flashSequenceSize_ = 0;
  std::size_t currentFlashIndex_ = 0;

  enum class State { Idle, WaitingBetweenSequences, FlashOn, FlashOff };
  State state_ = State::Idle;

  const Ms<TimeUnit> Long_Flash;
  const Ms<TimeUnit> Short_Flash;
  const Ms<TimeUnit> Pause_Between_Flashes;
  const Ms<TimeUnit> Pause_Between_Sequences;

  void resetState() {
    hasError_ = true;
    state_ = State::Idle;
    currentFlashIndex_ = 0;
    led_.write(false);
  }

  [[nodiscard]] constexpr bool isLongFlash(size_t index) const {
    if (index < flashSequenceSize_) {
      return flashSequence_[index];
    }
    return false;
  }

  void generateFlashSequence() {
    flashSequence_.reset();

    if (errorCode_ == 0) {
      flashSequenceSize_ = 1;
      return;
    }
    flashSequenceSize_ = std::bit_width(errorCode_);

    for (uint32_t i = 0; i < flashSequenceSize_; ++i) {
      if ((errorCode_ & (1u << i)) != 0) {
        flashSequence_.set(flashSequenceSize_ - i - 1);
      }
    }
  }
};

}  // namespace m

#endif  // LOG_ERROR_INDICATOR_HPP