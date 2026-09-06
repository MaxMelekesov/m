/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

/*
 * @example
 * // --- Driver (Tag = own class → conflict domain) ---
 * class Spi1 final : public m::mcu::Peripheral<Spi1> {
 *  public:
 *   explicit Spi1(HwKey key, SPI_Handle* h) : Peripheral(key), h_(h) {
 *     if (HAL_SPI_Init(h_) != HAL_OK) setInitFailed();  // rollback Tag
 *   }
 *   ~Spi1() { if (needsCleanup()) HAL_SPI_DeInit(h_); }   // skip if moved-from
 *   Spi1(const Spi1&) = delete;
 *   Spi1& operator=(const Spi1&) = delete;
 *   Spi1(Spi1&&) = default;                              // move → no deinit
 *   Spi1& operator=(Spi1&&) = delete;
 *  private:
 *   SPI_Handle* h_;
 * };
 *
 * // --- Two drivers sharing one pin-mux use the SAME Tag ---
 * struct Uart1Spi1PinMux {};  // Tag = conflict boundary, not driver name
 * class Uart1 final : public m::mcu::Peripheral<Uart1Spi1PinMux> { ... };
 * class Spi1Alt final : public m::mcu::Peripheral<Uart1Spi1PinMux> { ... };
 * // Only one can be active at a time.
 *
 * // --- Hardware ---
 * struct MyHW : m::mcu::Hardware<MyHW, Spi1> {};
 * auto& hw = MyHW::getInstance();
 * auto spi = hw.get<Spi1>(&hspi1);
 * if (!spi) { // Already_Taken or Init_Failed }
 */

#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <concepts>
#include <cstdint>
#include <expected>
#include <type_traits>
#include <utility>

namespace m::mcu {

enum class HwError : uint8_t {
  Already_Taken = 1,
  Init_Failed,
};

template <typename PeripheralTag>
class Peripheral;

template <typename T>
concept CPeripheral = requires {
  typename T::Tag;
  requires std::derived_from<T, Peripheral<typename T::Tag>>;
  requires std::is_nothrow_move_constructible_v<T>;
  requires !std::is_copy_constructible_v<T>;
  requires std::is_nothrow_destructible_v<T>;
};

template <typename PeripheralTag>
class Peripheral {
 public:
  class HwKey {
   public:
    HwKey(const HwKey&) = delete;
    HwKey(HwKey&&) = delete;
    HwKey& operator=(const HwKey&) = delete;
    HwKey& operator=(HwKey&&) = delete;

   private:
    HwKey() = default;
    ~HwKey() = default;

    template <typename, CPeripheral...>
    friend class Hardware;
  };

  using Tag = PeripheralTag;

  Peripheral(const Peripheral&) = delete;
  Peripheral& operator=(const Peripheral&) = delete;
  Peripheral& operator=(Peripheral&&) = delete;

  Peripheral(Peripheral&& other) noexcept
      : cleanup_(std::exchange(other.cleanup_, false)) {}

  ~Peripheral() noexcept {
    if (cleanup_) {
      release();
    }
  }

 protected:
  Peripheral(const HwKey&) { tag_held_ = true; }

  /// True unless moved-from.  Still true after setInitFailed() —
  /// partially-initialised hardware must be cleaned up.
  [[nodiscard]] bool needsCleanup() const noexcept { return cleanup_; }

  // Call in constructor when HAL init fails.
  void setInitFailed() noexcept { tag_held_ = false; }

 private:
  template <typename, CPeripheral...>
  friend class Hardware;

  static inline bool tag_held_ = false;
  bool cleanup_ = true;

  void release() {
    tag_held_ = false;
    cleanup_ = false;
  }

  [[nodiscard]] static bool isTagHeld() { return tag_held_; }
};

template <typename Derived, CPeripheral... Drivers>
class Hardware {
 public:
  static auto getInstance() -> Derived& {
    static Derived hw;
    return hw;
  }

  Hardware(const Hardware&) = delete;
  Hardware& operator=(const Hardware&) = delete;
  Hardware(Hardware&&) = delete;
  Hardware& operator=(Hardware&&) = delete;

  template <typename P, typename... Args>
    requires(std::is_same_v<P, Drivers> || ...)
  [[nodiscard]] auto get(Args&&... args) -> std::expected<P, HwError> {
    if (Peripheral<typename P::Tag>::isTagHeld()) {
      return std::unexpected{HwError::Already_Taken};
    }
    P candidate{typename P::HwKey{}, std::forward<Args>(args)...};
    // If setInitFailed() was called, tag_held_ is already false.
    if (!Peripheral<typename P::Tag>::isTagHeld()) {
      return std::unexpected{HwError::Init_Failed};
    }
    return candidate;
  }

 protected:
  Hardware() = default;
  ~Hardware() = default;
};

}  // namespace m::mcu

#endif  // HARDWARE_HPP
