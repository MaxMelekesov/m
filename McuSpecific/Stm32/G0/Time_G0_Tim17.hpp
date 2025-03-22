/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIME_G0_TIM17_H
#define TIME_G0_TIM17_H

#include <ITime.hpp>
#include <Ms.hpp>
#include <Us.hpp>

#include "stm32g0xx_hal.h"

class TimeUs final : public m::ifc::ITime<Us<uint16_t>> {
 public:
  TimeUs() {
    htim17.Instance = TIM17;
    htim17.Init.Prescaler = HAL_RCC_GetPCLK1Freq() / 1'000'000 - 1;
    htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim17.Init.Period = 65535;
    htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim17.Init.RepetitionCounter = 0;
    htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    htim17.Base_MspInitCallback = [](TIM_HandleTypeDef* htim) {
      __HAL_RCC_TIM17_CLK_ENABLE();
    };
    htim17.Base_MspDeInitCallback = [](TIM_HandleTypeDef* htim) {
      __HAL_RCC_TIM17_CLK_DISABLE();
    };

    if (HAL_TIM_Base_Init(&htim17) != HAL_OK) {
    }

    HAL_TIM_Base_Start(&htim17);
  }

  TimeUs(const TimeUs&) = delete;
  TimeUs& operator=(const TimeUs&) = delete;
  TimeUs(TimeUs&&) = delete;
  TimeUs& operator=(TimeUs&&) = delete;

  void delay(Us<uint16_t> value) override {
    uint16_t start = htim17_.Instance->CNT;
    uint16_t delay = value.value();
    while (1) {
      uint16_t now = htim17_.Instance->CNT;
      uint16_t diff = now - start;
      if (diff >= delay) break;
    }
  }

  Us<uint16_t> getTick() override {
    return Us<uint16_t>{static_cast<uint16_t>(htim17_.Instance->CNT)};
  }

  Us<uint16_t> getDiff(Us<uint16_t> value) override {
    uint16_t diff = htim17_.Instance->CNT;
    diff -= value.value();
    return Us<uint16_t>{diff};
  }

 private:
  TIM_HandleTypeDef htim17_{0};
};

class TimeMs final : public m::ifc::ITime<Ms<uint32_t>> {
 public:
  TimeMs() {}

  TimeMs(const TimeMs&) = delete;
  TimeMs& operator=(const TimeMs&) = delete;
  TimeMs(TimeMs&&) = delete;
  TimeMs& operator=(TimeMs&&) = delete;

  void delay(Ms<uint32_t> value) override { HAL_Delay(value.value()); }

  Ms<uint32_t> getTick() override { return Ms<uint32_t>{HAL_GetTick()}; }

  Ms<uint32_t> getDiff(Ms<uint32_t> value) override {
    uint32_t diff = HAL_GetTick();
    diff -= value.value();
    return Ms<uint32_t>{diff};
  }
};

#endif  // TIME_G0_TIM17_H