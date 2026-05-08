/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef LOG_MIXIN_HPP
#define LOG_MIXIN_HPP

#include <ILog.hpp>
#include <cstdint>
#include <type_traits>

#ifndef LOG_LEVEL
#define LOG_LEVEL 4
#endif

// ============================================================================
// LogMixin — compile-time logging with zero overhead when disabled
// ============================================================================
//
// Inherit, wire an ILog&, call error() / warn() / info() / debug() / trace()
// from within the class.  Calls + string literals are eliminated when the
// global LOG_LEVEL or per-class ModuleLevel threshold excludes them.
//
// ModuleLevel = None → storage is empty (0 bytes), all calls dead.
//
// Example usage:
//
//   class Motor : protected m::LogMixin<m::LogLevel::Info> {
//   public:
//     Motor(m::ifc::ILog& log) : LogMixin(log) {}
//     void move() {
//       info("move start");
//       debug("step");            // compiled out (Info < Debug)
//     }
//   };
//
// CMake (production):
//   target_compile_definitions(app PRIVATE LOG_LEVEL=0)

namespace m {

enum class LogLevel : uint8_t {
  None = 0,
  Error = 1,
  Warning = 2,
  Info = 3,
  Debug = 4,
  Trace = 5
};

constexpr LogLevel Log_Level = static_cast<LogLevel>(LOG_LEVEL);

template <LogLevel ModuleLevel = Log_Level>
class LogMixin {
  static constexpr bool Active = (ModuleLevel > LogLevel::None);
  struct Nil {};
  using Storage = std::conditional_t<Active, m::ifc::ILog*, Nil>;

 protected:
  explicit constexpr LogMixin(m::ifc::ILog& log)
    requires(Active)
      : log_(&log) {}
  explicit constexpr LogMixin(m::ifc::ILog&)
    requires(!Active)
  {}

  constexpr void error(const char* m) { log<LogLevel::Error>(m); }
  constexpr void warn(const char* m) { log<LogLevel::Warning>(m); }
  constexpr void info(const char* m) { log<LogLevel::Info>(m); }
  constexpr void debug(const char* m) { log<LogLevel::Debug>(m); }
  constexpr void trace(const char* m) { log<LogLevel::Trace>(m); }

 private:
  [[no_unique_address]] Storage log_{};

  template <LogLevel L>
  constexpr void log(const char* msg) {
    if constexpr (Active && L <= Log_Level && L <= ModuleLevel) log_->add(msg);
  }
};

}  // namespace m

#endif  // LOG_MIXIN_HPP
