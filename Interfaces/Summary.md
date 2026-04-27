# Interfaces/ — Abstract Interfaces

Pure abstract C++ interfaces. All hardware and logic dependencies are expressed through these interfaces, enabling full decoupling of logic from platform.

Each interface has a corresponding C++ concept (e.g. `CIO_Async`, `CMemory`) for template-constraint usage without virtual dispatch.

See also:
- [`Mcu/`](Mcu/Summary.md) — MCU peripheral interfaces (GPIO, timer, ADC DMA)
- [`StpMotor/`](StpMotor/Summary.md) — Stepper motor subsystem interfaces

## Files

### `IDataLink.hpp` — `IDataLink`, `IRingDataLink`
Serial data link layer interfaces. `IDataLink` for flat buffers; `IRingDataLink` for ring buffers (wrap-around aware, via `RingSpan`).  
**Key methods:** `startReceive`, `getPacket`, `startTransmit`, `transmitDone`, `stopReceive/Transmit`, `error`  
**Concepts:** `CDataLink`, `CRingDataLink`

---

### `IDateTime.hpp` — `m::ifc::IDateTime`
RTC date/time read/write interface.  
**Types:** `Date{year, month, day, weekday}`, `Time{hours, minutes, seconds}`  
**Key methods:** `getDate/setDate`, `getTime/setTime`  
**Concept:** `CDateTime`

---

### `IErrorTracer.hpp` — `m::ifc::IErrorTracer<UnitT>`
Error code accumulator interface. Stores a trace of error values.  
**Key methods:** `add(value)→bool`, `clear()`, `getTrace()→span<Unit>`  
**Concepts:** `CErrorTracer`, `CErrorTracerOf<TracerT, UnitT>`

---

### `IFlashMemory.hpp` — `m::ifc::IFlashMemory`
Coroutine-based erase-block flash memory interface.  
**Key methods:** `size()`, `eraseBlockSize()`, `writeBlockSize()`, `eraseBlock(addr)→Task<bool>`, `writeBlock(addr,data)→Task<bool>`, `read(addr,data)→Task<bool>`  
**Concept:** `CFlashMemory`

---

### `IFlashMemoryAsync.hpp` — `m::ifc::IFlashMemoryAsync` *(deprecated)*
Legacy non-blocking flash interface using start/poll pattern.  
**Key methods:** `startErase/isDone`, `startWrite/isWriteDone`, `startRead/isReadDone`, `error`  
**Concept:** `CFlashMemoryAsync`

---

### `IHash.hpp` — `m::ifc::IHash<hash_bytes=4>`
Hash calculation and verification interface. `type = array<uint8_t, hash_bytes>`.  
**Key methods:** `calc(data)→type`, `check(data,hash)→bool`, `size()→uint32_t`  
**Concepts:** `CHash`, `CHashOf<T, hash_bytes>`

---

### `IIO_Async.hpp` — `m::ifc::IIO_Async<UnitT>`
Non-blocking async IO interface (DMA UART/SPI). `UnitT` is the baud rate type (e.g. `Bps<uint32_t>`).  
**Key methods:** `startWrite/abortWrite/isWriteDone/bytesWritten`, `startRead/abortRead/isReadDone/bytesReaded`, `getBaudrate/setBaudrate`, `error`  
**Concepts:** `CIO_Async`, `CIO_AsyncOf<T, BaudT>`

---

### `IIO_Sync.hpp` — `m::ifc::IIO_Sync<Baudrate>` *(deprecated)*
Synchronous (blocking) IO interface.  
**Key methods:** `write(span)→bool`, `read(span)→bool`, `getBaudrate/setBaudrate`, `error`  
**Concept:** `CIO_Sync`

---

### `ILog.hpp` — `m::ifc::ILog`
Minimal text logging interface.  
**Key methods:** `add(string_view)`, `clear()`  
**Concept:** `CLog`

---

### `IMemory.hpp` — `m::ifc::IMemory`
Synchronous byte-addressed memory interface (EEPROM/RAM/Flash).  
**Key methods:** `size()→size_t`, `write(addr,data)→bool`, `read(addr,data)→bool`  
**Concept:** `CMemory`

---

### `IMemoryCoro.hpp` — `m::ifc::IMemoryCoro`
Coroutine-based memory interface. Async counterpart of `IMemory`.  
**Key methods:** `size()→size_t`, `write(addr,data)→Task<bool>`, `read(addr,data)→Task<bool>`  
**Concept:** `CMemoryCoro`

---

### `IoSyncAdpter.hpp` — `m::ifc::IoSyncAdapter<Baudrate, TimeUnitT, TimeT>` *(deprecated)*
Adapts `IIO_Async` to `IIO_Sync` via timeout polling loop.  
Constructor takes `IIO_Async&`, `ITime&`, and a timeout-calculator lambda.

---

### `ITempSense.hpp` — `m::ifc::ITempSense<Unit>`, `m::ifc::ITempSenseError`
Temperature sensor interface with error flags.  
**Key methods (ITempSense):** `value()`, `min()`, `max()`  
**Key methods (ITempSenseError):** `shorted()`, `broken()`  
**Concepts:** `CTempSense`, `CTempSenseError`

---

### `ITime.hpp` — `m::ifc::ITime<UnitT>`
Time provider interface for delays, timestamps, and elapsed time.  
**Key methods:** `delay(Unit)`, `now()→Unit`, `diff(Unit)→Unit`  
**Concepts:** `CTime`, `CTimeOf<TimeT, UnitT>`
