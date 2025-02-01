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

#include <ITime.hpp>
#include <Ms.hpp>
#include <Us.hpp>

#include "stm32f4xx_hal.h"

class TimeUs final : public m::ifc::ITime<Us<uint32_t>> {
 public:
  TimeUs() {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim5_.Instance = TIM5;
    htim5_.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() * 2 / 1'000'000) - 1;
    htim5_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5_.Init.Period = 0xFF'FF'FF'FF;
    htim5_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    htim5_.Base_MspInitCallback = [](TIM_HandleTypeDef* htim) {
      __HAL_RCC_TIM5_CLK_ENABLE();
    };

    HAL_TIM_Base_Init(&htim5_);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim5_, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim5_, &sMasterConfig);

    HAL_TIM_Base_Start(&htim5_);
  }

  TimeUs(const TimeUs&) = delete;
  TimeUs& operator=(const TimeUs&) = delete;
  TimeUs(TimeUs&&) = delete;
  TimeUs& operator=(TimeUs&&) = delete;

  void delay(Us<uint32_t> value) override {
    uint32_t start = htim5_.Instance->CNT;
    uint32_t delay = value.value();
    while (1) {
      uint32_t now = htim5_.Instance->CNT;
      uint32_t diff = now - start;
      if (diff >= delay) break;
    }
  }

  Us<uint32_t> getTick() override { return Us<uint32_t>{htim5_.Instance->CNT}; }

  Us<uint32_t> getDiff(Us<uint32_t> value) override {
    uint32_t diff = htim5_.Instance->CNT;
    diff -= value.value();
    return Us<uint32_t>{diff};
  }

 private:
  TIM_HandleTypeDef htim5_{0};
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

#endif  // TIME_F4_TIM5_H