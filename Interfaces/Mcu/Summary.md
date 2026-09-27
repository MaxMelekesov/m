# Interfaces/Mcu/ — MCU Peripheral Interfaces

Abstract interfaces for MCU peripherals. Used by `Logic/` and `IC/` classes to remain platform-independent. Implemented by classes in `McuSpecific/`.

## Files

### `IPin.hpp` — `m::ifc::mcu::IPin`
Single GPIO pin interface.  
**Key methods:** `write(bool)`, `read()→bool`, `toggle()`  
**Concept:** `CPin`

---

### `IEnable.hpp` — `m::ifc::mcu::IEnable`
Enable/disable control for a peripheral.  
**Key methods:** `enable()→bool`, `isEnabled()→bool`, `disable()→bool`  
**Concept:** `CEnable`

---

### `IIt.hpp` — `m::ifc::mcu::IIt`
Timer or periodic interrupt interface with callback.  
**Key methods:** `setCallback(fn)`, `start()→bool`, `running()→bool`, `stop()→bool`  
**Concept:** `CIt`

---

### `IPeriodicIt.hpp` — `m::ifc::mcu::IPeriodicIt<UnitT>`
Timer interrupt firing at a run-time adjustable rate: the callback/start/stop contract of `IIt` plus frequency control. Parameterised by the frequency unit, so the storage type stays in the unit and out of the signatures. `using Unit = UnitT` (e.g. `m::Hz<uint32_t>`; `Hz` itself lives in `Logic/Units`).  
**Key methods:** `setCallback(fn)`, `setFrequency(value)→bool` (false → rate unreachable, previous kept; safe to call while running), `getFrequency()→Unit`, `start()→bool`, `running()→bool`, `stop()→bool`  
**Concepts:** `CPeriodicItOf<T, UnitT>`, `CPeriodicIt` (also satisfies `CIt`)

---

### `IEndstop.hpp` — `m::ifc::mcu::IEndstop`
Mechanical end position switch (limit switch): the `IIt` interrupt contract plus the switch state, so both the level and the event are available. Used for homing/travel calibration.  
**Key methods:** `pressed()→bool` (closed right now), `triggered()→bool` (pressed since the last `clear()`), `clear()`, plus `setCallback(fn)`, `start()`, `running()`, `stop()` from `IIt`  
**Concept:** `CEndstop`

---

### `IAdcDmaCircularReader.hpp` — `m::ifc::mcu::IAdcDmaCircularReader<T>`
ADC DMA circular buffer reader interface. Supports half/full conversion callbacks for double-buffering.  
**Key methods:** `setHalfConversionCallback(cb)`, `setFullConversionCallback(cb)`, `start(data)→bool`, `running()→bool`, `stop()→bool`  
**Concept:** `CAdcDmaCircularReader`
