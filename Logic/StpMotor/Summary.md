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
