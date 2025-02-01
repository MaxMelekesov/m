/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2025 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef MEMORYTEST_H
#define MEMORYTEST_H

#include <IMemory.hpp>
#include <cstdlib>

namespace m::tsts {

template <uint32_t Buf_Size>
bool memoryTest(ifc::IMemory& mem) {
  std::srand(0x12'34'56'78);

  if (mem.size() < Buf_Size - 2) return false;

  std::array<uint8_t, Buf_Size> test_buf_output;
  std::array<uint8_t, Buf_Size> test_buf_input;

  for (auto& v : test_buf_output) {
    v = std::rand() % 255;
  }

  uint32_t addr = std::rand() % (mem.size() - Buf_Size - 1);

  if (!mem.write(addr, test_buf_output)) return false;
  if (!mem.read(addr, test_buf_input)) return false;

  for (auto i = 0u; i < test_buf_output.size(); ++i) {
    if (test_buf_output[i] != test_buf_input[i]) {
      return false;
    }
  }

  return true;
}
}  // namespace m::tsts

#endif  // MEMORYTEST_H