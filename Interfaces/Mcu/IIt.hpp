/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IIT_HPP
#define IIT_HPP

#include <functional>

namespace m::ifc::mcu {
class IIt {
 public:
  virtual ~IIt() {}

  virtual void setCallback(std::function<void()>&& cb) = 0;

  virtual bool start() = 0;
  virtual bool running() = 0;
  virtual bool stop() = 0;
};

template <typename T>
concept CIt =
    requires(T it, std::function<void()>&& cb) {
      { it.setCallback(std::move(cb)) } -> std::same_as<void>;
      { it.start() } -> std::same_as<bool>;
      { it.running() } -> std::same_as<bool>;
      { it.stop() } -> std::same_as<bool>;
    } && std::is_same_v<decltype(&T::setCallback),
                        void (T::*)(std::function<void()>&&)>;

static_assert(CIt<IIt>, "IIt must satisfy CIt concept");

}  // namespace m::ifc::mcu

#endif  // IIT_H