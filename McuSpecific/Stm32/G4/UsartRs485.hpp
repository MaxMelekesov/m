/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef USART_RS485_HPP
#define USART_RS485_HPP

#include <Bps.hpp>
#include <IIO_Async.hpp>
#include <IPin.hpp>
#include <cstdint>
#include <span>

#include "stm32g4xx_hal_uart.h"

class UsartRs485 final : public m::ifc::IIO_Async<Bps<uint32_t>> {
 public:
  UsartRs485(m::ifc::mcu::IPin& dr_en, UART_HandleTypeDef& huart, uint32_t baud)
      : dr_en_(dr_en), huart_(huart), baud_(baud) {}

  std::size_t bytesToWrite() override { return huart_.hdmatx->Instance->CNDTR; }

  bool writeAsync(std::span<uint8_t const> data) override {
    dr_en_.write(1);
    auto res = (HAL_UART_Transmit_DMA(&huart_, (uint8_t*)data.data(),
                                      data.size()) == HAL_OK);
    if (res) {
      dma_tx_started_ = true;
    }
    return res;
  }

  bool abortWrite() override {
    if (!dma_tx_started_) return true;
    auto res = (HAL_UART_AbortTransmit(&huart_) == HAL_OK);
    if (res) {
      dma_tx_started_ = false;
    }

    return res;
  }

  bool writeDone() override {
    if (dma_tx_started_) {
      if (bytesToWrite() == 0) {
        if (HAL_UART_GetState(&huart_) == HAL_UART_STATE_READY) {
          dma_tx_started_ = false;
          return true;
        } else {
          return false;
        }
      }
      return false;
    }
    return true;
  }

  std::size_t bytesAvailable() override {
    return rx_size_ - huart_.hdmarx->Instance->CNDTR;
  }

  bool readAsync(std::span<uint8_t> data) override {
    dr_en_.write(0);
    bool res = (HAL_UART_Receive_DMA(&huart_, (uint8_t*)data.data(),
                                     data.size()) == HAL_OK);
    if (res) {
      dma_rx_started_ = true;
      rx_size_ = data.size();
    }
    return res;
  }

  bool abortRead() override {
    if (!dma_rx_started_) return true;
    auto res = (HAL_UART_AbortReceive(&huart_) == HAL_OK);
    if (res) {
      dma_rx_started_ = false;
    }

    return res;
  }

  bool readDone() override {
    if (dma_rx_started_) {
      if (bytesAvailable() == rx_size_) {
        if (abortRead()) {
          if (HAL_UART_GetState(&huart_) == HAL_UART_STATE_READY) {
            return true;
          }
        }
      }
      return false;
    }
    return true;
  }

  Bps<uint32_t> getBaudrate() override { return baud_ / 10; }

  bool setBaudrate(Bps<uint32_t> baud) override { return false; }

  bool error() override {
    return HAL_UART_GetError(&huart_) != HAL_UART_ERROR_NONE;
  }

 private:
  m::ifc::mcu::IPin& dr_en_;
  UART_HandleTypeDef& huart_;
  Bps<uint32_t> baud_;

  bool dma_tx_started_ = false;
  bool dma_rx_started_ = false;
  uint32_t rx_size_ = 0;
};

#endif  // USART_RS485_H