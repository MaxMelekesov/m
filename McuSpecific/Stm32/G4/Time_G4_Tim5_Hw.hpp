/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef TIME_G4_TIM5_HW_HPP
#define TIME_G4_TIM5_HW_HPP

#include <Hardware.hpp>
#include <ITime.hpp>
#include <Ms.hpp>
#include <Us.hpp>
#include <cstdint>

#include "stm32g4xx_hal.h"

class TimeUs final : public m::mcu::Peripheral<TimeUs>,
                     public m::ifc::ITime<Us<uint32_t>> {
 public:
  explicit TimeUs(HwKey key) : Peripheral(key) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim5_.Instance = TIM5;
    htim5_.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() / 1'000'000) - 1;
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
  ~TimeUs() override = default;

  TimeUs(const TimeUs&) = delete;
  TimeUs& operator=(const TimeUs&) = delete;
  TimeUs(TimeUs&&) = default;
  TimeUs& operator=(TimeUs&&) = delete;

  void delay(Us<uint32_t> value) override {
    Us<uint32_t> start = Us<uint32_t>{htim5_.Instance->CNT};
    Us<uint32_t> delay = Us<uint32_t>{value.value()};
    while (1) {
      Us<uint32_t> now = Us<uint32_t>{htim5_.Instance->CNT};
      Us<uint32_t> diff = now - start;
      if (diff >= delay) break;
    }
  }

  Us<uint32_t> now() override { return Us<uint32_t>{htim5_.Instance->CNT}; }

  Us<uint32_t> diff(Us<uint32_t> value) override {
    Us<uint32_t> diff = Us<uint32_t>{htim5_.Instance->CNT};
    diff -= value;
    return diff;
  }

 private:
  TIM_HandleTypeDef htim5_{0};
};

class TimeMs final : public m::mcu::Peripheral<TimeMs>,
                     public m::ifc::ITime<Ms<uint32_t>> {
 public:
  explicit TimeMs(HwKey key) : Peripheral(key) {}
  ~TimeMs() override = default;
  TimeMs(const TimeMs&) = delete;
  TimeMs& operator=(const TimeMs&) = delete;
  TimeMs(TimeMs&&) = default;
  TimeMs& operator=(TimeMs&&) = delete;

  void delay(Ms<uint32_t> value) override { HAL_Delay(value.value()); }

  Ms<uint32_t> now() override { return Ms<uint32_t>{HAL_GetTick()}; }

  Ms<uint32_t> diff(Ms<uint32_t> value) override {
    Ms<uint32_t> diff = Ms<uint32_t>{HAL_GetTick()};
    diff -= value;
    return diff;
  }
};

#endif  // TIME_G4_TIM5_HW_HPP