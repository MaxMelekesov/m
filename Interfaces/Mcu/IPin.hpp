/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef IPIN_H
#define IPIN_H

namespace m::ifc::mcu {
class IPin {
 public:
  virtual ~IPin(){};

  virtual void write(bool state) = 0;
  virtual bool read() = 0;
  virtual void toggle() = 0;
};

}  // namespace m::ifc::mcu

#endif  // IPIN_H