/**
 * This file is part of m library.
 *
 * m library is free software: you can redistribute it and/or modify
 * it under the terms of the MIT License. See the LICENSE file in the
 * project root for more information.
 *
 * Copyright (c) 2026 Max Melekesov <max.melekesov@gmail.com>
 */

#ifndef FLASH_MEM_CORO_TEST_HPP
#define FLASH_MEM_CORO_TEST_HPP

#include <CoroScheduler.hpp>
#include <IMemoryCoro.hpp>

#include <array>
#include <cstdint>
#include <expected>
#include <span>

namespace m::tsts {

// ─── internal helpers ─────────────────────────────────────────────────────────

namespace detail::flash_coro_test {

inline uint8_t lcg(uint32_t& s) noexcept {
  s = s * 1664525u + 1013904223u;
  return static_cast<uint8_t>(s >> 16u);
}

template <std::size_t N>
void fillBuf(std::array<uint8_t, N>& buf, uint32_t seed) noexcept {
  for (auto& b : buf) b = lcg(seed);
}

// Returns true when sp == LCG(seed)[skip .. skip+sp.size()).
inline bool verifySpan(std::span<const uint8_t> sp, uint32_t seed,
                       std::size_t skip = 0u) noexcept {
  uint32_t s = seed;
  for (std::size_t i = 0u; i < skip; ++i) lcg(s);
  for (const auto b : sp) {
    if (b != lcg(s)) return false;
  }
  return true;
}

}  // namespace detail::flash_coro_test

// ─── flashMemCoroTest ─────────────────────────────────────────────────────────

/**
 * @brief Comprehensive IMemoryCoro (FlashMem) coroutine test.
 *
 * Tests the RMW write layer built on top of IFlashMemory.
 * The geometry is not exposed through IMemoryCoro, so it must be supplied
 * as template parameter.
 *
 * @tparam Erase_Block_Bytes  Must equal the underlying flash erase-block size.
 *
 * Returns std::expected<void, uint8_t>:
 *   has_value() == true → all tests passed
 *   error()             → failed point code (see table below)
 *
 * Error code encoding:  test_number * 10 + sub_step
 *
 *  Code │ Meaning
 * ──────┼────────────────────────────────────────────────────────────────────────
 *    0  │ flash size < 3 × Erase_Block_Bytes
 *    1  │ SPI probe: bare read(addr 0, 1 byte) failed  ← check SPI first
 *    2  │ Large-read probe: read(0, EB) failed  ← SPI RX broken for big buffers
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   10  │ T1  write(0, 1 B) returned false  (if code 2 passed: erase or WB fail)
 *   11  │ T1  read(0, 1 B) returned false
 *   12  │ T1  data mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   20  │ T2  write(0, EB) returned false
 *   21  │ T2  read(0, EB) returned false
 *   22  │ T2  data mismatch (full block)
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   30  │ T3  write patch (3 B at Quarter) returned false
 *   31  │ T3  read(0, EB) returned false
 *   32  │ T3  prefix [0, Quarter) mismatch (should be unchanged)
 *   33  │ T3  patch [Quarter, Quarter+3) mismatch
 *   34  │ T3  suffix [Quarter+3, EB) mismatch (should be unchanged)
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   40  │ T4  initial write(0, EB) returned false
 *   41  │ T4  patch write(EB-1, 1 B) returned false
 *   42  │ T4  read(0, EB) returned false
 *   43  │ T4  prefix [0, EB-1) mismatch (should be unchanged)
 *   44  │ T4  last byte mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   50  │ T5  initial write(0, EB) returned false
 *   51  │ T5  patch write(1, 3 B) returned false
 *   52  │ T5  read(0, EB) returned false
 *   53  │ T5  byte[0] mismatch (should be unchanged)
 *   54  │ T5  patch [1..3] mismatch
 *   55  │ T5  suffix [4, EB) mismatch (should be unchanged)
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   60  │ T6  write block-0 (3Q, EB pre-fill) returned false
 *   61  │ T6  write block-1 (EB, EB pre-fill) returned false
 *   62  │ T6  spanning write(3Q, Half+2) returned false
 *   63  │ T6  read block-0 returned false
 *   64  │ T6  block-0 prefix [0, 3Q) mismatch
 *   65  │ T6  block-0 written tail [3Q, EB) mismatch
 *   66  │ T6  read block-1 returned false
 *   67  │ T6  block-1 written head mismatch
 *   68  │ T6  block-1 untouched tail mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   70  │ T7  write(EB, EB) returned false
 *   71  │ T7  read(EB, EB) returned false
 *   72  │ T7  data mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   80  │ T8  write(last, 1 B) returned false
 *   81  │ T8  read(last, 1 B) returned false
 *   82  │ T8  data mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *   90  │ T9  setup write(0, EB) returned false
 *   91  │ T9  read(Q+1, Half+3) returned false
 *   92  │ T9  data mismatch
 * ──────┼────────────────────────────────────────────────────────────────────────
 *  100  │ T10 read(fsz, 1) should return false but returned true
 *  110  │ T11 read(fsz-1, 2) should return false but returned true
 *  120  │ T12 write(fsz, 1) should return false but returned true
 *  130  │ T13 write(fsz-1, 2) should return false but returned true
 *  140  │ T14 write(empty) should return true but returned false
 *  150  │ T15 read(empty) should return true but returned false
 */
template <std::size_t Erase_Block_Bytes = 4u * 1024u>
m::Task<std::expected<void, uint8_t>> flashMemCoroTest(
    m::ifc::IMemoryCoro& mem) {
  static_assert(Erase_Block_Bytes >= 8u,
                "Erase_Block_Bytes must be >= 8 for sub-block tests");
  static_assert((Erase_Block_Bytes & (Erase_Block_Bytes - 1u)) == 0u,
                "Erase_Block_Bytes must be a power of two");

  static constexpr std::size_t Half    = Erase_Block_Bytes / 2u;
  static constexpr std::size_t Quarter = Erase_Block_Bytes / 4u;
  static constexpr std::size_t EB      = Erase_Block_Bytes;

  // Two erase-block-sized buffers in BSS (not on the coroutine frame).
  static std::array<uint8_t, EB> buf_w{};
  static std::array<uint8_t, EB> buf_r{};

  namespace dt = detail::flash_coro_test;

  using Res = std::expected<void, uint8_t>;
  auto fail = [](uint8_t code) noexcept -> Res { return std::unexpected(code); };

  // ─── T0: minimum size ──────────────────────────────────────────────────
  const std::size_t fsz = mem.size();
  if (fsz < 3u * EB) co_return fail(0u);

  // ─── SPI probe: bare read 1 byte from addr 0 ───────────────────────────
  // Does NOT check the value — only verifies the transport is alive.
  // If this returns error code 1, the issue is in SPI/hardware, not logic.
  {
    std::array<uint8_t, 1u> probe{};
    if (!(co_await mem.read(0u, probe))) co_return fail(1u);
  }

  // ─── Large-read probe: read full erase block ───────────────────────────
  // Code 2: mem.read(0, EB) failed — large SPI RX doesn't work.
  // If this passes but T1 fails (code 10), the fault is in erase or page-write.
  if (!(co_await mem.read(0u, buf_r))) co_return fail(2u);

  // ─── T1: single-byte write at addr 0, read back ────────────────────────
  {
    constexpr uint32_t S = 0xAA11'0001u;
    uint32_t s = S;
    buf_w[0u] = dt::lcg(s);

    if (!(co_await mem.write(0u, std::span<const uint8_t>{buf_w.data(), 1u})))
      co_return fail(10u);
    if (!(co_await mem.read(0u, std::span<uint8_t>{buf_r.data(), 1u})))
      co_return fail(11u);
    if (buf_r[0u] != buf_w[0u]) co_return fail(12u);
  }

  // ─── T2: write full erase block, read back ─────────────────────────────
  {
    constexpr uint32_t S = 0xBB22'0002u;
    dt::fillBuf(buf_w, S);

    if (!(co_await mem.write(0u, buf_w))) co_return fail(20u);
    if (!(co_await mem.read(0u, buf_r)))  co_return fail(21u);
    if (buf_r != buf_w)                   co_return fail(22u);
  }

  // ─── T3: RMW — 3-byte patch at Quarter offset ──────────────────────────
  // Block 0 holds T2 pattern; only 3 bytes change.
  {
    constexpr uint32_t S_prev = 0xBB22'0002u;
    constexpr uint8_t  MA = 0x5Au, MB = 0xA5u, MC = 0x3Cu;

    std::array<uint8_t, 3u> patch{MA, MB, MC};
    if (!(co_await mem.write(Quarter, patch)))   co_return fail(30u);
    if (!(co_await mem.read(0u, buf_r)))          co_return fail(31u);

    if (!dt::verifySpan({buf_r.data(), Quarter}, S_prev))
      co_return fail(32u);
    if (buf_r[Quarter] != MA || buf_r[Quarter + 1u] != MB ||
        buf_r[Quarter + 2u] != MC)
      co_return fail(33u);
    if (!dt::verifySpan({buf_r.data() + Quarter + 3u, EB - Quarter - 3u},
                        S_prev, Quarter + 3u))
      co_return fail(34u);
  }

  // ─── T4: RMW — single-byte at the very last byte of block 0 ─────────────
  {
    constexpr uint32_t S = 0xCC33'0004u;
    constexpr uint8_t  LAST = 0xDEu;

    dt::fillBuf(buf_w, S);
    if (!(co_await mem.write(0u, buf_w)))               co_return fail(40u);
    if (!(co_await mem.write(EB - 1u, std::array<uint8_t, 1u>{LAST})))
      co_return fail(41u);
    if (!(co_await mem.read(0u, buf_r)))                 co_return fail(42u);
    if (!dt::verifySpan({buf_r.data(), EB - 1u}, S))    co_return fail(43u);
    if (buf_r[EB - 1u] != LAST)                          co_return fail(44u);
  }

  // ─── T5: unaligned addr=1, size=3 — check only 3 bytes change ──────────
  {
    constexpr uint32_t S  = 0xDD44'0005u;
    constexpr uint8_t  V0 = 0x11u, V1 = 0x22u, V2 = 0x33u;

    dt::fillBuf(buf_w, S);
    if (!(co_await mem.write(0u, buf_w)))               co_return fail(50u);
    if (!(co_await mem.write(1u, std::array<uint8_t, 3u>{V0, V1, V2})))
      co_return fail(51u);
    if (!(co_await mem.read(0u, buf_r)))                 co_return fail(52u);
    if (!dt::verifySpan({buf_r.data(), 1u}, S))          co_return fail(53u);
    if (buf_r[1u] != V0 || buf_r[2u] != V1 || buf_r[3u] != V2)
      co_return fail(54u);
    if (!dt::verifySpan({buf_r.data() + 4u, EB - 4u}, S, 4u))
      co_return fail(55u);
  }

  // ─── T6: write spanning erase-block boundary ───────────────────────────
  // Write starts at 3Q inside block 0 with size Half+2, crossing into block 1.
  {
    constexpr uint32_t S0 = 0xEE55'0006u;
    constexpr uint32_t S1 = 0xFF66'0006u;
    constexpr uint32_t SW = 0x1177'0006u;

    static constexpr std::size_t SPAN_SIZE  = Half + 2u;
    static constexpr std::size_t B1_WRITTEN = SPAN_SIZE - Quarter;

    static std::array<uint8_t, SPAN_SIZE> span_w{};

    dt::fillBuf(buf_w, S0);
    if (!(co_await mem.write(0u, buf_w))) co_return fail(60u);
    dt::fillBuf(buf_w, S1);
    if (!(co_await mem.write(EB, buf_w))) co_return fail(61u);

    dt::fillBuf(span_w, SW);
    if (!(co_await mem.write(3u * Quarter, span_w))) co_return fail(62u);

    // Verify block 0: prefix untouched, tail rewritten.
    if (!(co_await mem.read(0u, buf_r))) co_return fail(63u);
    if (!dt::verifySpan({buf_r.data(), 3u * Quarter}, S0))
      co_return fail(64u);
    if (!dt::verifySpan({buf_r.data() + 3u * Quarter, Quarter}, SW))
      co_return fail(65u);

    // Verify block 1: head rewritten, tail untouched.
    if (!(co_await mem.read(EB, buf_r))) co_return fail(66u);
    if (!dt::verifySpan({buf_r.data(), B1_WRITTEN}, SW, Quarter))
      co_return fail(67u);
    if (!dt::verifySpan({buf_r.data() + B1_WRITTEN, EB - B1_WRITTEN},
                        S1, B1_WRITTEN))
      co_return fail(68u);
  }

  // ─── T7: write exactly EB starting at block-1 boundary (spans 1 and 2) ─
  {
    constexpr uint32_t S = 0x2288'0007u;
    dt::fillBuf(buf_w, S);

    if (!(co_await mem.write(EB, buf_w))) co_return fail(70u);
    if (!(co_await mem.read(EB, buf_r)))  co_return fail(71u);
    if (buf_r != buf_w)                   co_return fail(72u);
  }

  // ─── T8: write 1 byte at the very last flash address ────────────────────
  {
    constexpr uint8_t LAST = 0xBEu;
    if (!(co_await mem.write(fsz - 1u, std::array<uint8_t, 1u>{LAST})))
      co_return fail(80u);
    if (!(co_await mem.read(fsz - 1u, std::span<uint8_t>{buf_r.data(), 1u})))
      co_return fail(81u);
    if (buf_r[0u] != LAST) co_return fail(82u);
  }

  // ─── T9: arbitrary read — unaligned addr, non-multiple size ─────────────
  {
    constexpr uint32_t S = 0x3399'0009u;
    dt::fillBuf(buf_w, S);
    if (!(co_await mem.write(0u, buf_w))) co_return fail(90u);

    static constexpr std::size_t RD_SIZE = Half + 3u;
    static std::array<uint8_t, RD_SIZE> rd{};
    if (!(co_await mem.read(Quarter + 1u, rd)))  co_return fail(91u);
    if (!dt::verifySpan(rd, S, Quarter + 1u))    co_return fail(92u);
  }

  // ─── Negative tests ─────────────────────────────────────────────────────

  // T10: read at addr == fsz → must return false
  {
    std::array<uint8_t, 1u> tmp{};
    if (co_await mem.read(fsz, tmp)) co_return fail(100u);
  }

  // T11: read where addr+size > fsz → must return false
  {
    std::array<uint8_t, 2u> tmp{};
    if (co_await mem.read(fsz - 1u, tmp)) co_return fail(110u);
  }

  // T12: write at addr == fsz → must return false
  {
    const std::array<uint8_t, 1u> tmp{0u};
    if (co_await mem.write(fsz, tmp)) co_return fail(120u);
  }

  // T13: write where addr+size > fsz → must return false
  {
    const std::array<uint8_t, 2u> tmp{0u, 0u};
    if (co_await mem.write(fsz - 1u, tmp)) co_return fail(130u);
  }

  // T14: write empty span → must return true (no-op)
  {
    std::span<const uint8_t> empty{};
    if (!(co_await mem.write(0u, empty))) co_return fail(140u);
  }

  // T15: read empty span → must return true (no-op)
  {
    std::span<uint8_t> empty{};
    if (!(co_await mem.read(0u, empty))) co_return fail(150u);
  }

  co_return Res{};
}

}  // namespace m::tsts

#endif  // FLASH_MEM_CORO_TEST_HPP

