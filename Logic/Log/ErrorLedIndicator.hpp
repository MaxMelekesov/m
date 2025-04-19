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
#include <array>
#include <cstdint>

namespace m {

template <typename TimeUnit, std::size_t MaxErrorCode>
class ErrorLedIndicator {
 public:
  ErrorLedIndicator(ifc::mcu::IPin& led, ifc::ITime<Ms<TimeUnit>>& time,
                    Ms<TimeUnit> Long_Flash, Ms<TimeUnit> Short_Flash,
                    Ms<TimeUnit> Pause_Between_Flashes,
                    Ms<TimeUnit> Pause_Between_Sequences)
      : led_(led),
        time_(time),
        timer_(time),
        Long_Flash_Ms(Long_Flash),
        Short_Flash_Ms(Short_Flash),
        Pause_Between_Flashes_Ms(Pause_Between_Flashes),
        Pause_Between_Sequences_Ms(Pause_Between_Sequences) {}

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
        if (timer_.restart(Pause_Between_Sequences_Ms)) {
          state_ = State::WaitingBetweenSequences;
        }
        break;
      }
      case State::WaitingBetweenSequences: {
        if (timer_.timeOver()) {
          currentFlashIndex_ = 0;
          state_ = State::FlashOn;
          led_.write(true);
          timer_.restart(flashSequence_[currentFlashIndex_] ? Long_Flash_Ms
                                                            : Short_Flash_Ms);
        }
        break;
      }
      case State::FlashOn: {
        if (timer_.timeOver()) {
          led_.write(false);
          timer_.restart(Ms<uint32_t>{Pause_Between_Flashes_Ms});
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
            timer_.restart(flashSequence_[currentFlashIndex_] ? Long_Flash_Ms
                                                              : Short_Flash_Ms);
            state_ = State::FlashOn;
          }
        }
        break;
      }
    }
  }

 private:
  m::ifc::mcu::IPin& led_;
  m::ifc::ITime<Ms<TimeUnit>>& time_;
  m::Timer<Ms<TimeUnit>> timer_;
  const Ms<TimeUnit> Long_Flash_Ms;
  const Ms<TimeUnit> Short_Flash_Ms;
  const Ms<TimeUnit> Pause_Between_Flashes_Ms;
  const Ms<TimeUnit> Pause_Between_Sequences_Ms;

  static constexpr std::size_t bitsNeeded(uint32_t value) {
    std::size_t bits = 1;
    while (value > 1) {
      value >>= 1;
      bits++;
    }
    return bits;
  }

  static constexpr std::size_t Max_Flash_Sequence_Size =
      bitsNeeded(MaxErrorCode);

  std::array<bool, Max_Flash_Sequence_Size> flashSequence_;
  std::size_t flashSequenceSize_ = 0;
  std::size_t currentFlashIndex_ = 0;

  enum class State { Idle, WaitingBetweenSequences, FlashOn, FlashOff };

  State state_ = State::Idle;

  void resetState() {
    hasError_ = true;
    state_ = State::Idle;
    currentFlashIndex_ = 0;
    led_.write(false);
  }

  void generateFlashSequence() {
    flashSequenceSize_ = 0;

    if (errorCode_ == 0) {
      flashSequence_[0] = false;
      flashSequenceSize_ = 1;
      return;
    }

    uint32_t tempCode = errorCode_;
    uint32_t bitCount = 0;
    while (tempCode > 0) {
      tempCode >>= 1;
      bitCount++;
    }

    flashSequenceSize_ = bitCount;
    for (uint32_t i = 0; i < bitCount; ++i) {
      flashSequence_[bitCount - i - 1] = ((errorCode_ >> i) & 1) != 0;
    }
  }

  bool hasError_ = false;
  uint32_t errorCode_ = 0;
};

}  // namespace m

#endif  // LOG_ERROR_INDICATOR_HPP