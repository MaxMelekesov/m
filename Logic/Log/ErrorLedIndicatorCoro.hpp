/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ERROR_LED_INDICATOR_CORO_HPP
#define ERROR_LED_INDICATOR_CORO_HPP

#include <CoroScheduler.hpp>
#include <CoroUntil.hpp>
#include <EnumClass.hpp>
#include <IPin.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <bit>
#include <bitset>
#include <cstddef>
#include <optional>

namespace m {

/**
 * @brief Coroutine twin of m::ErrorLedIndicator.
 *
 * Same code, same timings, same API — but the class drives itself instead of
 * being polled: `coroRun()` is awaited once and the coroutine then lives as
 * long as the object, suspending on the clock between the flashes. The state
 * machine of the synchronous twin turns into straight-line code, and no Timer
 * is needed.
 *
 * The code is flashed bit by bit, MSB first — a long flash for a 1 bit, a short
 * one for a 0 bit, `pause_between_flashes` between the bits of a sequence and
 * `pause_between_sequences` before the sequence repeats:
 *
 * ```cpp
 * enum class Error : uint16_t { None = 0, Timeout = 1, Oom = 3, Size };
 *
 * m::ErrorLedIndicatorCoro<m::ifc::mcu::IPin, TimeMs, Error> indicator{
 *     hw.getRedLed(), hw.getTimeMs()};
 *
 * void setup() { auto task = indicator.coroRun(); }   // lives while the object
 * does void onError(Error code) { indicator.setError(code); } void onCleared()
 * { indicator.clearError(); }
 * ```
 *
 * so `Timeout` is one long flash and `Oom` (0b11) two long ones.
 *
 * `setError()` / `clearError()` may be called from anywhere — another task, an
 * interrupt, a plain function in the superloop: the coroutine picks the change
 * up within one scheduler round, even in the middle of a flash. A sequence that
 * is no longer wanted stops where it is (the long pause between sequences is
 * not started either), so what is on the LED always follows the caller.
 *
 * Notes:
 *   - `setError()` shows the newest code given to it; deciding *which* code is
 *     worth showing is the caller's business (in the firmware: FaultManager,
 *     which latches the root cause and hands the same code over every round).
 *   - `clearError()` is idempotent — with nothing to show it does not touch the
 *     pin, so it may be called every round as well.
 *   - While there is nothing to show the task is suspended and the pin is not
 *     touched at all — one ready-check per scheduler round and no hardware
 *     access, so a lamp shared with something else (e.g. a traffic blink)
 *     keeps working.
 *   - `coroRun()` is awaited exactly once and never returns.
 */
template <m::ifc::mcu::CPin PinT, m::ifc::CTime TimeT,
          m::EnumClassWithSize ErrorT>
  requires m::ifc::CMs<typename TimeT::Unit>
class ErrorLedIndicatorCoro {
 public:
  using Error = ErrorT;
  using MsT = typename TimeT::Unit;

  ErrorLedIndicatorCoro(PinT& led, TimeT& time, MsT long_flash = MsT{1000},
                        MsT short_flash = MsT{200},
                        MsT pause_between_flashes = MsT{1000},
                        MsT pause_between_sequences = MsT{5000})
      : led_(led),
        time_(time),
        Long_Flash(long_flash),
        Short_Flash(short_flash),
        Pause_Between_Flashes(pause_between_flashes),
        Pause_Between_Sequences(pause_between_sequences) {}

  void setError(ErrorT error_code) {
    if (error_code_ == error_code) return;
    if (static_cast<std::size_t>(error_code) == 0) {
      clearError();
      return;
    }
    error_code_ = error_code;

    generateFlashSequence();
    restart();
  }

  [[nodiscard]] bool hasError() { return error_code_.has_value(); }

  void clearError() {
    if (!error_code_) return;
    error_code_.reset();
    restart();
  }

  m::Task<void> coroRun() {
    while (true) {
      if (!error_code_ || flash_sequence_sze_ == 0) {
        co_await wait(MsT{}, generation_);
        continue;
      }

      const std::size_t sequence = generation_;

      for (std::size_t i = 0; i < flash_sequence_sze_; ++i) {
        led_.write(true);
        if (co_await wait(isLongFlash(i) ? Long_Flash : Short_Flash,
                          sequence)) {
          break;
        }
        led_.write(false);
        if (co_await wait(Pause_Between_Flashes, sequence)) {
          break;
        }
      }
      co_await wait(Pause_Between_Sequences, sequence);
    }
  }

 private:
  [[nodiscard]] m::Task<bool> wait(MsT duration, std::size_t sequence) {
    const auto start = time_.now();
    co_await m::coroUntil([&] {
      return generation_ != sequence ||
             (duration.value() != 0 && time_.diff(start) >= duration);
    });
    co_return generation_ != sequence;
  }

  void restart() {
    ++generation_;
    led_.write(false);
  }

  bool isLongFlash(std::size_t index) const {
    if (index < flash_sequence_sze_) {
      return flash_sequence_[index];
    }
    return false;
  }

  void generateFlashSequence() {
    flash_sequence_.reset();

    if (!error_code_) {
      flash_sequence_sze_ = 1;
      return;
    }

    const auto value = static_cast<std::size_t>(error_code_.value());
    flash_sequence_sze_ = std::bit_width(value);

    for (std::size_t i = 0; i < flash_sequence_sze_; ++i) {
      if ((value & (std::size_t{1} << i)) != 0) {
        flash_sequence_.set(flash_sequence_sze_ - i - 1);
      }
    }
  }

  static constexpr std::size_t bitsNeeded() {
    const auto temp = std::bit_width(static_cast<std::size_t>(ErrorT::Size));
    return temp > 0 ? temp : 1;
  }
  static constexpr std::size_t Max_Flash_Sequence_Size = bitsNeeded();

  PinT& led_;
  TimeT& time_;

  std::optional<ErrorT> error_code_;
  std::size_t generation_ = 0;

  std::bitset<Max_Flash_Sequence_Size> flash_sequence_;
  std::size_t flash_sequence_sze_ = 0;

  const MsT Long_Flash;
  const MsT Short_Flash;
  const MsT Pause_Between_Flashes;
  const MsT Pause_Between_Sequences;
};

}  // namespace m

#endif  // ERROR_LED_INDICATOR_CORO_HPP
