/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ITEMPSENSE_HPP
#define ITEMPSENSE_HPP

#include <concepts>

namespace m::ifc {
template <typename Unit>
class ITempSense {
 public:
  using type = Unit;

  virtual ~ITempSense() {}

  virtual type value() = 0;

  virtual type min() = 0;
  virtual type max() = 0;
};

template <typename T>
concept CTempSense = requires(T ts) {
  typename T::type;
  { ts.value() } -> std::same_as<typename T::type>;
  { ts.min() } -> std::same_as<typename T::type>;
  { ts.max() } -> std::same_as<typename T::type>;
};

static_assert(CTempSense<ITempSense<int>>,
              "ITempSense must satisfy CTempSense concept");

class ITempSenseError {
 public:
  virtual ~ITempSenseError() {}

  virtual bool shorted() = 0;
  virtual bool broken() = 0;
};

template <typename T>
concept CTempSenseError = requires(T err) {
  { err.shorted() } -> std::same_as<bool>;
  { err.broken() } -> std::same_as<bool>;
};

static_assert(CTempSenseError<ITempSenseError>,
              "ITempSenseError must satisfy CTempSenseError concept");
}  // namespace m::ifc

#endif  // ITEMPSENSE_HPP
