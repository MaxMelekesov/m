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
 public:
  template <typename EventType>
  void processEvent() {
    using TransitionType =
        typename FindTransition<std::decay_t<decltype(currentState)>, EventType,
                                Transitions...>::type;

    static_assert(!std::is_void_v<TransitionType>);

    invokeHandleEvent<typename TransitionType::From,
                      typename TransitionType::Event>();

    setState<typename TransitionType::To>();
  }

  void checkEvents() {
    [&]<typename... Ts>(Ts...) {
      (static_cast<void>(
           checkAndProcessEvent<typename Ts::From, typename Ts::Event>()),
       ...);
    }((Transitions{})...);
  }

 private:
  StateVariant currentState;

  Fsm_v4() { currentState.template emplace<InitialState>(); }
  friend Derived;

  template <typename NewState>
  void setState() {
    static_assert(std::is_base_of_v<State<NewState>, NewState>);
    currentState.template emplace<NewState>();
  }

  template <typename FromState, typename EventType>
  void invokeHandleEvent() {
    static_cast<Derived*>(this)->handleEvent(FromState{}, EventType{});
  }

  template <typename FromState, typename EventType>
  void checkAndProcessEvent() {
    if (static_cast<Derived*>(this)->checkEvent(FromState{}, EventType{})) {
      invokeHandleEvent<FromState, EventType>();
      setState<typename FindTransition<FromState, EventType,
                                       Transitions...>::type::To>();
    }
  }
};

}  // namespace m

#endif  // FSM_V4_H