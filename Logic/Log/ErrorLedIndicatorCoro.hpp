/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef LOG_ERROR_INDICATOR_CORO_HPP
#define LOG_ERROR_INDICATOR_CORO_HPP

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
 * being polled: `coroRun()` is awaited once and the coroutine then lives as long
 * as the object, suspending on the clock between the flashes. The state machine
 * of the synchronous twin turns into straight-line code, and no Timer is needed.
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
 * void setup() { auto task = indicator.coroRun(); }   // lives while the object does
 * void onError(Error code) { indicator.setError(code); }
 * void onCleared() { indicator.clearError(); }
 * ```
 *
 * so `Timeout` is one long flash and `Oom` (0b11) two long ones.
 *
 * `setError()` / `clearError()` may be called from anywhere — another task, an
 * interrupt, a plain function in the superloop: the coroutine picks the change
 * up within one scheduler round, even in the middle of a flash. A sequence that
 * is no longer wanted stops where it is (the long pause between sequences is not
 * started either), so what is on the LED always follows the caller.
 *
 * Notes:
 *   - `setError()` shows the newest code given to it; deciding *which* code is
 *     worth showing is the caller's business (in the firmware: FaultManager,
 *     which latches the root cause and hands the same code over every round).
 *   - `clearError()` is idempotent — with nothing to show it does not touch the
 *     pin, so it may be called every round as well.
 *   - While there is nothing to show the task is suspended with the LED dark: it
 *     costs one ready-check per scheduler round and no hardware access at all.
 *   - `coroRun()` is awaited exactly once and never returns.
 */
template <m::ifc::mcu::CPin PinT, m::ifc::CTime TimeT, EnumClassWithSize ErrorT>
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

  /// Shows `error_code`. The newest call wins: the sequence is regenerated and
  /// the display starts over, so what is shown always follows the caller — it is
  /// the caller that decides which code is worth showing (see FaultManager,
  /// which latches the root cause). A code without a single set bit (0) shows
  /// nothing and leaves the LED dark.
  ///
  /// Calling it again with the code already on display does nothing, so a task
  /// may hand the same code over every round.
  void setError(ErrorT error_code) {
    if (error_code_ == error_code) return;
    error_code_ = error_code;

    generateFlashSequence();
    restart();
  }

  [[nodiscard]] bool hasError() { return error_code_.has_value(); }

  /// Stops the indication and leaves the LED dark. Idempotent: with nothing to
  /// show it does not touch the pin, so it may be called every round as well.
  void clearError() {
    if (!error_code_) return;
    error_code_.reset();
    restart();
  }

  /// Runs the indication: await the returned task once, it then lives as long as
  /// the object does — the counterpart of calling handle() forever.
  m::Task<void> coroRun() {
    while (true) {
      led_.write(false);

      // Nothing to show (no code, or the code 0 without a single set bit): park
      // with the LED dark until setError() moves the state.
      if (!error_code_ || flash_sequence_sze_ == 0) {
        (void)co_await wait(MsT{}, generation_);
        continue;
      }

      // The generation this sequence belongs to: setError() / clearError() bump
      // it, and every wait below then ends at once — a sequence that is no
      // longer wanted stops where it is instead of being played to its end (and
      // the long pause between sequences is not started either).
      const std::size_t sequence = generation_;

      // One sequence of the flash code, MSB first: the LED is lit for the whole
      // width of the bit and the gap between the bits follows.
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

      led_.write(false);

      // The pause between two sequences of the same code — longer than the gap
      // between the bits, so the bits cannot be confused with the sequences.
      (void)co_await wait(Pause_Between_Sequences, sequence);
    }
  }

 private:
  /// Waits for `duration`, or returns earlier as soon as the state moves on from
  /// `sequence` — a clear request or a new code must be visible at once, not at
  /// the end of the current flash. `true` means the state changed on the way.
  ///
  /// Called with the current generation and no duration it only waits for such a
  /// change: that is how the task parks while there is nothing to show.
  [[nodiscard]] m::Task<bool> wait(MsT duration, std::size_t sequence) {
    const auto start = time_.now();
    co_await m::coroUntil([&] {
      return generation_ != sequence ||
             (duration.value() != 0 && time_.diff(start) >= duration);
    });
    co_return generation_ != sequence;
  }

  /// Tells the running coroutine to drop what it shows and start over.
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
  /// Bumped by setError()/clearError(): the only thing the coroutine has to look
  /// at to notice that what it is showing is no longer wanted.
  std::size_t generation_ = 0;

  std::bitset<Max_Flash_Sequence_Size> flash_sequence_;
  std::size_t flash_sequence_sze_ = 0;

  const MsT Long_Flash;
  const MsT Short_Flash;
  const MsT Pause_Between_Flashes;
  const MsT Pause_Between_Sequences;
};

}  // namespace m

#endif  // LOG_ERROR_INDICATOR_CORO_HPP
