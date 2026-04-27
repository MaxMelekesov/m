# Logic/Protocol/ — Communication Protocols

Modbus RTU master and slave implementations, plus a generic async data link layer.  
Slave implementations are coroutine-based (`coroRun()→Task<bool>`). Master supports both polling and coroutine wrappers.

## Files

### `DataLinkAsync.hpp` — `m::DataLinkAsync<UsT, BpsT>` : `IDataLink`
Generic time-gap delimited async data link. Packet boundaries are detected by inter-byte silence.  
Uses `IIO_Async` + microsecond timer (`ITime<Us<>>`).  
**Key methods:** `startReceive`, `getPacket`, `startTransmit`, `transmitDone`, `stopReceive/Transmit`, `error`

---

### `ModbusRtuMaster.hpp` — `m::ModbusRtuMaster<IoT, TimeUsT>`
Modbus RTU master (polling-based). Supports:
- FC03 — Read Multiple Holding Registers
- FC06 — Write Single Holding Register
- FC16 — Write Multiple Holding Registers

**Key methods:** `readMhr(unit, buf)→bool`, `writeShr(unit, buf)→bool`, `writeMhr(unit, data, req_buf, resp_buf)→bool`, `handle()`, `getResponse()→optional<span>`, `checkResponse()`, `changeResponseEndian()`  
**Concept:** `CModbusRtuMaster`

---

### `ModbusRtuMasterWrapper.hpp` — `m::ModbusRtuMasterWrapper<Mdbs, TimeUsT>`
Coroutine wrapper for `ModbusRtuMaster`. Adds inter-frame delay and `CoroMutex` for safe concurrent access from multiple coroutines.  
**Key methods:** `readMhr(addr, reg, num, data)→Task<bool>`, `writeShr(addr, reg, val)→Task<bool>`, `writeMhr(addr, reg, num, data)→Task<bool>`

---

### `ModbusRtuMultiProtocol.hpp` — `m::ModbusRtuMultiProtocol<TimeUsT, PinT, AddrCount=1>` *(recommended slave)*
Modbus RTU slave supporting multiple device addresses. Full FC01–FC21 command set. Coroutine-based.  
Per-address, per-function-code callbacks: `RC_Cb`, `RDI_Cb`, `RMHR_Cb`, `RIR_Cb`, `WSC_Cb`, `WSHR_Cb`, `WMC_Cb`, `WMHR_Cb`.  
**Key methods:** `coroRun()→Task<bool>`, `setAddr(idx, addr)`, `setRunning(bool)`, `addXxxCallback(idx, cb)`

---

### `ModbusRtuStaticProtocol.hpp` — `m::ModbusRtuStaticProtocol<TimeUsT, PinT, Nodes...>`
Compile-time Modbus RTU slave. Device addresses are template parameters (`ModbusAddressNode<Addr, Handler>`). Duplicate-address detection at compile time.  
**Helper:** `makeModbusAddressNode<Addr>(handler)`  
**Key method:** `coroRun()→Task<bool>`

---

### `ModbusRtuProtocol.hpp` — `m::ModbusRtuProtocol<TimeUsT>` *(deprecated — use ModbusRtuMultiProtocol)*
Single-address Modbus RTU slave. Same FC set as `ModbusRtuMultiProtocol`.
