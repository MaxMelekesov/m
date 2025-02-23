/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IADCDMACIRCULARREADER_H
#define IADCDMACIRCULARREADER_H

#include <functional>
#include <span>

namespace m::ifc::mcu {
template <typename T>
class IAdcDmaCircularReader {
 public:
  using type = T;

  virtual ~IAdcDmaCircularReader() {}

  virtual void setHalfConversionCallback(
      std::function<void(std::span<volatile type>)>&& first_half_cb) = 0;

  virtual void setFullConversionCallback(
      std::function<void(std::span<volatile type>)>&& second_half_cb) = 0;

  virtual bool start(std::span<volatile type> data) = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;
};
}  // namespace m::ifc::mcu

#endif  // IADCDMACIRCULARREADER_H