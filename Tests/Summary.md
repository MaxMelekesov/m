# Tests/ — Tests and Benchmarks

Test fixtures and benchmarks for verifying library components. Not compiled into production firmware — include selectively in test builds.

## Files

### `FlashMemTest.hpp` — `m::tsts::flashMemTest<Erase_Block_Bytes, Write_Block_Bytes>(IFlashMemory&)→Task<expected<void,uint8_t>>`
Comprehensive coroutine test for the raw `IFlashMemory` interface.  
Tests: erase, `writeBlock`, `read` for single page, full erase block, and cross-block patterns. Validates geometry parameters.

---

### `FlashMemCoroTest.hpp` — `m::tsts::flashMemCoroTest<Erase_Block_Bytes>(IMemoryCoro&)→Task<expected<void,uint8_t>>`
Comprehensive coroutine test for `IMemoryCoro` (the `FlashMem` RMW layer).  
Tests: single-byte write, full-block write, cross-boundary write, full-memory sequential write. LCG pseudo-random fill/verify. Returns encoded error codes on failure.

---

### `MemoryTest.hpp` — `m::tsts::memoryTest<Buf_Size>(IMemory&)→bool`
Simple synchronous memory test: write a random buffer at a random address, read back, compare byte-by-byte.

---

### `ProtoThreadTest.hpp`
Unit tests for `ProtoThread`. Covers: basic yield, `PT_WAIT_UNTIL`, `PT_AWAIT` with child, `PT_RETURN` with value, `PtDelay` timing.  
Uses `FakeTimeMs` (a test stub for `ITime`). Multiple `Proto<>` subclasses used as test fixtures.

---

### `FlowNestedBenchmark.hpp`
Benchmark comparing three concurrency mechanisms with deeply nested (15-level) task chains:
- `ChainTask` pipeline
- `CoroScheduler` coroutines
- `ProtoThread`

Measures latency and flash/RAM overhead. Configurable via `BENCH_ENABLE_*` macros.
