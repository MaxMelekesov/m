/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ILOG_H
#define ILOG_H

#include <string_view>

namespace m::ifc {
class ILog {
 public:
  virtual ~ILog() {}

  virtual void add(std::string_view text) = 0;
};
}  // namespace m::ifc

#endif  // ILOG_H