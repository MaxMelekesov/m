/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PROTO_THREAD_HPP
#define PROTO_THREAD_HPP

#include <cstdint>
#include <utility>

namespace m {

enum class PtStatus : std::uint8_t { Running, Done };

struct PtVoid {};

namespace detail {

class PtBase {
  friend class PtSchedulerImpl;

 protected:
  static constexpr std::uint16_t Done_State = 0xFFFF;

  std::uint16_t state_ = 0;

  using RunFunc = PtStatus (*)(PtBase*);
  RunFunc run_fn_ = nullptr;

  PtBase* active_child_ = nullptr;

  void setDone() { state_ = Done_State; }

  template <typename Derived>
  static PtStatus trampoline(PtBase* base) {
    return static_cast<Derived*>(base)->run();
  }

 private:
  PtBase* next_ = nullptr;
  bool registered_ = false;

 public:
  PtStatus resume() { return run_fn_(this); }
  [[nodiscard]] bool done() const { return state_ == Done_State; }
  void reset() {
    state_ = 0;
    active_child_ = nullptr;
  }
};

class PtSchedulerImpl {
  PtBase* head_ = nullptr;
  PtBase* tail_ = nullptr;

  PtSchedulerImpl() = default;
  static PtSchedulerImpl instance_;

 public:
  static PtSchedulerImpl& getInstance() { return instance_; }

  void add(PtBase& task) {
    if (task.registered_) return;
    task.registered_ = true;
    task.next_ = nullptr;
    if (tail_) {
      tail_->next_ = &task;
    } else {
      head_ = &task;
    }
    tail_ = &task;
  }

  void remove(PtBase& task) {
    if (!task.registered_) return;
    PtBase* prev = nullptr;
    for (auto* t = head_; t; prev = t, t = t->next_) {
      if (t == &task) {
        if (prev) {
          prev->next_ = t->next_;
        } else {
          head_ = t->next_;
        }
        if (tail_ == t) {
          tail_ = prev;
        }
        t->next_ = nullptr;
        t->registered_ = false;
        return;
      }
    }
  }

  void clear() {
    for (auto* t = head_; t;) {
      auto* next = t->next_;
      t->next_ = nullptr;
      t->registered_ = false;
      t = next;
    }
    head_ = nullptr;
    tail_ = nullptr;
  }

  bool handle() {
    bool any_active = false;
    for (auto* t = head_; t; t = t->next_) {
      if (t->done()) continue;
      any_active = true;
      for (;;) {
        auto* leaf = t;
        while (leaf->active_child_ && !leaf->active_child_->done()) {
          leaf = leaf->active_child_;
        }
        auto* prev_active = leaf->active_child_;
        leaf->resume();
        if (t->done()) break;
        bool spawned =
            leaf->active_child_ && leaf->active_child_ != prev_active;
        if (!spawned && !leaf->done()) break;
      }
    }
    return any_active;
  }

  [[nodiscard]] bool allDone() const {
    for (auto* t = head_; t; t = t->next_) {
      if (!t->done()) return false;
    }
    return true;
  }
};

}  // namespace detail

template <typename Derived, typename T = void>
class Proto : public detail::PtBase {
  T result_{};

 public:
  Proto() { run_fn_ = &PtBase::trampoline<Derived>; }
  Proto(const Proto&) = delete;
  Proto& operator=(const Proto&) = delete;

  [[nodiscard]] const T& result() const { return result_; }
  T resultValue() { return std::move(result_); }

 protected:
  void ptReturn(T value) { result_ = std::move(value); }
};

template <typename Derived>
class Proto<Derived, void> : public detail::PtBase {
 public:
  Proto() { run_fn_ = &PtBase::trampoline<Derived>; }
  Proto(const Proto&) = delete;
  Proto& operator=(const Proto&) = delete;

  PtVoid resultValue() const { return {}; }
};

class PtMutex {
  bool locked_ = false;

 public:
  PtMutex() = default;
  [[nodiscard]] bool isLocked() const { return locked_; }
  [[nodiscard]] bool tryLock() {
    if (locked_) return false;
    locked_ = true;
    return true;
  }
  void unlock() { locked_ = false; }
};

using PtScheduler = detail::PtSchedulerImpl;

}  // namespace m

inline m::detail::PtSchedulerImpl m::detail::PtSchedulerImpl::instance_;

// ─── Protothread Macros ─────────────────────────────────────────

#define PT_BEGIN() \
  switch (this->state_) { \
    case 0:

#define PT_END() \
  } \
  this->setDone(); \
  return ::m::PtStatus::Done

#define PT_YIELD() \
  do { \
    this->state_ = __LINE__; \
    return ::m::PtStatus::Running; \
    case __LINE__:; \
  } while (0)

#define PT_WAIT_UNTIL(cond) \
  do { \
    this->state_ = __LINE__; \
    case __LINE__: \
    if (!(cond)) return ::m::PtStatus::Running; \
  } while (0)

#define PT_WAIT_WHILE(cond) PT_WAIT_UNTIL(!(cond))

#define PT_AWAIT(child) \
  __extension__({ \
    (child).reset(); \
    this->active_child_ = &(child); \
    this->state_ = __LINE__; \
    case __LINE__: \
    if (!(child).done()) \
      return ::m::PtStatus::Running; \
    this->active_child_ = nullptr; \
    (child).resultValue(); \
  })

#define PT_SPAWN(child) \
  do { \
    (child).reset(); \
    ::m::PtScheduler::getInstance().add(child); \
  } while (0)

#define PT_RETURN(...) \
  do { \
    __VA_OPT__(this->ptReturn(__VA_ARGS__);) \
    this->setDone(); \
    return ::m::PtStatus::Done; \
  } while (0)

#define PT_RESTART() \
  do { \
    this->state_ = 0; \
    return ::m::PtStatus::Running; \
  } while (0)

#endif  // PROTO_THREAD_HPP
