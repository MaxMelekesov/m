/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MULTIPIN_HPP
#define MULTIPIN_HPP

#include <IPin.hpp>
#include <array>
#include <functional>

template <std::size_t N>
class Multipin : public m::ifc::mcu::IPin {
 public:
  template <typename... Pins>
  explicit Multipin(Pins&... pins) : pins_{std::ref(pins)...} {
    static_assert(sizeof...(Pins) == N, "Pins count must be equal to N");
  }

  void write(bool state) override {
    for (auto& pin : pins_) {
      pin.get().write(state);
    }
  }

  bool read() override {
    for (const auto& pin : pins_) {
      if (pin.get().read()) {
        return true;
      }
    }
    return false;
  }

  void toggle() override {
    for (auto& pin : pins_) {
      pin.get().toggle();
    }
  }

 private:
  std::array<std::reference_wrapper<m::ifc::mcu::IPin>, N> pins_;
};

template <typename... Pins>
Multipin(Pins&...) -> Multipin<sizeof...(Pins)>;

#endif  // MULTIPIN_HPP