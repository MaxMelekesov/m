/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FLASH_MEM_TEST_HPP
#define FLASH_MEM_TEST_HPP

#include <CoroScheduler.hpp>
#include <IFlashMemory.hpp>
#include <array>
#include <cstdint>
#include <expected>
#include <span>

namespace m::tsts {

// ─── internal helpers ────────────────────────────────────────────────────────

namespace detail::flash_test {

inline uint8_t lcg(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return static_cast<uint8_t>(s >> 16u);
}

template <std::size_t N>
void fillBuf(std::array<uint8_t, N>& buf, uint32_t seed) {
  for (auto& b : buf) b = lcg(seed);
}

// Verify that span == LCG(seed)[skip .. skip+span.size())
inline bool verifySpan(std::span<const uint8_t> sp, uint32_t seed,
                       std::size_t skip = 0u) {
  uint32_t s = seed;
  for (std::size_t i = 0u; i < skip; ++i) lcg(s);
  for (const auto b : sp) {
    if (b != lcg(s)) return false;
  }
  return true;
}

inline bool allFF(std::span<const uint8_t> sp) {
  for (const auto b : sp)
    if (b != 0xFFu) return false;
  return true;
}

}  // namespace detail::flash_test

// ─── flashMemTest
// ─────────────────────────────────────────────────────────────

/**
 * @brief Comprehensive IFlashMemory coroutine test.
 *
 * @tparam Erase_Block_Bytes  Must match flash.eraseBlockSize() at runtime.
 * @tparam Write_Block_Bytes  Must match flash.writeBlockSize() at runtime.
 *
 * Returns std::expected<void, uint8_t>:
 *   has_value() == true  → all tests passed
 *   error()              → first failed test number:
 *                          0  = geometry / minimum-size mismatch
 *                          1…16 = specific test (see table below)
 *
 * Minimum flash size required: 4 × Erase_Block_Bytes.
 *
 * Static BSS per instantiation (Erase=4096, Write=256):
 *   2×256 + 2×4096 + 1×512 + 1×257 ≈ 9.6 KB
 *
 * ┌─────┬────────────────────────────────────────────────────────────────────┐
 * │Test │ What is verified                                                   │
 * ├─────┼────────────────────────────────────────────────────────────────────┤
 * │ T0  │ eraseBlockSize / writeBlockSize match template params;             │
 * │     │ flash.size() >= 4 × Erase_Block_Bytes                             │
 * ├─────┼────────────────────────────────────────────────────────────────────┤
 * │ T1  │ Erase block 0, write page 0, read back Write_Block_Bytes          │
 * │ T2  │ Erase block 0 → verify every byte == 0xFF; write all PpB pages;   │
 * │     │ read back the full Erase_Block_Bytes in one call                  │
 * │ T3  │ Last write-block of the entire flash (boundary: top of address)   │
 * │ T4  │ Cross-erase-block boundary: last page of block 0 + first page     │
 * │     │ of block 1; read 2×Write_Block_Bytes in a single call             │
 * │ T5  │ Single-byte read at addr 0                                        │
 * │ T6  │ Write-block in the middle of flash (page 1 of middle erase block) │
 * │ T7  │ Arbitrary read: start NOT aligned to write-block, length NOT a    │
 * │     │ multiple of Write_Block_Bytes, crosses a page boundary            │
 * ├─────┼────────────────────────────────────────────────────────────────────┤
 * │ T8  │ read(addr = flash.size())           → must return false           │
 * │ T9  │ read(addr = flash.size()-1, size=2) → must return false           │
 * │ T10 │ eraseBlock(1)                       → must return false           │
 * │ T11 │ eraseBlock(flash.size())            → must return false           │
 * │ T12 │ writeBlock(size = 1)                → must return false           │
 * │ T13 │ writeBlock(size = 2×Write_Block_Bytes) → must return false        │
 * │ T14 │ writeBlock(addr = 1)                → must return false           │
 * │ T15 │ writeBlock(addr = flash.size())     → must return false           │
 * │ T16 │ read(empty span)                    → must return true (no-op)    │
 * └─────┴────────────────────────────────────────────────────────────────────┘
 */
template <std::size_t Erase_Block_Bytes, std::size_t Write_Block_Bytes>
m::Task<std::expected<void, uint8_t>> flashMemTest(
    m::ifc::IFlashMemory& flash) {
  static_assert(Erase_Block_Bytes > 0u && Write_Block_Bytes > 0u);
  static_assert(
      Write_Block_Bytes >= 2u,
      "Write_Block_Bytes must be >= 2 for the partial-read test (T7)");
  static_assert((Erase_Block_Bytes % Write_Block_Bytes) == 0u,
                "Erase block must be a multiple of write block");

  static constexpr std::size_t PpB = Erase_Block_Bytes / Write_Block_Bytes;
  static constexpr std::size_t Half_WB = Write_Block_Bytes / 2u;

  // All read/write buffers live in BSS, not on the coroutine frame.
  static std::array<uint8_t, Write_Block_Bytes> wbuf{};
  static std::array<uint8_t, Write_Block_Bytes> rbuf{};
  static std::array<uint8_t, Erase_Block_Bytes> eblk_w{};
  static std::array<uint8_t, Erase_Block_Bytes> eblk_r{};
  static std::array<uint8_t, 2u * Write_Block_Bytes> span2_w{};
  static std::array<uint8_t, 2u * Write_Block_Bytes> span2_r{};
  // Write_Block_Bytes + 1: not a multiple of Write_Block_Bytes (>= 2),
  // read starts at Half_WB → crosses the page-0 / page-1 boundary.
  static std::array<uint8_t, Write_Block_Bytes + 1u> partial_r{};

  namespace dt = detail::flash_test;

  using Res = std::expected<void, uint8_t>;
  auto fail = [](uint8_t t) -> Res { return std::unexpected(t); };

  // ─── T0: geometry & minimum-size ───────────────────────────────────────
  if (flash.eraseBlockSize() != Erase_Block_Bytes ||
      flash.writeBlockSize() != Write_Block_Bytes ||
      flash.size() < 4u * Erase_Block_Bytes) {
    co_return fail(0u);
  }

  const std::size_t fsz = flash.size();

  // ─── T1: single write-block at addr 0, read-back ───────────────────────
  {
    constexpr uint32_t S = 0xA5A5'0001u;
    dt::fillBuf(wbuf, S);

    if (!(co_await flash.eraseBlock(0u))) co_return fail(1u);
    if (!(co_await flash.writeBlock(0u, wbuf))) co_return fail(1u);
    if (!(co_await flash.read(0u, rbuf))) co_return fail(1u);
    if (!dt::verifySpan(rbuf, S)) co_return fail(1u);
  }

  // ─── T2: full erase-block cycle ────────────────────────────────────────
  // Erase → verify 0xFF → write all PpB pages → read full block at once.
  {
    constexpr uint32_t S = 0xB6B6'0002u;
    {
      uint32_t s = S;
      for (auto& b : eblk_w) b = dt::lcg(s);
    }

    if (!(co_await flash.eraseBlock(0u))) co_return fail(2u);
    if (!(co_await flash.read(0u, eblk_r))) co_return fail(2u);
    if (!dt::allFF(eblk_r)) co_return fail(2u);

    for (std::size_t p = 0u; p < PpB; ++p) {
      const std::size_t page_addr = p * Write_Block_Bytes;
      std::span<const uint8_t> page{eblk_w.data() + page_addr,
                                    Write_Block_Bytes};
      if (!(co_await flash.writeBlock(page_addr, page))) co_return fail(2u);
    }

    if (!(co_await flash.read(0u, eblk_r))) co_return fail(2u);
    if (eblk_r != eblk_w) co_return fail(2u);
  }

  // ─── T3: last write-block of the entire flash ───────────────────────────
  {
    constexpr uint32_t S = 0xC7C7'0003u;
    const std::size_t last_eblk = fsz - Erase_Block_Bytes;
    const std::size_t last_wblk = fsz - Write_Block_Bytes;

    dt::fillBuf(wbuf, S);

    if (!(co_await flash.eraseBlock(last_eblk))) co_return fail(3u);
    if (!(co_await flash.writeBlock(last_wblk, wbuf))) co_return fail(3u);
    if (!(co_await flash.read(last_wblk, rbuf))) co_return fail(3u);
    if (!dt::verifySpan(rbuf, S)) co_return fail(3u);
  }

  // ─── T4: cross erase-block boundary read ───────────────────────────────
  // Write last page of block 0 and first page of block 1 with a continuous
  // LCG pattern; read 2×Write_Block_Bytes across the boundary in one call.
  {
    constexpr uint32_t S = 0xD8D8'0004u;
    {
      uint32_t s = S;
      for (auto& b : span2_w) b = dt::lcg(s);
    }

    const std::size_t lo = Erase_Block_Bytes - Write_Block_Bytes;
    const std::size_t hi = Erase_Block_Bytes;

    if (!(co_await flash.eraseBlock(0u))) co_return fail(4u);
    if (!(co_await flash.eraseBlock(Erase_Block_Bytes))) co_return fail(4u);

    if (!(co_await flash.writeBlock(
            lo, std::span<const uint8_t>{span2_w.data(), Write_Block_Bytes})))
      co_return fail(4u);
    if (!(co_await flash.writeBlock(
            hi, std::span<const uint8_t>{span2_w.data() + Write_Block_Bytes,
                                         Write_Block_Bytes})))
      co_return fail(4u);

    if (!(co_await flash.read(lo, span2_r))) co_return fail(4u);
    if (span2_r != span2_w) co_return fail(4u);
  }

  // ─── T5: single-byte read at addr 0 ────────────────────────────────────
  // Block 0 page 0 is in erased state (0xFF) after T4 erased block 0.
  {
    constexpr uint32_t S = 0xE9E9'0005u;
    dt::fillBuf(wbuf, S);

    if (!(co_await flash.writeBlock(0u, wbuf))) co_return fail(5u);

    std::array<uint8_t, 1u> one{};
    if (!(co_await flash.read(0u, one))) co_return fail(5u);
    if (one[0u] != wbuf[0u]) co_return fail(5u);
  }

  // ─── T6: write-block in the middle of flash (page 1 of erase block) ────
  {
    constexpr uint32_t S = 0xF0F0'0006u;
    const std::size_t mid_eblk =
        (fsz / Erase_Block_Bytes / 2u) * Erase_Block_Bytes;
    const std::size_t mid_wblk = mid_eblk + Write_Block_Bytes;  // page 1

    dt::fillBuf(wbuf, S);

    if (!(co_await flash.eraseBlock(mid_eblk))) co_return fail(6u);
    if (!(co_await flash.writeBlock(mid_wblk, wbuf))) co_return fail(6u);
    if (!(co_await flash.read(mid_wblk, rbuf))) co_return fail(6u);
    if (!dt::verifySpan(rbuf, S)) co_return fail(6u);
  }

  // ─── T7: arbitrary read — unaligned start, non-multiple length, cross-page
  // Erase block 2, write pages 0 and 1 with one continuous LCG stream.
  // Read Write_Block_Bytes+1 bytes starting at Half_WB into page 0:
  //   • addr is NOT a multiple of Write_Block_Bytes  (Half_WB offset)
  //   • length is NOT a multiple of Write_Block_Bytes  (WB+1 bytes)
  //   • read crosses the page-0 / page-1 boundary
  {
    constexpr uint32_t S = 0x1234'0007u;
    const std::size_t blk2 = 2u * Erase_Block_Bytes;
    {
      uint32_t s = S;
      for (auto& b : span2_w) b = dt::lcg(s);
    }

    if (!(co_await flash.eraseBlock(blk2))) co_return fail(7u);
    if (!(co_await flash.writeBlock(
            blk2, std::span<const uint8_t>{span2_w.data(), Write_Block_Bytes})))
      co_return fail(7u);
    if (!(co_await flash.writeBlock(
            blk2 + Write_Block_Bytes,
            std::span<const uint8_t>{span2_w.data() + Write_Block_Bytes,
                                     Write_Block_Bytes})))
      co_return fail(7u);

    if (!(co_await flash.read(blk2 + Half_WB, partial_r))) co_return fail(7u);
    if (!dt::verifySpan(partial_r, S, Half_WB)) co_return fail(7u);
  }

  // ─── Negative tests ─────────────────────────────────────────────────────

  // T8: read at addr == flash.size() (out-of-bounds) → false
  if (co_await flash.read(fsz, rbuf)) co_return fail(8u);

  // T9: read where addr + size overflows flash end → false
  {
    std::array<uint8_t, 2u> two{};
    if (co_await flash.read(fsz - 1u, two)) co_return fail(9u);
  }

  // T10: eraseBlock at unaligned address → false
  if (co_await flash.eraseBlock(1u)) co_return fail(10u);

  // T11: eraseBlock out-of-bounds (addr == fsz, aligned but past end) → false
  if (co_await flash.eraseBlock(fsz)) co_return fail(11u);

  // T12: writeBlock with data smaller than Write_Block_Bytes → false
  {
    std::array<uint8_t, 1u> tiny{};
    if (co_await flash.writeBlock(0u, tiny)) co_return fail(12u);
  }

  // T13: writeBlock with data larger than Write_Block_Bytes → false
  if (co_await flash.writeBlock(0u, span2_w)) co_return fail(13u);

  // T14: writeBlock at unaligned address → false
  if (co_await flash.writeBlock(1u, wbuf)) co_return fail(14u);

  // T15: writeBlock out-of-bounds (addr == fsz, aligned but past end) → false
  if (co_await flash.writeBlock(fsz, wbuf)) co_return fail(15u);

  // T16: zero-length read → must succeed (documented no-op)
  {
    std::span<uint8_t> empty{};
    if (!(co_await flash.read(0u, empty))) co_return fail(16u);
  }

  co_return Res{};
}

}  // namespace m::tsts

#endif  // FLASH_MEM_TEST_HPP
