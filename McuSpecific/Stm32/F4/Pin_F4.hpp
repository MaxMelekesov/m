/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef PIN_F4_H
#define PIN_F4_H

#include <IPin.hpp>
#include <array>

#include "stm32f4xx_hal.h"

class Pin final : public m::ifc::mcu::IPin {
 public:
  enum class PinNum : uint16_t {
    Pin_0 = 0x00'01,
    Pin_1 = 0x00'02,
    Pin_2 = 0x00'04,
    Pin_3 = 0x00'08,
    Pin_4 = 0x00'10,
    Pin_5 = 0x00'20,
    Pin_6 = 0x00'40,
    Pin_7 = 0x00'80,
    Pin_8 = 0x01'00,
    Pin_9 = 0x02'00,
    Pin_10 = 0x04'00,
    Pin_11 = 0x08'00,
    Pin_12 = 0x10'00,
    Pin_13 = 0x20'00,
    Pin_14 = 0x40'00,
    Pin_15 = 0x80'00,
    Pin_All = 0xFF'FF
  };

  enum class Mode : uint32_t {
    Input = 0x00'00'00'00,
    Output_PP = 0x00'00'00'01,
    Output_OD = 0x00'00'00'11,
    Mode_AF_PP = 0x00'00'00'02,
    Mode_AF_OD = 0x00'00'00'12,

    Mode_Analog = 0x00'00'00'03,

    Mode_IT_Rising = 0x00'11'00'00,
    Mode_IT_Falling = 0x00'21'00'00,
    Mode_IT_Rising_Falling = 0x00'31'00'00,

    Mode_EVT_Rising = 0x00'12'00'00,
    Mode_EVT_Falling = 0x00'22'00'00,
    Mode_EVT_Rising_Falling = 0x00'32'00'00
  };

  enum class Pull : uint32_t {
    Nopull = 0x00'00'00'00,
    Pullup = 0x00'00'00'01,
    Pulldown = 0x00'00'00'02
  };

  enum class Speed : uint32_t {
    Freq_Low = 0x00'00'00'00,
    Freq_Medium = 0x00'00'00'01,
    Freq_High = 0x00'00'00'02,
    Freq_Very_High = 0x00'00'00'03
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
        alternate_(alternate) {
    rcc_.enableClock(port);

    if (mode_ == Mode::Output_PP || mode_ == Mode::Output_OD) {
      if ((bool)init_state_ != (bool)inversion_)
        HAL_GPIO_WritePin(port_, static_cast<uint16_t>(pin_num_), GPIO_PIN_SET);
      else
        HAL_GPIO_WritePin(port_, static_cast<uint16_t>(pin_num_),
                          GPIO_PIN_RESET);
    }

    GPIO_InitTypeDef GPIO_InitStruct{0};
    GPIO_InitStruct.Pin = static_cast<uint32_t>(pin_num_);
    GPIO_InitStruct.Mode = (uint32_t)mode_;
    GPIO_InitStruct.Pull = (uint32_t)pull_;
    GPIO_InitStruct.Speed = (uint32_t)speed_;
    GPIO_InitStruct.Alternate = alternate_;
    HAL_GPIO_Init(port_, &GPIO_InitStruct);
  }

  ~Pin() override {
    HAL_GPIO_DeInit(port_, static_cast<uint32_t>(pin_num_));
    rcc_.disableClock(port_);
  }

  void write(bool state) override {
    if (state != (bool)inversion_)
      HAL_GPIO_WritePin(port_, static_cast<uint32_t>(pin_num_), GPIO_PIN_SET);
    else
      HAL_GPIO_WritePin(port_, static_cast<uint32_t>(pin_num_), GPIO_PIN_RESET);
  }

  bool read() const override {
    return HAL_GPIO_ReadPin(port_, static_cast<uint32_t>(pin_num_)) !=
           (bool)inversion_;
  }

  void toggle() override {
    HAL_GPIO_TogglePin(port_, static_cast<uint32_t>(pin_num_));
  }

 private:
  GPIO_TypeDef* const port_;
  const PinNum pin_num_;
  const Mode mode_;
  const Pull pull_;
  const Speed speed_;
  const Inversion inversion_;
  const InitState init_state_;
  const uint32_t alternate_;

  class GpioRcc {
   private:
    std::array<uint32_t, 9> port_init_count{0};

   public:
    GpioRcc() {}

    void enableClock(GPIO_TypeDef* port) noexcept {
      if (port == GPIOA) {
        if (port_init_count[0]++ == 0) __HAL_RCC_GPIOA_CLK_ENABLE();
      } else if (port == GPIOB) {
        if (port_init_count[1]++ == 0) __HAL_RCC_GPIOB_CLK_ENABLE();
      } else if (port == GPIOC) {
        if (port_init_count[2]++ == 0) __HAL_RCC_GPIOC_CLK_ENABLE();
      } else if (port == GPIOD) {
        if (port_init_count[3]++ == 0) __HAL_RCC_GPIOD_CLK_ENABLE();
      } else if (port == GPIOE) {
        if (port_init_count[4]++ == 0) __HAL_RCC_GPIOE_CLK_ENABLE();
      } else if (port == GPIOF) {
        if (port_init_count[5]++ == 0) __HAL_RCC_GPIOF_CLK_ENABLE();
      } else if (port == GPIOG) {
        if (port_init_count[6]++ == 0) __HAL_RCC_GPIOG_CLK_ENABLE();
      } else if (port == GPIOH) {
        if (port_init_count[7]++ == 0) __HAL_RCC_GPIOH_CLK_ENABLE();
      } else if (port == GPIOI) {
        if (port_init_count[8]++ == 0) __HAL_RCC_GPIOI_CLK_ENABLE();
      }
    }

    void disableClock(GPIO_TypeDef* port) noexcept {
      if (port == GPIOA) {
        if (port_init_count[0]-- == 1) __HAL_RCC_GPIOA_CLK_DISABLE();
      } else if (port == GPIOB) {
        if (port_init_count[1]-- == 1) __HAL_RCC_GPIOB_CLK_DISABLE();
      } else if (port == GPIOC) {
        if (port_init_count[2]-- == 1) __HAL_RCC_GPIOC_CLK_DISABLE();
      } else if (port == GPIOD) {
        if (port_init_count[3]-- == 1) __HAL_RCC_GPIOD_CLK_DISABLE();
      } else if (port == GPIOE) {
        if (port_init_count[4]-- == 1) __HAL_RCC_GPIOE_CLK_DISABLE();
      } else if (port == GPIOF) {
        if (port_init_count[5]-- == 1) __HAL_RCC_GPIOF_CLK_DISABLE();
      } else if (port == GPIOG) {
        if (port_init_count[6]-- == 1) __HAL_RCC_GPIOG_CLK_DISABLE();
      } else if (port == GPIOH) {
        if (port_init_count[7]-- == 1) __HAL_RCC_GPIOH_CLK_DISABLE();
      } else if (port == GPIOI) {
        if (port_init_count[8]-- == 1) __HAL_RCC_GPIOI_CLK_DISABLE();
      }
    }
  };

  static inline GpioRcc rcc_;

  static_assert(GPIO_SPEED_FREQ_LOW == (uint32_t)Speed::Freq_Low,
                "Bad Speed::Freq_Low value");
  static_assert(GPIO_SPEED_FREQ_MEDIUM == (uint32_t)Speed::Freq_Medium,
                "Bad Speed::Freq_Medium value");
  static_assert(GPIO_SPEED_FREQ_HIGH == (uint32_t)Speed::Freq_High,
                "Bad Speed::Freq_High value");
  static_assert(GPIO_SPEED_FREQ_VERY_HIGH == (uint32_t)Speed::Freq_Very_High,
                "Bad Speed::Freq_High value");

  static_assert(GPIO_NOPULL == (uint32_t)Pull::Nopull,
                "Bad Pull::Nopull value");
  static_assert(GPIO_PULLUP == (uint32_t)Pull::Pullup,
                "Bad Pull::Pullup value");
  static_assert(GPIO_PULLDOWN == (uint32_t)Pull::Pulldown,
                "Bad Pull::Pulldown value");

  static_assert(GPIO_MODE_INPUT == (uint32_t)Mode::Input,
                "Bad Mode::Input value");
  static_assert(GPIO_MODE_OUTPUT_PP == (uint32_t)Mode::Output_PP,
                "Bad Mode::Output_PP value");
  static_assert(GPIO_MODE_OUTPUT_OD == (uint32_t)Mode::Output_OD,
                "Bad Mode::Output_OD value");
  static_assert(GPIO_MODE_AF_PP == (uint32_t)Mode::Mode_AF_PP,
                "Bad Mode::Mode_AF_PP value");
  static_assert(GPIO_MODE_AF_OD == (uint32_t)Mode::Mode_AF_OD,
                "Bad Mode::Mode_AF_OD value");

  static_assert(GPIO_MODE_ANALOG == (uint32_t)Mode::Mode_Analog,
                "Bad Mode::Mode_Analog value");

  static_assert(GPIO_MODE_IT_RISING == (uint32_t)Mode::Mode_IT_Rising,
                "Bad Mode::Mode_IT_Rising value");
  static_assert(GPIO_MODE_IT_FALLING == (uint32_t)Mode::Mode_IT_Falling,
                "Bad Mode::Mode_IT_Falling value");
  static_assert(GPIO_MODE_IT_RISING_FALLING ==
                    (uint32_t)Mode::Mode_IT_Rising_Falling,
                "Bad Mode::Mode_IT_Rising_Falling value");

  static_assert(GPIO_MODE_EVT_RISING == (uint32_t)Mode::Mode_EVT_Rising,
                "Bad Mode::Mode_EVT_Rising value");
  static_assert(GPIO_MODE_EVT_FALLING == (uint32_t)Mode::Mode_EVT_Falling,
                "Bad Mode::Mode_EVT_Falling value");
  static_assert(GPIO_MODE_EVT_RISING_FALLING ==
                    (uint32_t)Mode::Mode_EVT_Rising_Falling,
                "Bad Mode::Mode_EVT_Rising_Falling value");

  static_assert(GPIO_PIN_0 == (uint32_t)PinNum::Pin_0, "Bad Pin::Pin_0 value");
  static_assert(GPIO_PIN_1 == (uint32_t)PinNum::Pin_1, "Bad Pin::Pin_1 value");
  static_assert(GPIO_PIN_2 == (uint32_t)PinNum::Pin_2, "Bad Pin::Pin_2 value");
  static_assert(GPIO_PIN_3 == (uint32_t)PinNum::Pin_3, "Bad Pin::Pin_3 value");
  static_assert(GPIO_PIN_4 == (uint32_t)PinNum::Pin_4, "Bad Pin::Pin_4 value");
  static_assert(GPIO_PIN_5 == (uint32_t)PinNum::Pin_5, "Bad Pin::Pin_5 value");
  static_assert(GPIO_PIN_6 == (uint32_t)PinNum::Pin_6, "Bad Pin::Pin_6 value");
  static_assert(GPIO_PIN_7 == (uint32_t)PinNum::Pin_7, "Bad Pin::Pin_7 value");
  static_assert(GPIO_PIN_8 == (uint32_t)PinNum::Pin_8, "Bad Pin::Pin_8 value");
  static_assert(GPIO_PIN_9 == (uint32_t)PinNum::Pin_9, "Bad Pin::Pin_9 value");
  static_assert(GPIO_PIN_10 == (uint32_t)PinNum::Pin_10,
                "Bad Pin::Pin_10 value");
  static_assert(GPIO_PIN_11 == (uint32_t)PinNum::Pin_11,
                "Bad Pin::Pin_11 value");
  static_assert(GPIO_PIN_12 == (uint32_t)PinNum::Pin_12,
                "Bad Pin::Pin_12 value");
  static_assert(GPIO_PIN_13 == (uint32_t)PinNum::Pin_13,
                "Bad Pin::Pin_13 value");
  static_assert(GPIO_PIN_14 == (uint32_t)PinNum::Pin_14,
                "Bad Pin::Pin_14 value");
  static_assert(GPIO_PIN_15 == (uint32_t)PinNum::Pin_15,
                "Bad Pin::Pin_15 value");
  static_assert(GPIO_PIN_All == (uint32_t)PinNum::Pin_All,
                "Bad Pin::Pin_All value");
};

#endif  // PIN_F4_H