/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_V9_HPP
#define FSM_V9_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

// Fsm_v8 plus state bodies: the machine itself runs what the current state does
// on every handle(), so the caller never asks for the state. Items are
// m::Transition (what moves the state) and m::StateBody (what a state does).
//
// class MotorController {
//  public:
//   void requestStart() { start_requested_ = true; }
//   void requestStop() { stop_requested_ = true; }
//   void setFault(bool fault) { fault_ = fault; }
//   void handle() { (void)fsm_.handle(*this); }

//   [[nodiscard]] bool isRunning() const { return fsm_.state() == State::Run; }
//   [[nodiscard]] bool isFault() const { return fsm_.state() == State::Fault; }

//  private:
//   bool start_requested_ = false;
//   bool stop_requested_ = false;
//   bool fault_ = false;
//   uint16_t duty_ = 0;

//   enum class State : uint8_t { Idle, Run, Fault, Count };
//   enum class Event : uint8_t { Start, Stop, Error, Count };

//   bool canStart(State, Event) const { return start_requested_; }
//   bool canStop(State, Event) const { return stop_requested_; }
//   bool canFault(State, Event) const { return fault_; }

//   void onStart(State, Event) { start_requested_ = false; }
//   void onStop(State, Event) { stop_requested_ = false; }
//   void onFault(State, Event) {}

//   // Bodies: what a state does on every handle() while it is active.
//   void hold(State) { duty_ = 0; }
//   void run(State) { duty_ = 100; }
//   void fault(State) { duty_ = 0; }

//   using Fsm = m::Fsm_v9<
//       State, Event, MotorController,
//       m::Transition{State::Idle, Event::Start, State::Run,
//                     &MotorController::canStart, &MotorController::onStart},
//       m::Transition{State::Run, Event::Stop, State::Idle,
//                     &MotorController::canStop, &MotorController::onStop},
//       m::Transition{State::Idle, Event::Error, State::Fault,
//                     &MotorController::canFault, &MotorController::onFault},
//       m::Transition{State::Run, Event::Error, State::Fault,
//                     &MotorController::canFault, &MotorController::onFault},
//       m::StateBody{State::Idle, &MotorController::hold},
//       m::StateBody{State::Run, &MotorController::run},
//       m::StateBody{State::Fault, &MotorController::fault}>;

//   Fsm fsm_{State::Idle};
// };

namespace m {

template <typename S, typename E, typename Ctx>
struct Transition {
  S from;
  E ev;
  S to;
  bool (Ctx::*check)(S, E);
  void (Ctx::*handle)(S, E);
  auto operator<=>(const Transition&) const = default;
};

template <typename S, typename E, typename Ctx>
Transition(S, E, S, bool (Ctx::*)(S, E), void (Ctx::*)(S, E))
    -> Transition<S, E, Ctx>;

template <typename S, typename Ctx>
struct StateBody {
  S state;
  void (Ctx::*handle)(S);
  auto operator<=>(const StateBody&) const = default;
};

template <typename S, typename Ctx>
StateBody(S, void (Ctx::*)(S)) -> StateBody<S, Ctx>;

template <typename S, typename E, typename Ctx, auto... Items>
  requires(std::is_enum_v<S> && std::is_enum_v<E> &&
           requires {
             S::Count;
             E::Count;
           })
class Fsm_v9 {
  template <auto Item>
  static constexpr bool Is_Transition =
      std::is_same_v<std::remove_cvref_t<decltype(Item)>,
                     Transition<S, E, Ctx>>;

  template <auto Item>
  static constexpr bool Is_StateBody =
      std::is_same_v<std::remove_cvref_t<decltype(Item)>, StateBody<S, Ctx>>;

  static_assert(((Is_Transition<Items> || Is_StateBody<Items>) && ...),
                "items must be m::Transition or m::StateBody");

  static constexpr std::size_t Transition_Count =
      ((Is_Transition<Items> ? 1U : 0U) + ... + 0U);
  static constexpr std::size_t StateBody_Count =
      ((Is_StateBody<Items> ? 1U : 0U) + ... + 0U);
  static constexpr std::size_t State_Count = static_cast<std::size_t>(S::Count);
  static constexpr std::size_t Event_Count = static_cast<std::size_t>(E::Count);

  using state_index_t = std::conditional_t<
      (State_Count <=
       static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())),
      std::uint8_t,
      std::conditional_t<(State_Count <=
                          static_cast<std::size_t>(
                              std::numeric_limits<std::uint16_t>::max())),
                         std::uint16_t, std::uint32_t>>;

  using event_index_t = std::conditional_t<
      (Event_Count <=
       static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())),
      std::uint8_t,
      std::conditional_t<(Event_Count <=
                          static_cast<std::size_t>(
                              std::numeric_limits<std::uint16_t>::max())),
                         std::uint16_t, std::uint32_t>>;

  using transition_index_t = std::conditional_t<
      (Transition_Count <=
       static_cast<std::size_t>(std::numeric_limits<std::uint8_t>::max())),
      std::uint8_t,
      std::conditional_t<(Transition_Count <=
                          static_cast<std::size_t>(
                              std::numeric_limits<std::uint16_t>::max())),
                         std::uint16_t, std::uint32_t>>;

  static constexpr transition_index_t Npos =
      std::numeric_limits<transition_index_t>::max();

  struct Node {
    E ev;
    S to;
    transition_index_t next;
    bool (Ctx::*check)(S, E);
    void (Ctx::*handle)(S, E);
  };

  struct IndexData {
    std::array<transition_index_t, State_Count> head;
    std::array<Node, Transition_Count> nodes;
    std::array<void (Ctx::*)(S), State_Count> bodies;
  };

  [[noreturn]] static consteval void failEnumOutOfRange() { __builtin_trap(); }

  [[noreturn]] static consteval void failDuplicateStateEvent() {
    __builtin_trap();
  }

  [[noreturn]] static consteval void failStateCoverage() { __builtin_trap(); }

  [[noreturn]] static consteval void failEventCoverage() { __builtin_trap(); }

  [[noreturn]] static consteval void failDuplicateStateBody() {
    __builtin_trap();
  }

  [[noreturn]] static consteval void failStateBodyCoverage() {
    __builtin_trap();
  }

  static constexpr IndexData index_ = []() consteval {
    IndexData data{};
    data.head.fill(Npos);
    data.bodies.fill(nullptr);

    std::array<transition_index_t, State_Count> tail{};
    tail.fill(Npos);

    std::array<S, Transition_Count * 2> states_used{};
    std::size_t states_count = 0;
    std::array<E, Transition_Count> events_used{};
    std::size_t events_count = 0;

    std::array<std::array<bool, Event_Count>, State_Count> seen{};

    auto addState = [&](S state) {
      for (std::size_t i = 0; i < states_count; ++i) {
        if (states_used[i] == state) return;
      }
      states_used[states_count++] = state;
    };

    auto addEvent = [&](E event) {
      for (std::size_t i = 0; i < events_count; ++i) {
        if (events_used[i] == event) return;
      }
      events_used[events_count++] = event;
    };

    transition_index_t next_node = 0;
    auto addItem = [&](auto item) {
      using T = std::remove_cvref_t<decltype(item)>;
      if constexpr (std::is_same_v<T, Transition<S, E, Ctx>>) {
        const auto from = static_cast<std::size_t>(item.from);
        const auto to = static_cast<std::size_t>(item.to);
        const auto ev = static_cast<std::size_t>(item.ev);

        if (from >= State_Count || to >= State_Count || ev >= Event_Count)
          failEnumOutOfRange();

        if (seen[from][ev]) failDuplicateStateEvent();
        seen[from][ev] = true;

        data.nodes[next_node] =
            Node{item.ev, item.to, Npos, item.check, item.handle};

        if (data.head[from] == Npos) {
          data.head[from] = next_node;
        } else {
          data.nodes[tail[from]].next = next_node;
        }
        tail[from] = next_node;
        ++next_node;

        addState(item.from);
        addState(item.to);
        addEvent(item.ev);
      } else {
        const auto state = static_cast<std::size_t>(item.state);
        if (state >= State_Count) failEnumOutOfRange();
        if (data.bodies[state] != nullptr) failDuplicateStateBody();
        data.bodies[state] = item.handle;
      }
    };
    (addItem(Items), ...);

    if (states_count != State_Count) failStateCoverage();
    if (events_count != Event_Count) failEventCoverage();
    if constexpr (StateBody_Count > 0) {
      for (const auto& body : data.bodies) {
        if (body == nullptr) failStateBodyCoverage();
      }
    }

    return data;
  }();

  S state_;
  static constexpr bool _validated = (index_.head[0], true);

 public:
  explicit Fsm_v9(S init) noexcept : state_{init} {}

  /// Runs at most one transition, then the body of the (new) state.
  bool handle(Ctx& ctx) {
    auto idx = index_.head[static_cast<state_index_t>(state_)];
    while (idx != Npos) {
      const auto& n = index_.nodes[idx];
      if ((ctx.*n.check)(state_, n.ev)) {
        (ctx.*n.handle)(state_, n.ev);
        state_ = n.to;
        runBody(ctx);
        return true;
      }
      idx = n.next;
    }
    runBody(ctx);
    return false;
  }

  [[nodiscard]] constexpr S state() const noexcept { return state_; }

 private:
  void runBody(Ctx& ctx) {
    if constexpr (StateBody_Count > 0) {
      (ctx.*index_.bodies[static_cast<std::size_t>(state_)])(state_);
    } else {
      (void)ctx;
    }
  }
};

}  // namespace m

#endif  // FSM_V9_HPP
