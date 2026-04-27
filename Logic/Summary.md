# Logic/ — Reusable Logic Blocks

Platform-independent logic components. No MCU-specific code. Depend only on interfaces from `Interfaces/`.

See also subdirectories:
- [`Containers/`](Containers/Summary.md) — Compile-time containers and register abstraction
- [`Filters/`](Filters/Summary.md) — DSP signal filters
- [`FlowControl/`](FlowControl/Summary.md) — Cooperative concurrency, FSM, timers
- [`Hash/`](Hash/Summary.md) — Hash functions
- [`Log/`](Log/Summary.md) — Logging and error indication
- [`Nextion/`](Nextion/Summary.md) — Nextion HMI display driver
- [`Protocol/`](Protocol/Summary.md) — Modbus RTU and async data link
- [`StpMotor/`](StpMotor/Summary.md) — Stepper motor position controllers
- [`Units/`](Units/Summary.md) — Strongly-typed physical units
- [`Utility/`](Utility/Summary.md) — Small utilities

## Files

### `Pid.hpp` — `m::Pid`
PID controller with anti-windup (back-calculation method).  
**Settings:** `{kp, ki, kd, kt, dt, min_out, max_out}`  
**Key methods:** `update(setpoint, measurement)→float`, `getRegulationStep()→{p,i,d}`, `reset()`

---

### `NtcConverter.hpp` — `m::NtcConverter`
Beta-equation NTC thermistor converter using typed units (`Kelvin`, `Ohm`).  
**Constructor:** `(Kelvin<float> b25_100, Ohm<float> r0, Kelvin<float> t0)`  
**Key method:** `getTemperature(Ohm<float>)→Kelvin<float>`

---

### `Settings.hpp` — `m::Settings<Derived, SettingsTags...>` (CRTP)
Synchronous settings manager with persistence and change tracking.  
Derived class provides `saveImpl(storage)` and `loadImpl(storage)`.  
Storage is `TaggedStorage<SettingsTags...>`.  
**Key methods:** `setValue<Tag>(val)`, `getValue<Tag>()`, `hasChanges()→bool`, `save()→bool`, `load()→bool`, `resetToDefaults()`

---

### `CoroSettings.hpp` — `m::CoroSettings<Derived, SettingsTags...>` (CRTP)
Coroutine-based settings manager. Async counterpart of `Settings`.  
**Key methods:** same as `Settings` but `save()→Task<bool>`, `load()→Task<bool>`

---

### `MemoryPart.hpp` — `m::MemoryPart` : `IMemory`
Memory sub-region view. Remaps a slice of another `IMemory` by offset + size. Translates all addresses transparently.  
**Constructor:** `(IMemory&, offset, size)`

---

### `Multipin.hpp` — `Multipin<N>` : `IPin`
Groups N `IPin` instances into one. Write/toggle propagates to all pins; read returns OR of all.  
CTAD deduction guide provided: `Multipin pin(pinA, pinB, pinC)`.
