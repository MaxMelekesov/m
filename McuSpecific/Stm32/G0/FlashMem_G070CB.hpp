#pragma once
#include <IMemory.hpp>
#include <array>
#include <cstring>
#include <span>

#include "stm32g0xx_hal.h"
#include "stm32g0xx_ll_utils.h"

class FlashMem_G070CB final : public m::ifc::IMemory {
 private:
  std::size_t const Last_Page_Address =
      FLASH_BASE + FLASH_SIZE - FLASH_PAGE_SIZE;

  static std::size_t const Mem_Size = FLASH_PAGE_SIZE;

 public:
  std::size_t size() override { return Mem_Size; }

  bool write(std::size_t addr, std::span<uint8_t const> data) override {
    if (addr > size() || data.size() > size() - addr) return false;

    alignas(alignof(uint64_t)) std::array<uint8_t, Mem_Size> page;
    if (!read(0, page)) return false;
    memcpy(&page[addr], data.data(), data.size());

    if (HAL_FLASH_Unlock() != HAL_OK) return false;

    FLASH_EraseInitTypeDef eraseData;
    eraseData.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseData.Banks = FLASH_BANK_1;
    eraseData.Page = FLASH_PAGE_NB - 1;
    eraseData.NbPages = 1;

    uint32_t pageError;

    if (HAL_FLASHEx_Erase(&eraseData, &pageError) != HAL_OK) {
      HAL_FLASH_Lock();
      return false;
    }

    uint32_t start_addr = static_cast<uint32_t>(Last_Page_Address);
    for (auto i = 0u; i < Mem_Size / sizeof(uint64_t); ++i) {
      uint64_t data64 = 0;
      memcpy(&data64, &page[i * sizeof(uint64_t)], sizeof(uint64_t));
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, start_addr, data64) !=
          HAL_OK) {
        HAL_FLASH_Lock();
        return false;
      }
      start_addr += sizeof(uint64_t);
    }

    if (HAL_FLASH_Lock() != HAL_OK) return false;

    return true;
  }

  bool read(std::size_t addr, std::span<uint8_t> data) override {
    if (addr > size() || data.size() > size() - addr) return false;

    const auto flash_mem = reinterpret_cast<uint8_t const*>(Last_Page_Address);
    memcpy(data.data(), &flash_mem[addr], data.size());

    return true;
  }
};