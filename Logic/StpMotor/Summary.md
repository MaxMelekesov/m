# Logic/StpMotor/ — Stepper Motor Position Controllers

High-level coroutine-based stepper motor position controllers. Depend on `IStepDriver`, `IStepCounter`, `IStepGen` from `Interfaces/StpMotor/`.

## Files

### `SAccCurve.hpp` — `m::SAccCurve`
S-curve (quintic polynomial) acceleration/deceleration profile generator.  
**Constructor:** `(acc_t_limit, min_v_limit, max_v_limit)`  
**Key methods:** `setMinV/setMaxV/setAccT(...)→bool`, `vt(t)→float` (velocity at time t), `st(t)→float` (position at time t)

---

### `LinearStepPositioner.hpp` — `m::LinearStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT>`
Stepper motor position controller with constant-speed (trapezoidal) motion.  
**Key methods:** `startMove(steps)→Task<bool>`, `startMoveTo(pos)→Task<bool>`, `softStop()→bool`, `emgStop()→bool`, `setSpeed/getSpeed()`, `setAutohold()`, `setStopDelay()`, `moving()→bool`

---

### `StepPositioner.hpp` — `m::StepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT>`
Same as `LinearStepPositioner` but uses `SAccCurve` for smooth S-curve acceleration.  
Additional method: `getAccCurve()→SAccCurve&` for tuning the acceleration profile at runtime.

---

### `SCurveStepPositioner.hpp` — `m::SCurveStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT, EndstopT>`
Advanced S-curve positioner with endstop support and on-the-fly target updates.  
Uses coroutines for driver-enable delays and DMA pipeline draining.  
Employs chunked step emission compatible with double-buffered DMA step generators.  
**Key methods:** `startMove(steps)→Task<bool>`, `startMoveTo(pos)→Task<bool>`, `softStop()→bool`, `emgStop()→bool`, `reset(pos)`, `getAccCurve()→SAccCurve&`, `setAutohold()`, `setRunCurrent()/setHoldCurrent()`

---

### `ISRStepPositioner.hpp` — `m::ISRStepPositioner<TimeMsT, StepDriverT, StepCounterT, StepGenT, EndstopT>`
**Minimal ISR-only S-curve positioner.** All motion logic runs inside the ISR callback — users only set atomic flags (no coroutines, no mutex, no epoch tracking).  
The generator runs continuously; idle ticks are dummy 1 kHz pulses.  
Designed for maximum simplicity and safety: the ISR owns all state, users communicate via lock-free atomic flags.  
**Key methods:** `startMove(steps)→void`, `startMoveTo(pos)→void`, `softStop()→void`, `emgStop()→void`, `reset(pos)→void`, `moving()→bool`, `getLoadedPos()→int32_t`, `getAccCurve()→SAccCurve&`, `setAutohold()`, `setDriverEnDelay()`, `setStopDelay()`, `setRunCurrent()/setHoldCurrent()`