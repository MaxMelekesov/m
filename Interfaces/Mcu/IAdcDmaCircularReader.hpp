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

  IAdcDmaCircularReader(
      const std::function<void(std::span<volatile type> first_half)>&
          half_conv_cb,
      const std::function<void(std::span<volatile type> second_half)>& conv_cb)
      : half_conv_cb_(half_conv_cb), conv_cb_(conv_cb) {}

  virtual ~IAdcDmaCircularReader() {}

  virtual bool start(std::span<volatile type> data) = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;

 protected:
  const std::function<void(std::span<volatile type> first_half)>& half_conv_cb_;
  const std::function<void(std::span<volatile type> second_half)>& conv_cb_;
};
}  // namespace m::ifc::mcu

#endif  // IADCDMACIRCULARREADER_H