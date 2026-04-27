# Interfaces/StpMotor/ — Stepper Motor Interfaces

Abstract interfaces for the stepper motor subsystem. Decouple motion control logic (`Logic/StpMotor/`) from hardware drivers.

## Files

### `IStepDriver.hpp` — `m::ifc::IStepDriver<mAT>`
Stepper motor driver control: enable/sleep/reset, direction, microstep resolution, current.  
**Template param:** `mAT` — current unit type (e.g. `mA<uint16_t>`)  
**Enums:** `Microstep{M_1..M_256}`, `Dir{Forward,Backward}`, `DirInversion`  
**Concept:** `CStepDriver`

---

### `IStepGen.hpp` — `m::ifc::IStepGen`
Step pulse generator interface. Callback-driven frequency and step-count sequencing.  
**Type:** `Step{freq, steps, dummy}` — one segment of a motion profile  
**Type:** `NextStepCallback` — called to get the next `Step`  
**Key methods:** `setCallback(cb)`, `start/stop/running()`, `maxPeriod()→uint32_t`, `maxSteps()→uint32_t`  
**Concept:** `CStepGen`

---

### `IStepCounter.hpp` — `m::ifc::IStepCounter`
Step counter (encoder or step pulse counter) interface.  
**Key methods:** `start/stop/running()`, `setDirection/getDirection()`, `setDirectionInversion()`, `getCount/setCount(int32_t)`  
**Enums:** `Dir{Up,Down}`, `DirInversion{No,Yes}`  
**Concept:** `CStepCounter`

---

### `IEndstop.hpp` — `m::ifc::IEndStop`
End-stop switch controller for left/right limit switches.  
**Key methods:** `getSwitchesState()→State`, `setCallbacks(left, right)`, `setSwapSwitches`, `setIgnoreSwL/R`, `setActiveLevelSwL/R`  
**Concept:** `CEndStop`
