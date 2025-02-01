/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef SIMPLEERRORTRACER_H
#define SIMPLEERRORTRACER_H

#include <IErrorTracer.hpp>
#include <span>

namespace m {

template <typename T, std::size_t Max_Elements>
class SimpleErrorTracer : public m::ifc::IErrorTracer<T> {
 public:
  using type = T;

  bool add(type value) override {
    if (log_span_.size()) {
      log_span_.front() = value;
      log_span_ = log_span_.subspan(1);
      return true;
    }

    return false;
  }

  void clear() override { log_span_ = log_; }

  std::span<type> getTrace() override {
    return std::span<type>{log_.data(), log_.size() - log_span_.size()};
  }

 private:
  std::array<type, Max_Elements> log_;
  std::span<type> log_span_{log_.data(), log_.size()};
};

}  // namespace m

#endif  // SIMPLEERRORTRACER_H