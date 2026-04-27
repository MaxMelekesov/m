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

### `IAdcDmaCircularReader.hpp` — `m::ifc::mcu::IAdcDmaCircularReader<T>`
ADC DMA circular buffer reader interface. Supports half/full conversion callbacks for double-buffering.  
**Key methods:** `setHalfConversionCallback(cb)`, `setFullConversionCallback(cb)`, `start(data)→bool`, `running()→bool`, `stop()→bool`  
**Concept:** `CAdcDmaCircularReader`
