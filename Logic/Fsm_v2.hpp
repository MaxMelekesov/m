/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_2_H
#define FSM_2_H

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace m {

template <typename State, typename Event>
class Fsm_2 {
 public:
  using CheckEvents_Cb = std::function<std::optional<Event>()>;
  using ProcessEvent_Cb = std::function<State(Event)>;

  Fsm_2(State state,
        std::array<CheckEvents_Cb, static_cast<std::size_t>(State::Size)>&&
            check_events,
        std::array<ProcessEvent_Cb, static_cast<std::size_t>(State::Size)>&&
            process_event)
      : state_(state),
        check_events_(std::move(check_events)),
        process_event_(std::move(process_event)) {}

  void dispatch() {
    if (auto event = check_events_[static_cast<std::size_t>(state_)](); event) {
      auto next_state =
          process_event_[static_cast<std::size_t>(state_)](event.value());
      state_ = next_state;
    }
  }

  State getState() { return state_; }
  void reset(State state) { state_ = state; }

 private:
  State state_;
  const std::array<CheckEvents_Cb, static_cast<std::size_t>(State::Size)>
      check_events_;
  const std::array<ProcessEvent_Cb, static_cast<std::size_t>(State::Size)>
      process_event_;
};
}  // namespace m

#endif  // FSM_2_H