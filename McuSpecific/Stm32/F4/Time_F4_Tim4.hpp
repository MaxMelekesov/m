/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIME_F4_TIM5_HPP
#define TIME_F4_TIM5_HPP

#include <ITime.hpp>
#include <Ms.hpp>
#include <Us.hpp>

#include "stm32f4xx_hal.h"

class TimeUs final : public m::ifc::ITime<Us<uint32_t>> {
 public:
  TimeUs() {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim4_.Instance = TIM4;
    htim4_.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() * 2 / 1'000'000) - 1;
    htim4_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4_.Init.Period = 0xFF'FF;
    htim4_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4_.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    htim4_.Base_MspInitCallback = [](TIM_HandleTypeDef* htim) {
      __HAL_RCC_TIM5_CLK_ENABLE();
    };

    HAL_TIM_Base_Init(&htim4_);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim4_, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim4_, &sMasterConfig);

    HAL_TIM_Base_Start(&htim4_);
  }

  TimeUs(const TimeUs&) = delete;
  TimeUs& operator=(const TimeUs&) = delete;
  TimeUs(TimeUs&&) = delete;
  TimeUs& operator=(TimeUs&&) = delete;

  void delay(Us<uint32_t> value) override {
    Us<uint32_t> start = Us<uint32_t>{htim4_.Instance->CNT};
    Us<uint32_t> delay = Us<uint32_t>{value.value()};
    while (1) {
      Us<uint32_t> now = Us<uint32_t>{htim4_.Instance->CNT};
      Us<uint32_t> diff = now - start;
      if (diff >= delay) break;
    }
  }

  Us<uint32_t> getTick() override { return Us<uint32_t>{htim4_.Instance->CNT}; }

  Us<uint32_t> getDiff(Us<uint32_t> value) override {
    Us<uint32_t> diff = Us<uint32_t>{htim4_.Instance->CNT};
    diff -= value;
    return diff;
  }

 private:
  TIM_HandleTypeDef htim4_{0};
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
    Ms<uint32_t> diff = Ms<uint32_t>{HAL_GetTick()};
    diff -= value;
    return diff;
  }
};

#endif  // TIME_F4_TIM5_HPP