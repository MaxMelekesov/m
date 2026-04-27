# Logic/Nextion/ — Nextion HMI Driver

Driver for Nextion serial HMI displays. Layered into a data link and a high-level display manager.

## Files

### `NextionDataLink.hpp` — `m::nxt::NextionDataLink<UsType, BpsType>` : `CRingDataLink`
Serial data link for Nextion displays. Detects packet boundaries by 3×0xFF terminator with inter-byte timeout. Handles ring buffer wrap-around.  
**Template params:** `UsType` (microsecond time `CTime+CUs`), `BpsType` (`CBps`)  
**Key methods:** `startReceive()`, `getPacket()→optional<RingSpan>`, `startTransmit()`, `transmitDone()→optional<bool>`, `stopReceive/Transmit()`, `error()`

---

### `Nextion.hpp` — `m::nxt::Nextion<IoType, MaxComponents, BufferSize>`
High-level Nextion HMI driver. FSM-based (Idle/Receiving). Manages a component registry and dispatches touch/value events via callbacks.  
**Key methods:** `start/stop()→Task<bool>`, `setPicture/setText/setVisibility/setNumber(component, ...)→Task<bool>`, `addComponent(component*)`  
**Supporting classes:** `m::nxt::Component` (base), `m::nxt::Button`  
**Enums:** `ReturnCode`, `EventType{Release, Press, ValueChanged}`  
**Concept:** `CNextion`
