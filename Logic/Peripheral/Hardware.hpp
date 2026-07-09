/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <optional>
#include <type_traits>
#include <utility>

namespace m::mcu {

template <typename PeripheralTag>
class Peripheral;

template <typename T>
concept CPeripheral = requires {
  typename T::Tag;
  requires std::is_base_of_v<Peripheral<typename T::Tag>, T>;
  requires std::is_move_constructible_v<T>;
  requires !std::is_copy_constructible_v<T>;
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

  Peripheral(Peripheral&& other) noexcept
      : active_(std::exchange(other.active_, false)) {}

  Peripheral& operator=(Peripheral&& other) noexcept {
    if (this != &other) {
      if (active_) {
        release();
      }
      active_ = std::exchange(other.active_, false);
    }
    return *this;
  }

  virtual ~Peripheral() {
    if (active_) {
      release();
    }
  }

  [[nodiscard]] explicit operator bool() const { return active_; }

  [[nodiscard]] static bool isTaken() { return taken_; }

 protected:
  Peripheral(const HwKey&) { taken_ = true; }

 private:
  static inline bool taken_ = false;
  bool active_ = true;

  void release() {
    taken_ = false;
    active_ = false;
  }
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
  [[nodiscard]] auto get(Args&&... args) -> std::optional<P> {
    if (Peripheral<typename P::Tag>::isTaken()) {
      return std::nullopt;
    }
    return P{typename P::HwKey{}, std::forward<Args>(args)...};
  }

 protected:
  Hardware() = default;
  ~Hardware() = default;
};

}  // namespace m::mcu

#endif  // HARDWARE_HPP
