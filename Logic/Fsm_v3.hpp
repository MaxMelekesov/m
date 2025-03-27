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

#include <tuple>
#include <type_traits>
#include <variant>

namespace m {

template <typename Derived, typename EventTuple, typename... States>
class Fsm_v3 {
 protected:
  using StateVariant = std::variant<States...>;
  StateVariant currentState;

  using Events = EventTuple;

 public:
  constexpr Fsm_v3() = default;

  template <typename State>
  constexpr void setState() noexcept {
    currentState = State{};
  }

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

  constexpr void checkEvents() {
    std::visit(
        [&](auto&& state) {
          std::apply(
              [&](auto&&... events) {
                (static_cast<Derived*>(this)->checkEvent(state, events), ...);
              },
              Events{});
        },
        currentState);
  }

  template <typename State, typename Event>
  constexpr void onUnhandledEvent(const State&, const Event&) {}
};
}  // namespace m

#endif  // FSM_V3_H