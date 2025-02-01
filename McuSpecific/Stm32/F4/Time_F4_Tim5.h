/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIME_F4_TIM5_H
#define TIME_F4_TIM5_H

#include <ITime.h>

#include "stm32f4xx_hal.h"

class Time final : public m::ifc::ITime {
 private:
  using Ms = m::ifc::Ms;
  using Us = m::ifc::Us;

 public:
  Time() {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim5_.Instance = TIM5;
    htim5_.Init.Prescaler = 84 - 1;  // TIM5 freq 84MHz
    // TODO: calc prescaler automaticaly
    // htim5_.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() * 2 / 1'000'000) - 1;
    htim5_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5_.Init.Period = 0xFF'FF'FF'FF;
    htim5_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim5_) != HAL_OK) {
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5_, &sClockSourceConfig) != HAL_OK) {
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5_, &sMasterConfig) !=
        HAL_OK) {
    }

    HAL_TIM_Base_Start(&htim5_);
  }

  Time(const Time&) = delete;
  Time& operator=(const Time&) = delete;
  Time(Time&&) = delete;
  Time& operator=(Time&&) = delete;

  void delay(Us us) override {
    uint32_t start = htim5_.Instance->CNT;
    uint32_t delay = us.value();
    while (1) {
      uint32_t now = htim5_.Instance->CNT;
      uint32_t diff = now - start;
      if (diff >= delay) break;
    }
  }
  void delay(Ms ms) override {
    uint32_t start = htim5_.Instance->CNT;
    uint32_t delay_us = ms.value() * 1э000;
    while (1) {
      uint32_t now = htim5_.Instance->CNT;
      uint32_t diff = now - start;
      if (diff >= delay_us) break;
    }
  }

  Us getTickUs() override { return Us{htim5_.Instance->CNT}; }
  Ms getTickMs() override { return Ms{HAL_GetTick()}; }

  Us getDiff(Us startUs) override {
    uint32_t diff = htim5_.Instance->CNT;
    diff -= (uint32_t)startUs.value();
    return Us{diff};
  }
  Ms getDiff(Ms startMs) override { return getTickMs() - startMs; }

 private:
  TIM_HandleTypeDef htim5_{0};
};

#endif  // TIME_F4_TIM5_H