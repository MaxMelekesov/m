/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IADCDMACIRCULARREADER_HPP
#define IADCDMACIRCULARREADER_HPP

#include <cstdint>
#include <functional>
#include <span>

namespace m::ifc::mcu {
template <typename T>
class IAdcDmaCircularReader {
 public:
  using type = T;

  virtual ~IAdcDmaCircularReader() {}

  virtual void setHalfConversionCallback(
      std::function<void(std::span<type>)>&& first_half_cb) = 0;

  virtual void setFullConversionCallback(
      std::function<void(std::span<type>)>&& second_half_cb) = 0;

  virtual bool start(std::span<type> data) = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;
};

template <typename T>
concept CAdcDmaCircularReader =
    requires(T reader,
             std::function<void(std::span<typename T::type>)>&&
                 first_half_cb,
             std::function<void(std::span<typename T::type>)>&&
                 second_half_cb,
             std::span<typename T::type> data) {
      {
        reader.setHalfConversionCallback(std::move(first_half_cb))
      } -> std::same_as<void>;
      {
        reader.setFullConversionCallback(std::move(second_half_cb))
      } -> std::same_as<void>;
      { reader.start(data) } -> std::same_as<bool>;
      { reader.running() } -> std::same_as<bool>;
      { reader.stop() } -> std::same_as<bool>;
    } &&
    std::is_same_v<decltype(&T::setHalfConversionCallback),
                   void (T::*)(std::function<void(
                                   std::span<typename T::type>)>&&)> &&
    std::is_same_v<decltype(&T::setFullConversionCallback),
                   void (T::*)(std::function<void(
                                   std::span<typename T::type>)>&&)>;

static_assert(
    CAdcDmaCircularReader<IAdcDmaCircularReader<uint16_t>>,
    "IAdcDmaCircularReader must satisfy CAdcDmaCircularReader concept");

}  // namespace m::ifc::mcu

#endif  // IADCDMACIRCULARREADER_HPP