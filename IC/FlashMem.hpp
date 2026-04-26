/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FLASH_MEM_HPP
#define FLASH_MEM_HPP

#include <IFlashMemory.hpp>
#include <IMemoryCoro.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace m::ic {

template <m::ifc::CFlashMemory FlashT,
          std::size_t Max_Erase_Block_Bytes = 4u * 1024u>
class FlashMem : public m::ifc::IMemoryCoro {
 public:
  static_assert(Max_Erase_Block_Bytes > 0u, "Max erase block size must be > 0");

  explicit FlashMem(FlashT& flash) : flash_(flash) {}

  std::size_t size() override { return flash_.size(); }

  m::Task<bool> read(std::size_t addr, std::span<uint8_t> data) override {
    if (data.empty()) {
      co_return true;
    }
    if (!isRangeValid(addr, data.size())) {
      co_return false;
    }

    co_return co_await flash_.read(addr, data);
  }

  m::Task<bool> write(std::size_t addr,
                      std::span<uint8_t const> data) override {
    if (data.empty()) {
      co_return true;
    }
    if (!isRangeValid(addr, data.size())) {
      co_return false;
    }

    const auto erase_block_size = flash_.eraseBlockSize();
    const auto write_block_size = flash_.writeBlockSize();
    if (!isGeometryValid(erase_block_size, write_block_size)) {
      co_return false;
    }

    std::size_t src_offset = 0u;
    while (src_offset < data.size()) {
      const std::size_t write_addr = addr + src_offset;
      const std::size_t erase_base = alignDown(write_addr, erase_block_size);
      const std::size_t erase_offset = write_addr - erase_base;
      const std::size_t chunk_size =
          std::min(erase_block_size - erase_offset, data.size() - src_offset);

      auto erase_buf = std::span<uint8_t>{erase_buf_}.first(erase_block_size);
      if (!(co_await flash_.read(erase_base, erase_buf))) {
        co_return false;
      }

      std::copy_n(data.data() + src_offset, chunk_size,
                  erase_buf.data() + erase_offset);

      if (!(co_await flash_.eraseBlock(erase_base))) {
        co_return false;
      }

      for (std::size_t page_offset = 0u; page_offset < erase_block_size;
           page_offset += write_block_size) {
        const std::size_t page_addr = erase_base + page_offset;
        auto page_data = std::span<uint8_t const>{erase_buf}.subspan(
            page_offset, write_block_size);
        if (!(co_await flash_.writeBlock(page_addr, page_data))) {
          co_return false;
        }
      }

      src_offset += chunk_size;
    }

    co_return true;
  }

 private:
  FlashT& flash_;
  std::array<uint8_t, Max_Erase_Block_Bytes> erase_buf_{};

  static std::size_t alignDown(std::size_t value, std::size_t align) {
    return (value / align) * align;
  }

  bool isRangeValid(std::size_t addr, std::size_t size_bytes) {
    if (size_bytes == 0u) {
      return true;
    }

    const auto total_size = flash_.size();
    if (addr >= total_size) {
      return false;
    }

    return size_bytes <= (total_size - addr);
  }

  bool isGeometryValid(std::size_t erase_block_size,
                       std::size_t write_block_size) {
    if (erase_block_size == 0u || write_block_size == 0u) {
      return false;
    }
    if (erase_block_size > Max_Erase_Block_Bytes) {
      return false;
    }
    if (write_block_size > erase_block_size) {
      return false;
    }
    if ((erase_block_size % write_block_size) != 0u) {
      return false;
    }

    return true;
  }
};

}  // namespace m::ic

#endif  // FLASH_MEM_HPP
