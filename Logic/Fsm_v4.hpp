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

#include <tuple>
#include <type_traits>
#include <variant>

namespace m {
// Базовый класс для состояний
template <typename Derived>
struct State {
  // Пустой базовый класс для идентификации состояний
};

// Базовый класс для событий
template <typename Derived>
struct Event {
  // Пустой базовый класс для идентификации событий
};

// Класс для описания переходов
template <typename FromState, typename EventType, typename ToState>
struct Transition {
  using From = FromState;
  using Event = EventType;
  using To = ToState;
};

// Концепт для проверки совпадения состояния и события
template <typename TransitionType, typename CurrentState, typename EventType>
concept IsMatch = std::is_same_v<typename TransitionType::From, CurrentState> &&
                  std::is_same_v<typename TransitionType::Event, EventType>;

// Вспомогательный метапрограммный код для поиска перехода
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
  using type = void;  // Переход не найден
};

// Основной класс конечного автомата
template <typename Derived, typename InitialState, typename... Transitions>
class Fsm_v4 {
 private:
  // Текущее состояние представлено как тип
  template <typename CurrentState>
  struct StateWrapper {
    using State = CurrentState;
  };

  // Текущее состояние
  StateWrapper<InitialState> currentState;

  // Установка нового состояния
  template <typename NewState>
  void setState() {
    static_assert(std::is_base_of_v<State<NewState>, NewState>,
                  "NewState must inherit from State");
    currentState = StateWrapper<NewState>{};
  }

 protected:
  // Вызов handleEvent через CRTP
  template <typename FromState, typename EventType>
  void invokeHandleEvent() {
    static_cast<Derived*>(this)->handleEvent(FromState{}, EventType{});
  }

 public:
  // Конструктор
  Fsm_v4() = default;

  // Обработка события
  template <typename EventType>
  void processEvent() {
    using TransitionType =
        typename FindTransition<typename decltype(currentState)::State,
                                EventType, Transitions...>::type;

    static_assert(!std::is_void_v<TransitionType>,
                  "No transition found for the given event and current state");

    // Выполнение действия
    invokeHandleEvent<typename TransitionType::From,
                      typename TransitionType::Event>();

    // Переход в новое состояние
    setState<typename TransitionType::To>();
  }

  // Метод для проверки текущего состояния
  template <typename TargetState>
  bool isInState() const {
    return std::is_same_v<typename decltype(currentState)::State, TargetState>;
  }
};

}  // namespace m

#endif  // FSM_V4_H