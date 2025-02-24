/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef ITEMPSENSE_H
#define ITEMPSENSE_H

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

class ITempSenseError {
 public:
  virtual ~ITempSenseError() {}

  virtual bool shorted() = 0;
  virtual bool broken() = 0;
};
}  // namespace m::ifc

#endif  // ITEMPSENSE_H
