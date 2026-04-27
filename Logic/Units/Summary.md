# Logic/Units/ — Strongly-Typed Physical Units

Strongly-typed wrappers for physical quantities. Prevent accidental mixing of incompatible units at compile time.

All units are CRTP-based: `struct UnitName<T> : Unit<UnitName<T>, T>`.  
`Unit<Derived, Storage>` provides: arithmetic operators (`+`, `-`, `+=`, `-=`, `++`, `--`, spaceship `<=>`), explicit constructor from raw value, `value()` accessor.

Each unit type has a corresponding C++ concept for use in template constraints.

## Unit Types

| File | Type | Concept | Physical quantity |
|---|---|---|---|
| `Unit.hpp` | `Unit<Derived, Storage>` | — | CRTP base |
| `Bps.hpp` | `Bps<T>` | `CBps` | Bits per second (baud rate) |
| `Celsius.hpp` | `Celsius<T>` | `CCelsius` | Temperature in °C |
| `Kelvin.hpp` | `Kelvin<T>` | `CKelvin` | Temperature in K |
| `Gram.hpp` | `Gram<T>` | `CGram` | Mass in grams |
| `Micrometre.hpp` | `uM<T>` | `CuM` | Length in µm |
| `MilliAmpere.hpp` | `mA<T>` | `CmA` | Current in mA |
| `MilliVolt.hpp` | `mV<T>` | `CmV` | Voltage in mV |
| `Ohm.hpp` | `Ohm<T>` | `COhm` | Resistance in Ω |
| `Ms.hpp` | `Ms<T>` | `CMs` | Time in milliseconds |
| `Us.hpp` | `Us<T>` | `CUs` | Time in microseconds |
| `Second.hpp` | `Sec<T>` | `CSec` | Time in seconds |
| `Minute.hpp` | `Min<T>` | `CMin` | Time in minutes |
| `Hour.hpp` | `Hour<T>` | `CHour` | Time in hours |

## Conversion Utilities

### `TimeUnits.hpp`
Constexpr conversion functions between all time units:  
`toHour()`, `toMin()`, `toSec()`, `toMs()`, `toUs()` — all templated, work with any time unit type.
