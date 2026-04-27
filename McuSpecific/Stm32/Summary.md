# McuSpecific/Stm32/ — STM32 HAL Implementations

Platform-specific implementations of abstract interfaces from `Interfaces/` using STM32 HAL. Three MCU families are supported, each in its own subdirectory.

## STM32 G0 — `G0/`

| File | Class | Implements | Notes |
|---|---|---|---|
| `Pin_G0.hpp` | `Pin` | `IPin` | Full GPIO: enums `PinNum`, `Mode`, `Pull`, `Speed`, `InitState`, `Inversion`. HAL init/read/write/toggle. |
| `Pin_G0_Wrapper.hpp` | `PinWrapper` | `IPin` | Lightweight: init inline in constructor, no enums needed. |
| `Time_G0_Tim17.hpp` | `TimeUs` | `ITime<Us<uint16_t>>` | TIM17 microsecond timer (16-bit, wrap-around safe). |
| `Time_G0_Tim17.hpp` | `TimeMs` | `ITime<Ms<uint32_t>>` | `HAL_GetTick()` millisecond timer. |
| `Usart.hpp` | `Usart` | `IIO_Async<Bps<uint32_t>>` | DMA UART. Uses `CNDTR` for progress tracking. |
| `UsartRs485.hpp` | `UsartRs485` | `IIO_Async<Bps<uint32_t>>` | Same as `Usart` + DE/RE pin control for RS-485. |
| `FlashMem_G070CB.hpp` | `FlashMem_G070CB` | `IMemory` | Internal flash last-page R/W for STM32G070CB (64-bit double-word HAL programming). |

---

## STM32 F4 — `F4/`

| File | Class | Implements | Notes |
|---|---|---|---|
| `Pin_F4.hpp` | `Pin` | `IPin` | Same structure as G0 variant; includes `stm32f4xx_hal.h`. |
| `Pin_F4_Wrapper.hpp` | `PinWrapper` | `IPin` | Lightweight wrapper for F4. |
| `Time_F4_Tim4.hpp` | `TimeUs` | `ITime<Us<uint16_t>>` | TIM4 16-bit microsecond timer. |
| `Time_F4_Tim4.hpp` | `TimeMs` | `ITime<Ms<uint32_t>>` | `HAL_GetTick()`. |
| `Time_F4_Tim5.hpp` | `TimeUs` | `ITime<Us<uint32_t>>` | TIM5 32-bit microsecond timer (no overflow for ~4295 s). |
| `Usart.hpp` | `Usart` | `IIO_Async<Bps<uint32_t>>` | DMA UART for F4. Uses `NDTR`. |
| `UsartRs485.hpp` | `UsartRs485` | `IIO_Async<Bps<uint32_t>>` | F4 RS-485 UART with DE/RE control. |

---

## STM32 G4 — `G4/`

| File | Class | Implements | Notes |
|---|---|---|---|
| `Pin_G4.hpp` | `Pin` | `IPin` | GPIO for G4. Includes `GpioRcc` RAII helper that auto-enables/disables peripheral clock via reference counting. |
| `Pin_G4_Wrapper.hpp` | `PinWrapper` | `IPin` | Lightweight GPIO wrapper for G4. |
| `Time_G4_Tim17.hpp` | `TimeUs` | `ITime<Us<uint16_t>>` | TIM17 16-bit microsecond timer. |
| `Time_G4_Tim5.hpp` | `TimeUs` | `ITime<Us<uint32_t>>` | TIM5 32-bit microsecond timer. |
| `Usart.hpp` | `Usart` | `IIO_Async<Bps<uint32_t>>` | DMA UART for G4 (uses `CNDTR`). |
| `UsartRs485.hpp` | `UsartRs485` | `IIO_Async<Bps<uint32_t>>` | G4 RS-485 UART with DE/RE pin. |
