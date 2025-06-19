/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FSM_H
#define FSM_H

#include <cstdint>
#include <optional>
#include <span>

namespace m {

class Fsm;
class IState {
 public:
  IState(std::span<IState*> transitions) : transitions_(transitions) {}

  virtual ~IState() {}
  virtual void entry() = 0;
  virtual void run() = 0;
  virtual std::optional<uint32_t> checkEvents() = 0;
  virtual void exit() = 0;

 private:
  std::span<IState*> const transitions_;

  IState* processEvent(uint32_t event) {
    if (transitions_.size() <= event) return this;
    return transitions_[event];
  }

  friend Fsm;
};

class Fsm {
 public:
  Fsm(IState* state) : state_(state) {}

  void dispatch() {
    if (first_run_) {
      state_->entry();
      first_run_ = false;
    }

    state_->run();

    if (auto event = state_->checkEvents(); event) {
      auto* next_state = state_->processEvent(event.value());
      state_->exit();
      next_state->entry();
      state_ = next_state;
    }
  }

  IState* getState() { return state_; }

  void reset(IState* state) {
    state_ = state;
    first_run_ = true;
  }

 private:
  IState* state_;
  bool first_run_ = true;
};
}  // namespace m

#endif  // FSM_H