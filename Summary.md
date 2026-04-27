# m — Library Summary

Universal C++ library for embedded systems (microcontrollers). Architecture-independent.  
No heap, no macros, no RTTI, no exceptions. Designed for readability, maintainability, and reuse.

## Design Principles

- All hardware dependencies hidden behind abstract interfaces (`Interfaces/`)
- IC drivers implement those interfaces or use them as template parameters
- Logic is portable — no MCU-specific code outside `McuSpecific/`
- Coroutine-based concurrency via `CoroScheduler` + `Task<T>`; lightweight alternative via `ProtoThread`
- Strongly-typed physical units (`Logic/Units/`) prevent unit confusion at compile time

## Modules

| Folder | Description |
|---|---|
| [`IC/`](IC/Summary.md) | Drivers for specific ICs: ADC, capacitance converter, flash memory, OLED, NTC thermistor. Both synchronous and coroutine-based variants. |
| [`Interfaces/`](Interfaces/Summary.md) | Pure abstract C++ interfaces for IO, memory, time, logging, temperature, etc. Decouple logic from hardware. |
| [`Interfaces/Mcu/`](Interfaces/Mcu/Summary.md) | MCU-peripheral interfaces: GPIO pin, timer/interrupt, enable control, ADC DMA circular reader. |
| [`Interfaces/StpMotor/`](Interfaces/StpMotor/Summary.md) | Stepper motor subsystem interfaces: end-stop, step counter, step driver, step generator. |
| [`Logic/`](Logic/Summary.md) | Reusable logic blocks: PID, NTC converter, settings manager, memory view, multi-pin. |
| [`Logic/Containers/`](Logic/Containers/Summary.md) | Compile-time containers: bit-field register, static type→value map, tagged heterogeneous storage. |
| [`Logic/Filters/`](Logic/Filters/Summary.md) | DSP filters: 2nd-order Butterworth LPF, sliding mean, sliding median. |
| [`Logic/FlowControl/`](Logic/FlowControl/Summary.md) | Cooperative concurrency and FSM: CoroScheduler, ProtoThread, ChainTask, FSM variants, timers, delays. |
| [`Logic/Hash/`](Logic/Hash/Summary.md) | FAQ6 32-bit hash implementing `IHash`. |
| [`Logic/Log/`](Logic/Log/Summary.md) | Logging and error indication: async UART logger, ring-buffer logger, blink-coded LED error indicator, error tracer. |
| [`Logic/Nextion/`](Logic/Nextion/Summary.md) | Nextion HMI display driver: serial data link + high-level component/event manager. |
| [`Logic/Protocol/`](Logic/Protocol/Summary.md) | Communication protocols: Modbus RTU master and slave (polling and coroutine-based), async data link. |
| [`Logic/StpMotor/`](Logic/StpMotor/Summary.md) | Stepper motor position control: constant-speed and S-curve acceleration positioners. |
| [`Logic/Units/`](Logic/Units/Summary.md) | Strongly-typed physical units: time, temperature, electrical, mechanical. CRTP-based. |
| [`Logic/Utility/`](Logic/Utility/Summary.md) | Small utilities: RAII scope-exit guard, compile-time type iteration, stack allocator, binary serialization, tuple type check. |
| [`McuSpecific/Stm32/`](McuSpecific/Stm32/Summary.md) | STM32 HAL implementations of abstract interfaces: GPIO pin, timers, UART/DMA, RS-485, internal flash. Variants: G0, F4, G4. |
| [`Tests/`](Tests/Summary.md) | Test fixtures and benchmarks for flash memory, proto-threads, and flow-control mechanisms. |
