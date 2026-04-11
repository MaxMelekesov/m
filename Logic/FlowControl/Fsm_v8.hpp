/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_V8_HPP
#define FSM_V8_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

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

//   enum class State : uint8_t { Idle, Run, Fault, Count };
//   enum class Event : uint8_t { Start, Stop, Error, Count };

//   bool canStart(State, Event) const { return start_requested_; }
//   bool canStop(State, Event) const { return stop_requested_; }
//   bool canFault(State, Event) const { return fault_; }

//   void onStart(State, Event) { start_requested_ = false; }
//   void onStop(State, Event) { stop_requested_ = false; }
//   void onFault(State, Event) {}

//   using Fsm = m::Fsm_v8<
//       State, Event, MotorController,
//       m::Transition{State::Idle, Event::Start, State::Run,
//                     &MotorController::canStart, &MotorController::onStart},
//       m::Transition{State::Run, Event::Stop, State::Idle,
//                     &MotorController::canStop, &MotorController::onStop},
//       m::Transition{State::Idle, Event::Error, State::Fault,
//                     &MotorController::canFault, &MotorController::onFault},
//       m::Transition{State::Run, Event::Error, State::Fault,
//                     &MotorController::canFault, &MotorController::onFault}>;

//   Fsm fsm_{State::Idle};
// };

namespace m {

template <typename S, typename E, typename Ctx>
struct Transition {
  S from;
  E ev;
  S to;
  bool (Ctx::*check)(S, E) const;
  void (Ctx::*handle)(S, E);
  auto operator<=>(const Transition&) const = default;
};

template <typename S, typename E, typename Ctx>
Transition(S, E, S, bool (Ctx::*)(S, E) const, void (Ctx::*)(S, E))
    -> Transition<S, E, Ctx>;

template <typename S, typename E, typename Ctx, Transition<S, E, Ctx>... Ts>
  requires(std::is_enum_v<S> && std::is_enum_v<E> &&
           requires {
             S::Count;
             E::Count;
           })
class Fsm_v8 {
  static constexpr std::size_t Transition_Count = sizeof...(Ts);
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
    bool (Ctx::*check)(S, E) const;
    void (Ctx::*handle)(S, E);
  };

  struct IndexData {
    std::array<transition_index_t, State_Count> head;
    std::array<Node, Transition_Count> nodes;
  };

  [[noreturn]] static consteval void failEnumOutOfRange() { __builtin_trap(); }

  [[noreturn]] static consteval void failDuplicateStateEvent() {
    __builtin_trap();
  }

  [[noreturn]] static consteval void failStateCoverage() { __builtin_trap(); }

  [[noreturn]] static consteval void failEventCoverage() { __builtin_trap(); }

  static constexpr IndexData index_ = []() consteval {
    IndexData data{};
    data.head.fill(Npos);

    std::array<transition_index_t, State_Count> tail{};
    tail.fill(Npos);

    std::array<S, Transition_Count * 2> states_used{};
    std::size_t states_count = 0;
    std::array<E, Transition_Count> events_used{};
    std::size_t events_count = 0;

    std::array<std::array<bool, Event_Count>, State_Count> seen{};

    constexpr std::array<Transition<S, E, Ctx>, Transition_Count> raw{Ts...};

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

    for (std::size_t i = 0; i < raw.size(); ++i) {
      const auto& t = raw[i];
      const auto from = static_cast<std::size_t>(t.from);
      const auto to = static_cast<std::size_t>(t.to);
      const auto ev = static_cast<std::size_t>(t.ev);

      if (from >= State_Count || to >= State_Count || ev >= Event_Count)
        failEnumOutOfRange();

      if (seen[from][ev]) failDuplicateStateEvent();
      seen[from][ev] = true;

      data.nodes[i] = Node{t.ev, t.to, Npos, t.check, t.handle};

      const auto idx = static_cast<transition_index_t>(i);
      if (data.head[from] == Npos) {
        data.head[from] = idx;
      } else {
        data.nodes[tail[from]].next = idx;
      }
      tail[from] = idx;

      addState(t.from);
      addState(t.to);
      addEvent(t.ev);
    }

    if (states_count != State_Count) failStateCoverage();
    if (events_count != Event_Count) failEventCoverage();

    return data;
  }();

  S state_;
  static constexpr bool _validated = (index_.head[0], true);

 public:
  explicit Fsm_v8(S init) noexcept : state_{init} {}

  bool handle(Ctx& ctx) {
    auto idx = index_.head[static_cast<state_index_t>(state_)];
    while (idx != Npos) {
      const auto& n = index_.nodes[idx];
      if ((ctx.*n.check)(state_, n.ev)) {
        state_ = n.to;
        (ctx.*n.handle)(state_, n.ev);
        return true;
      }
      idx = n.next;
    }
    return false;
  }

  [[nodiscard]] constexpr S state() const noexcept { return state_; }
};

}  // namespace m

#endif  // FSM_V8_HPP