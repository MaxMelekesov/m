#ifndef FSM_V4_H
#define FSM_V4_H

#include <concepts>
#include <tuple>
#include <type_traits>
#include <variant>

namespace m {

// Базовые структуры для состояний и событий
template <typename Derived>
struct State {};

template <typename Derived>
struct Event {};

// Определение перехода между состояниями
template <typename FromState, typename EventType, typename ToState>
struct Transition {
  using From = FromState;
  using Event = EventType;
  using To = ToState;
};

// Концепт для проверки совпадения перехода
template <typename TransitionType, typename CurrentState, typename EventType>
concept IsMatch = std::is_same_v<typename TransitionType::From, CurrentState> &&
                  std::is_same_v<typename TransitionType::Event, EventType>;

// Поиск подходящего перехода
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
  using type = void;  // Нет подходящего перехода
};

// Главный класс FSM
template <typename Derived, typename StateVariant, typename EventVariant,
          typename InitialState, typename... Transitions>
class Fsm_v4 {
 private:
  StateVariant currentState;

  // Установка нового состояния с логированием
  template <typename NewState>
  void setState() {
    static_assert(std::is_base_of_v<State<NewState>, NewState>,
                  "NewState must inherit from State<NewState>");

    // Логирование перед переходом
    if constexpr (requires {
                    static_cast<Derived*>(this)->onStateTransition(
                        std::decay_t<decltype(currentState)>(), NewState{});
                  }) {
      static_cast<Derived*>(this)->onStateTransition(
          std::decay_t<decltype(currentState)>(), NewState{});
    }

    // Переход в новое состояние
    currentState.template emplace<NewState>();
  }

  // Вызов handleEvent через CRTP
  template <typename FromState, typename EventType>
  void invokeHandleEvent() {
    static_cast<Derived*>(this)->handleEvent(FromState{}, EventType{});
  }

  friend Derived;  // Дружба с производным классом

  // Инициализация начального состояния
  Fsm_v4() { currentState.template emplace<InitialState>(); }

 public:
  // Обработка события
  void processEvent(const EventVariant& event) {
    std::visit(
        [this](auto&& e) {
          using EventType = std::decay_t<decltype(e)>;
          using TransitionType =
              typename FindTransition<std::decay_t<decltype(currentState)>,
                                      EventType, Transitions...>::type;

          static_assert(
              !std::is_void_v<TransitionType>,
              "No matching transition found for the current state and event");

          // Вызов обработчика события
          invokeHandleEvent<typename TransitionType::From, EventType>();

          // Переход в новое состояние
          setState<typename TransitionType::To>();
        },
        event);
  }

  // Проверка текущего состояния
  template <typename TargetState>
  bool isInState() const {
    return std::holds_alternative<TargetState>(currentState);
  }

  // Проверка всех возможных событий
  void checkEvents() {
    [&]<typename... Ts>(Ts...) {
      (static_cast<void>(
           checkAndProcessEvent<typename Ts::From, typename Ts::Event>()),
       ...);
    }((Transitions{})...);
  }

 private:
  // Проверка и обработка события
  template <typename FromState, typename EventType>
  void checkAndProcessEvent() {
    // Проверяем наличие метода checkEvent в производном классе
    if constexpr (requires {
                    static_cast<Derived*>(this)->checkEvent(FromState{},
                                                            EventType{});
                  }) {
      if (static_cast<Derived*>(this)->checkEvent(FromState{}, EventType{})) {
        invokeHandleEvent<FromState, EventType>();
        setState<typename FindTransition<FromState, EventType,
                                         Transitions...>::type::To>();
      }
    }
  }
};
}  // namespace m

#endif  // FSM_V4_H