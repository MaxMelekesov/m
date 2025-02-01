/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PIN_F4_WRAPPER_H
#define PIN_F4_WRAPPER_H

#include <Pin_F4.hpp>

class PinWrapper final : public m::ifc::mcu::IPin {
 public:
  PinWrapper(GPIO_TypeDef* port, Pin::PinNum pin_num, Pin::Mode mode,
             Pin::Inversion inversion = Pin::Inversion::Not_Inverted,
             Pin::InitState init_state = Pin::InitState::Reset)
      : port_(port),
        pin_num_(pin_num),
        mode_(mode),
        inversion_(inversion),
        init_state_(init_state) {
    if (mode_ == Pin::Mode::Output_PP || mode_ == Pin::Mode::Output_OD) {
      if ((bool)init_state_ != (bool)inversion_)
        HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_SET);
      else
        HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_RESET);
    }
  }

  void write(bool state) override {
    if (state != (bool)inversion_)
      HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_RESET);
  }

  bool read() const override {
    return HAL_GPIO_ReadPin(port_, (uint32_t)pin_num_) != (bool)inversion_;
  }

  void toggle() override { HAL_GPIO_TogglePin(port_, (uint32_t)pin_num_); }

 private:
  GPIO_TypeDef* const port_;
  Pin::PinNum const pin_num_;
  Pin::Mode mode_;
  Pin::Inversion const inversion_;
  Pin::InitState const init_state_;
};

#endif  // PIN_F4_WRAPPER_H