/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PIN_G4_HPP
#define PIN_G4_HPP

#include <IPin.hpp>
#include <cstdint>

#include "stm32g4xx_hal_gpio.h"

class GpioRcc {
 public:
  explicit GpioRcc(GPIO_TypeDef* port) : port_(port) {
    auto& cnt = counter(port_);
    if (++cnt == 1) enableClock(port_);
  }
  ~GpioRcc() {
    auto& cnt = counter(port_);
    if (--cnt == 0) disableClock(port_);
  }

 private:
  GPIO_TypeDef* const port_;

  static uint32_t& counter(GPIO_TypeDef* port) {
    static uint32_t a = 0, b = 0, c = 0, d = 0, f = 0, g = 0;
    if (port == GPIOA) return a;
    if (port == GPIOB) return b;
    if (port == GPIOC) return c;
    if (port == GPIOD) return d;
    if (port == GPIOF) return f;
    if (port == GPIOG) return g;
    static uint32_t dummy = 0;
    return dummy;
  }

  static void enableClock(GPIO_TypeDef* port) {
    if (port == GPIOA)
      __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
      __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
      __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
      __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOF)
      __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG)
      __HAL_RCC_GPIOG_CLK_ENABLE();
  }

  static void disableClock(GPIO_TypeDef* port) {
    if (port == GPIOA)
      __HAL_RCC_GPIOA_CLK_DISABLE();
    else if (port == GPIOB)
      __HAL_RCC_GPIOB_CLK_DISABLE();
    else if (port == GPIOC)
      __HAL_RCC_GPIOC_CLK_DISABLE();
    else if (port == GPIOD)
      __HAL_RCC_GPIOD_CLK_DISABLE();
    else if (port == GPIOF)
      __HAL_RCC_GPIOF_CLK_DISABLE();
    else if (port == GPIOG)
      __HAL_RCC_GPIOF_CLK_DISABLE();
    else if (port == GPIOG)
      __HAL_RCC_GPIOG_CLK_DISABLE();
  }
};

class Pin final : public m::ifc::mcu::IPin {
 public:
  enum class PinNum : uint16_t {
    Pin_0 = GPIO_PIN_0,
    Pin_1 = GPIO_PIN_1,
    Pin_2 = GPIO_PIN_2,
    Pin_3 = GPIO_PIN_3,
    Pin_4 = GPIO_PIN_4,
    Pin_5 = GPIO_PIN_5,
    Pin_6 = GPIO_PIN_6,
    Pin_7 = GPIO_PIN_7,
    Pin_8 = GPIO_PIN_8,
    Pin_9 = GPIO_PIN_9,
    Pin_10 = GPIO_PIN_10,
    Pin_11 = GPIO_PIN_11,
    Pin_12 = GPIO_PIN_12,
    Pin_13 = GPIO_PIN_13,
    Pin_14 = GPIO_PIN_14,
    Pin_15 = GPIO_PIN_15,
    Pin_All = GPIO_PIN_All
  };

  enum class Mode : uint32_t {
    Input = GPIO_MODE_INPUT,
    Output_PP = GPIO_MODE_OUTPUT_PP,
    Output_OD = GPIO_MODE_OUTPUT_OD,
    AF_PP = GPIO_MODE_AF_PP,
    AF_OD = GPIO_MODE_AF_OD,
    Analog = GPIO_MODE_ANALOG,
    IT_Rising = GPIO_MODE_IT_RISING,
    IT_Falling = GPIO_MODE_IT_FALLING,
    IT_Rising_Falling = GPIO_MODE_IT_RISING_FALLING,
    EVT_Rising = GPIO_MODE_EVT_RISING,
    EVT_Falling = GPIO_MODE_EVT_FALLING,
    EVT_Rising_Falling = GPIO_MODE_EVT_RISING_FALLING
  };

  enum class Pull : uint32_t {
    Nopull = GPIO_NOPULL,
    Pullup = GPIO_PULLUP,
    Pulldown = GPIO_PULLDOWN
  };

  enum class Speed : uint32_t {
    Freq_Low = GPIO_SPEED_FREQ_LOW,
    Freq_Medium = GPIO_SPEED_FREQ_MEDIUM,
    Freq_High = GPIO_SPEED_FREQ_HIGH,
    Freq_Very_High = GPIO_SPEED_FREQ_VERY_HIGH
  };

  enum class InitState : bool { Reset = 0, Set };
  enum class Inversion : bool { Not_Inverted = 0, Inverted };

 public:
  Pin(GPIO_TypeDef* port, PinNum pin_num, Mode mode, Pull pull, Speed speed,
      Inversion inversion = Inversion::Not_Inverted,
      InitState init_state = InitState::Reset, uint32_t alternate = 0)
      : port_(port),
        pin_num_(pin_num),
        mode_(mode),
        pull_(pull),
        speed_(speed),
        inversion_(inversion),
        init_state_(init_state),
        alternate_(alternate),
        rcc_{port} {
    if (mode_ == Mode::Output_PP || mode_ == Mode::Output_OD) {
      if ((bool)init_state_ != (bool)inversion_)
        HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_SET);
      else
        HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_RESET);
    }
    GPIO_InitTypeDef GPIO_InitStruct{0};
    GPIO_InitStruct.Pin = (uint32_t)pin_num_;
    GPIO_InitStruct.Mode = (uint32_t)mode_;
    GPIO_InitStruct.Pull = (uint32_t)pull_;
    GPIO_InitStruct.Speed = (uint32_t)speed_;
    GPIO_InitStruct.Alternate = alternate_;
    HAL_GPIO_Init(port_, &GPIO_InitStruct);
  }
  ~Pin() override { HAL_GPIO_DeInit(port_, (uint32_t)pin_num_); }

  void write(bool state) override {
    if (state != (bool)inversion_)
      HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(port_, (uint32_t)pin_num_, GPIO_PIN_RESET);
  }

  bool read() override {
    return HAL_GPIO_ReadPin(port_, (uint32_t)pin_num_) != (bool)inversion_;
  }

  void toggle() override { HAL_GPIO_TogglePin(port_, (uint32_t)pin_num_); }

 private:
  GPIO_TypeDef* const port_;
  const PinNum pin_num_;
  const Mode mode_;
  const Pull pull_;
  const Speed speed_;
  const Inversion inversion_;
  const InitState init_state_;
  const uint32_t alternate_;
  GpioRcc rcc_;
};

#endif  // PIN_G4_HPP