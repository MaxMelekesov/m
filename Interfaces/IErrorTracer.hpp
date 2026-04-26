/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IERRORTRACER_HPP
#define IERRORTRACER_HPP

#include <concepts>
#include <span>

namespace m::ifc {

template <typename UnitT>
class IErrorTracer {
 public:
  using Unit = UnitT;

  virtual ~IErrorTracer() {}

  virtual bool add(Unit value) = 0;
  virtual void clear() = 0;
  virtual std::span<Unit> getTrace() = 0;
};

template <typename TracerT, typename UnitT>
concept CErrorTracerOf = requires(TracerT& tracer, UnitT value) {
  { tracer.add(value) } -> std::same_as<bool>;
  { tracer.clear() } -> std::same_as<void>;
  { tracer.getTrace() } -> std::same_as<std::span<UnitT>>;
};

template <typename TracerT>
concept CErrorTracer = requires { typename TracerT::Unit; } &&
                       CErrorTracerOf<TracerT, typename TracerT::Unit>;

static_assert(CErrorTracer<IErrorTracer<int>>,
              "IErrorTracer must satisfy CErrorTracer concept");
static_assert(CErrorTracerOf<IErrorTracer<int>, int>,
              "IErrorTracer<int> must satisfy CErrorTracerOf<int> concept");

}  // namespace m::ifc

#endif  // IERRORTRACER_HPP
