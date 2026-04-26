// #pragma once
// #include <meta>
// #include <array>
// #include <concepts>
// #include <algorithm>

// namespace fsm {

// template<typename S, typename E, typename Ctx>
// struct transition {
//     S from; E ev; S to;
//     bool (Ctx::*check)(S, E) const;
//     void (Ctx::*handle)(S, E);
//     auto operator<=>(const transition&) const = default;
// };

// template<typename S, typename E, typename Ctx, transition<S,E,Ctx>... Ts>
// requires (std::is_enum_v<S> && std::is_enum_v<E>)
// class machine {
//     static constexpr std::array table_{Ts...};
//     S state_;

//     // Выполняется строго при компиляции
//     static consteval void validate() {
//         auto arr = table_;
//         std::ranges::sort(arr);
//         for (std::size_t i = 1; i < arr.size(); ++i)
//             if (arr[i-1].from == arr[i].from && arr[i-1].ev == arr[i].ev)
//                 throw std::logic_error("Duplicate (State, Event)
//                 transition");

//         auto valid = []<typename T>(T v) {
//             for (auto e : std::meta::enum_values(std::meta::reflect<T>()))
//                 if (std::meta::enum_value<T>(e) == v) return true;
//             return false;
//         };
//         for (const auto& t : table_)
//             if (!valid(t.from) || !valid(t.to) || !valid(t.ev))
//                 throw std::logic_error("Invalid enum value in transition
//                 table");
//     }
//     static constexpr bool _validated = (validate(), true);

// public:
//     explicit machine(S init) noexcept : state_{init} {}

//     // Runtime-выполнение: порядок переходов сохраняется
//     bool handle(Ctx& ctx, E ev) {
//         for (const auto& t : table_) {
//             if (t.from == state_ && t.ev == ev) {
//                 if ((ctx.*t.check)(state_, ev)) {
//                     state_ = t.to;
//                     (ctx.*t.handle)(state_, ev);
//                     return true;
//                 }
//                 return false; // checkEvent отклонил событие
//             }
//         }
//         return false; // переход не определён
//     }
//     S state() const noexcept { return state_; }
// };

// } // namespace fsm

// // === Интеграция в класс ===
// enum class State : uint8_t { Idle, Moving, Blocked };
// enum class Event : uint8_t { Move, Obstacle, Clear };

// class Robot {
//     int battery = 100; // обычное runtime-поле
// public:
//     using Fsm = fsm::machine<State, Event, Robot,
//         fsm::transition{State::Idle,    Event::Move,      State::Moving,
//         &Robot::can_move,      &Robot::on_move},
//         fsm::transition{State::Moving,  Event::Obstacle,  State::Blocked,
//         &Robot::can_block,     &Robot::on_block},
//         fsm::transition{State::Blocked, Event::Clear,     State::Idle,
//         &Robot::can_clear,     &Robot::on_clear}
//     >;
//     Fsm fsm{State::Idle};

//     bool can_move(State, Event) const { return battery > 0; }
//     void on_move(State, Event) { battery -= 5; }

//     bool can_block(State, Event) const { return true; }
//     void on_block(State, Event) { /* alarm */ }

//     bool can_clear(State, Event) const { return battery > 20; }
//     void on_clear(State, Event) { battery -= 10; }

//     void dispatch(Event ev) { fsm.handle(*this, ev); }
// };