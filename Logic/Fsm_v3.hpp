/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_V3_H
#define FSM_V3_H

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace m {

#include <type_traits>
#include <variant>

template <typename Derived, typename... States>
class Fsm_v3 {
 protected:
  using StateVariant = std::variant<States...>;
  StateVariant currentState;

  template <typename State>
  constexpr void setState() noexcept {
    currentState = State{};
  }

  template <typename State>
  [[nodiscard]] constexpr bool isCurrentState() const noexcept {
    return std::holds_alternative<State>(currentState);
  }

 public:
  constexpr Fsm_v3() = default;

  template <typename Event>
  constexpr void processEvent(const Event& event) {
    std::visit(
        [&](auto&& state) {
          if constexpr (requires {
                          static_cast<Derived*>(this)->handleEvent(state,
                                                                   event);
                        }) {
            static_cast<Derived*>(this)->handleEvent(state, event);
          } else {
            static_cast<Derived*>(this)->onUnhandledEvent(state, event);
          }
        },
        currentState);
  }

  template <typename State, typename Event>
  constexpr void onUnhandledEvent(const State&, const Event&) {}
};
}  // namespace m

#endif  // FSM_V3_H