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

/*
// Usage example:
// StateA -> Event1 -> StateB
// StateA -> Event2 -> StateC
// StateB -> Event2 -> StateC
// StateC -> Event1 -> StateA
// StateA -> ErrorEvent -> ErrorState
// StateB -> ErrorEvent -> ErrorState
// StateC -> ErrorEvent -> ErrorState

struct StateA {};
struct StateB {};
struct StateC {};

struct Event1 {};
struct Event2 {};
struct EventError {};

using MyEvents = std::tuple<Event1, Event2>;

class MyFsm : public m::Fsm_v3<MyFsm, MyEvents, StateA, StateB, StateC> {
 public:

  void handle(){
   if(...){ //Check Event2 for all states
     processEvent(Event2{});
   }
   if(...){ //Check errors for all states
     processEvent(EventError{});
   }
   checkEvents();
  }

  template <typename State>
  constexpr void handleEvent(State, ErrorEvent) {
    //Do job ...
    setState<ErrorState>();
  }

  constexpr void checkEvent(const StateA&, const Event1&) {
    if (...) {
      processEvent(Event1{});
    }
  }

  template <typename State, typename Event>
  constexpr void handleEvent(const State&, const Event&) {
    if constexpr (std::is_same_v<State, StateA> &&
                  std::is_same_v<Event, Event1>) {
      //Do job ...
      setState<StateB>();
    } else if constexpr (std::is_same_v<State, StateA> &&
                         std::is_same_v<Event, Event2>) {
      //Do job ...
      setState<StateC>();
    }else if constexpr (std::is_same_v<State, StateB> &&
                         std::is_same_v<Event, Event2>) {
      //Do job ...
      setState<StateC>();
    }
  }

  constexpr void handleEvent(StateB, Event2) {
    // Do job ...
    setState<StateC>();
  }

  constexpr void checkEvent(const StateC&, const Event1&) {
    if (...) {
      processEvent(Event1{});
    }
  }
  constexpr void handleEvent(StateC, Event1) {
    // Do job ...
    setState<StateA>();
  }
};

int main(){
  MyFsm fsm;
  while(1){
    fsm.handle();
  }
}
*/

template <typename Derived, typename EventTuple, typename... States>
class Fsm_v3 {
 protected:
  using StateVariant = std::variant<States...>;
  StateVariant currentState;

  using Events = EventTuple;

 public:
  constexpr Fsm_v3() : currentState(std::in_place_index<0>) {}

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