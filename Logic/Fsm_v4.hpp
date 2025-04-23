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
#include <type_traits>
#include <variant>

namespace m {

struct State {};
struct Event {};

template <typename T>
concept CState = std::is_base_of_v<State, T>;

template <typename T>
concept CEvent = std::is_base_of_v<Event, T>;

template <CState FromState, CEvent EventType, CState ToState>
struct Transition {
  using From = FromState;
  using Event = EventType;
  using To = ToState;
};

template <typename T>
concept CTransition =
    requires {
      typename T::From;
      typename T::Event;
      typename T::To;
    } && CState<typename T::From> && CEvent<typename T::Event> &&
    CState<typename T::To>;

namespace {
template <typename... Ts>
struct collect_types;

template <>
struct collect_types<> {
  using states = std::tuple<>;
  using events = std::tuple<>;
};

template <typename First, typename... Rest>
struct collect_types<First, Rest...> {
 private:
  using rest_states = typename collect_types<Rest...>::states;
  using rest_events = typename collect_types<Rest...>::events;

  template <typename T, typename Tuple>
  struct add_unique;

  template <typename T, typename... Ts>
  struct add_unique<T, std::tuple<Ts...>> {
    using type = std::conditional_t<(std::is_same_v<T, Ts> || ...),
                                    std::tuple<Ts...>, std::tuple<T, Ts...>>;
  };

 public:
  using states = typename add_unique<
      typename First::From,
      typename add_unique<typename First::To, rest_states>::type>::type;

  using events = typename add_unique<typename First::Event, rest_events>::type;
};

template <typename Tuple>
struct tuple_to_variant;

template <typename... Ts>
struct tuple_to_variant<std::tuple<Ts...>> {
  using type = std::variant<Ts...>;
};

template <typename CurrentState, typename EventType, typename... Transitions>
struct FindTransition;

template <typename CurrentState, typename EventType, typename First,
          typename... Rest>
struct FindTransition<CurrentState, EventType, First, Rest...> {
  using type = std::conditional_t<
      std::is_same_v<typename First::From, CurrentState> &&
          std::is_same_v<typename First::Event, EventType>,
      First, typename FindTransition<CurrentState, EventType, Rest...>::type>;
};

template <typename CurrentState, typename EventType>
struct FindTransition<CurrentState, EventType> {
  using type = void;
};

}  // namespace

template <typename Derived, CState InitialState, CTransition... Transitions>
class Fsm_v4 {
 private:
  using TransitionsTypes = collect_types<Transitions...>;
  using StateVariant =
      typename tuple_to_variant<typename TransitionsTypes::states>::type;
  using EventVariant =
      typename tuple_to_variant<typename TransitionsTypes::events>::type;

 public:
  template <CEvent EventType>
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

  template <CState TargetState>
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
  StateVariant currentState;

  Fsm_v4() { currentState.template emplace<InitialState>(); }

  template <typename NewState>
  void setState() {
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

  friend Derived;
};
}  // namespace m

#endif  // FSM_V4_H