# Logic/Filters/ — Signal Filters

Sliding-window and IIR signal filters for embedded DSP. All stateless between `clear()` calls. Template parameter `T` is the value type; `N` is the maximum window size.

## Files

### `ButterworthLPF.hpp` — `m::ButterworthLPF<T, CutoffHz, SampleRateHz>`
2nd-order Butterworth low-pass IIR filter. Coefficients computed at compile time.  
**Constraints:** `std::floating_point T`, `CutoffHz < SampleRateHz / 2` (Nyquist enforced statically).  
**Key methods:** `add(x)→T`, `reset()`

---

### `MeanFilter.hpp` — `m::MeanFilter<T, N>`
Sliding window arithmetic mean filter. Window size configurable at runtime up to template `N`.  
**Key methods:** `add(val)→T`, `setWindowSize(size)→bool`, `getWindowSize()→size_t`, `clear()`

---

### `MedianFilter.hpp` — `m::MedianFilter<T, N>`
Sliding window median filter (sort-based). Good for spike rejection.  
**Key methods:** `add(val)`, `getValue()→T`, `clear()`
