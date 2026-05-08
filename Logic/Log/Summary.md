# Logic/Log/ — Logging and Error Indication

Logging utilities and visual error indication for embedded systems. All implementations are non-blocking.

## Files

### `SimpleLogger.hpp` — `m::SimpleLogger<MaxStringSize=64, LogSize=100>` : `ILog`
Ring-buffer string logger. Stores full string copies. Good for debug builds.  
**Key methods:** `add(text)`, `getFirst()→optional<string_view>`, `size()`, `empty()`, `full()`, `clear()`

---

### `DebugLogger.hpp` — `m::DebugLogger<LogSize=100>` (singleton)
Ring-buffer logger storing only `const char*` message pointers — no string copies.  
Minimal overhead; suitable for production logging of string literals.  
**Key methods:** `add(msg)`, `getFirst()→optional<const char*>`, `size()`, `empty()`, `full()`, `clear()`

---

### `IO_AsyncLog.hpp` — `m::IO_AsyncLog<Line_Length=63, Lines=100>` : `ILog`
Async UART logger. Buffers lines in a ring buffer, flushes them via `IIO_Async` when the bus is free.  
**Key methods:** `add(text)`, `handle()` (call in main loop), `clear()`

---

### `SimpleErrorTracer.hpp` — `m::SimpleErrorTracer<T, Max_Elements=16>` : `IErrorTracer<T>`
Static-array accumulator for error codes. Stores up to `Max_Elements` values.  
**Key methods:** `add(val)→bool`, `clear()`, `getTrace()→span<T>`

---

### `LogMixin.hpp` — `m::LogMixin<ModuleLevel=Log_Level>`
Zero-overhead compile-time logging mixin. Inherit, wire `ILog&`, call `error()`/`info()`/`debug()`/`trace()`.  
`if constexpr` eliminates calls + strings when `LOG_LEVEL` or `ModuleLevel` is too low.  
`ModuleLevel=None` → storage is `Nil` (0 bytes).  
**Key methods:** `error(msg)`, `warn(msg)`, `info(msg)`, `debug(msg)`, `trace(msg)`

---

### `ErrorLedIndicator.hpp` — `m::ErrorLedIndicator<PinT, TimeT, ErrorT>`
Blink-coded LED error indicator. Encodes an error enum value as a sequence of long/short flashes. FSM-driven.  
`ErrorT` must be an `enum class` with an `ErrorT::Size` member indicating the number of codes.  
**Key methods:** `setError(ErrorT)`, `clearError()`, `hasError()→bool`, `handle()` (call in main loop)
