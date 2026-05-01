/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SPI_HPP
#define SPI_HPP

#include <Bps.hpp>
#include <IIO_Async.hpp>
#include <cstdint>
#include <span>

#include "stm32f4xx_hal.h"

class Spi final : public m::ifc::IIO_Async<Bps<uint32_t>> {
 public:
  Spi(SPI_HandleTypeDef& hspi, Bps<uint32_t> baud) : hspi_(hspi), baud_(baud) {}

  std::size_t bytesWritten() override { return hspi_.hdmatx->Instance->NDTR; }

  bool startWrite(std::span<uint8_t const> data) override {
    auto res =
        (HAL_SPI_Transmit_DMA(&hspi_, data.data(), data.size()) == HAL_OK);
    if (res) {
      dma_tx_started_ = true;
    }
    return res;
  }

  bool abortWrite() override {
    if (!dma_tx_started_) return true;
    auto res = (HAL_SPI_Abort(&hspi_) == HAL_OK);
    if (res) {
      dma_tx_started_ = false;
    }
    return res;
  }

  bool isWriteDone() override {
    if (!dma_tx_started_) return true;
    auto status = HAL_SPI_GetState(&hspi_);
    if (status == HAL_SPI_STATE_READY || status == HAL_SPI_STATE_BUSY_RX) {
      dma_tx_started_ = false;
      return true;
    }
    return false;
  }

  std::size_t bytesReaded() override {
    return rx_size_ - hspi_.hdmarx->Instance->NDTR;
  }

  bool startRead(std::span<uint8_t> data) override {
    bool res =
        (HAL_SPI_Receive_DMA(&hspi_, data.data(), data.size()) == HAL_OK);
    if (res) {
      dma_rx_started_ = true;
      rx_size_ = data.size();
    }
    return res;
  }

  bool abortRead() override {
    if (!dma_rx_started_) return true;
    auto res = (HAL_SPI_Abort(&hspi_) == HAL_OK);
    if (res) {
      dma_rx_started_ = false;
    }
    return res;
  }

  bool isReadDone() override {
    if (!dma_rx_started_) return true;
    if (bytesReaded() != rx_size_) return false;
    auto status = HAL_SPI_GetState(&hspi_);
    if (status == HAL_SPI_STATE_READY || status == HAL_SPI_STATE_BUSY_TX) {
      dma_rx_started_ = false;
      return true;
    }
    return false;
  }

  Bps<uint32_t> getBaudrate() override { return baud_; }

  bool setBaudrate(Bps<uint32_t> baud) override { return false; }

  bool error() override {
    return HAL_SPI_GetError(&hspi_) != HAL_SPI_ERROR_NONE;
  }

 private:
  SPI_HandleTypeDef& hspi_;
  Bps<uint32_t> baud_;

  bool dma_tx_started_ = false;
  bool dma_rx_started_ = false;
  uint32_t rx_size_ = 0;
};

#endif  // SPI_HPP
