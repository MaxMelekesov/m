# IC/ — IC Drivers

Drivers for specific ICs. Each driver is a C++ class (or CRTP template) that wraps all register-level access and exposes a clean typed API.

Two base CRTP classes are provided: `Ic` (synchronous) and `IcCoro` (coroutine-based). Concrete drivers inherit from one of them.

## Files

### `Ic.hpp` — `m::ic::Ic<Derived, IcInfo>`
Synchronous CRTP base for register-level IC access.  
Derived class must implement `readImpl<Reg>()→optional<Reg>` and `writeImpl<Reg>(reg)→bool`.  
`IcInfo` requires: `Regs` (tuple of register types) + `Map` (`StaticMap` of register type → address).  
**Key methods:** `write(Reg)→bool`, `read<Reg>()→optional<Reg>`  
**Concepts:** `CIcInfo`, `CIcRegReadable`, `CIcRegWritable`

---

### `IcCoro.hpp` — `m::ic::IcCoro<Derived, IcInfo>`
Coroutine-based CRTP base. Async counterpart of `Ic`.  
**Key methods:** `write(Reg)→Task<bool>`, `read<Reg>()→Task<optional<Reg>>`  
**Concepts:** `CIcRegReadableCoro`, `CIcRegWritableCoro`

---

### `Ads1256.hpp` — `m::ic::Ads1256Ic<Time, Io>`
Driver for TI ADS1256 24-bit SPI ADC.  
Defines complete register map with bitfields (`ReadStatus`, `ReadMux`, `ReadAdcon`, `ReadDrate`, `Data`, `Selfcal`, `WriteStatus/Mux/Adcon/Drate`).  
Inherits `Ic<>`. SPI communication with CS control.  
**Template params:** `Time` (`CTime`+`CUs`), `Io` (`CIO_Async`+`CBps`)

---

### `B57861S0103F045.hpp` — `m::ic::B57861S0103F045`
NTC thermistor calculator for EPCOS B57861S0103F045 (10 kΩ, B25/100 = 3988 K). Steinhart-Hart equation.  
**Key method:** `resToTemp(float res)→float` — resistance (Ω) → temperature (°C)

---

### `FDC1004.hpp` — `m::ic::Fdc1004Ic<Time, Io>`
Driver for TI FDC1004 capacitance-to-digital converter (4 channels, CAPDAC, gain calibration).  
Register map: `Meas1..4Msb/Lsb`, `ConfMeas1..4`, `FdcConf`, `OffsetCal1..4`, `GainCal1..4`, `Manufacturer`.  
Uses `Fsm_v4` internally.

---

### `FlashMem.hpp` — `m::ic::FlashMem<FlashT, Max_Erase_Block_Bytes=4096>`
Read-Modify-Write adapter over `IFlashMemory`. Implements `IMemoryCoro`.  
Handles erase granularity transparently for arbitrary byte-addressed writes.  
**Key methods:** `size()→size_t`, `read(addr, span)→Task<bool>`, `write(addr, span)→Task<bool>`

---

### `JedecSpiFlash.hpp` — `m::ic::JedecSpiFlash<IoT, TimeT, CsPinT, TotalSize, EraseBlock, WriteBlock, AddrBytes, EraseCmd>`
Generic JEDEC SPI NOR Flash driver (W25Q, AT25, etc.). Implements `IFlashMemory`. Coroutine-based. Supports 3/4-byte addressing.  
**Key methods:** `read`, `eraseBlock`, `writeBlock`, `readJedecId()→Task<optional<array<3>>>`, `probeSizeByJedec()→Task<bool>`

---

### `SSD1306.hpp` — `m::ic::SSD1306<IoT>`
Driver for SSD1306 128×32/64 OLED display over I2C/SPI async IO. Coroutine-based.  
**Key methods:** `init()→Task<bool>`, `deinit()`, `clear()`, `onOff()`, `sendCmd()`, `sendData()`, plus full configuration commands (clock, mux, offset, start line, segment remap, scan direction, column/page range, charge pump, COM pins).
