/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_V4_H
#define FSM_V4_H

#include <concepts>
#include <tuple>
#include <type_traits>
#include <variant>

namespace m {

template <typename Derived>
struct State {};

template <typename Derived>
struct Event {};

template <typename FromState, typename EventType, typename ToState>
struct Transition {
  using From = FromState;
  using Event = EventType;
  using To = ToState;
};

template <typename TransitionType, typename CurrentState, typename EventType>
concept IsMatch = std::is_same_v<typename TransitionType::From, CurrentState> &&
                  std::is_same_v<typename TransitionType::Event, EventType>;

template <typename CurrentState, typename EventType, typename... Transitions>
struct FindTransition;

template <typename CurrentState, typename EventType, typename First,
          typename... Rest>
struct FindTransition<CurrentState, EventType, First, Rest...> {
  using type = std::conditional_t<
      IsMatch<First, CurrentState, EventType>, First,
      typename FindTransition<CurrentState, EventType, Rest...>::type>;
};

template <typename CurrentState, typename EventType>
struct FindTransition<CurrentState, EventType> {
  using type = void;
};

template <typename Derived, typename StateVariant, typename EventVariant,
          typename InitialState, typename... Transitions>
class Fsm_v4 {
 private:
  StateVariant currentState;

  template <typename NewState>
  void setState() {
    static_assert(std::is_base_of_v<State<NewState>, NewState>,
                  "NewState must inherit from State<NewState>");

    if constexpr (requires {
                    static_cast<Derived*>(this)->onStateTransition(NewState{});
                  }) {
      static_cast<Derived*>(this)->onStateTransition(NewState{});
    }

    currentState.template emplace<NewState>();
  }

  template <typename FromState, typename EventType>
  void invokeHandleEvent() {
    if constexpr (requires {
                    static_cast<Derived*>(this)->handleEvent(FromState{},
                                                             EventType{});
                  }) {
      static_cast<Derived*>(this)->handleEvent(FromState{}, EventType{});
    }
  }

  friend Derived;

  Fsm_v4() { currentState.template emplace<InitialState>(); }

 public:
  template <typename EventType>
  bool processEvent(const EventType& event) {
    using TransitionType =
        typename FindTransition<std::decay_t<decltype(currentState)>, EventType,
                                Transitions...>::type;

    if constexpr (std::is_void_v<TransitionType>) {
      return false;
    } else {
      if constexpr (requires {
                      static_cast<Derived*>(this)->onEvent(EventType{});
                    }) {
        static_cast<Derived*>(this)->onEvent(EventType{});
      }

      invokeHandleEvent<typename TransitionType::From, EventType>();
      setState<typename TransitionType::To>();
      return true;
    }
  }

  void processEvent(const EventVariant& event) {
    std::visit([this](auto&& e) { processEvent(e); }, event);
  }

  template <typename TargetState>
  bool isInState() const {
    return std::holds_alternative<TargetState>(currentState);
  }

  void checkEvents() {
    [&]<typename... Ts>(Ts...) {
      ((std::holds_alternative<typename Ts::From>(currentState) &&
        checkAndProcessEvent<typename Ts::From, typename Ts::Event>()) ||
       ...);
    }((Transitions{})...);
  }

 private:
  template <typename FromState, typename EventType>
  bool checkAndProcessEvent() {
    if constexpr (requires {
                    static_cast<Derived*>(this)->checkEvent(FromState{},
                                                            EventType{});
                  }) {
      if (static_cast<Derived*>(this)->checkEvent(FromState{}, EventType{})) {
        if constexpr (requires {
                        static_cast<Derived*>(this)->onEvent(EventType{});
                      }) {
          static_cast<Derived*>(this)->onEvent(EventType{});
        }

        invokeHandleEvent<FromState, EventType>();
        setState<typename FindTransition<FromState, EventType,
                                         Transitions...>::type::To>();
        return true;
      }
    }
    return false;
  }
};
}  // namespace m

#endif  // FSM_V4_H