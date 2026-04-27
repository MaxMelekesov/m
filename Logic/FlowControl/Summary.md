# Logic/FlowControl/ — Cooperative Concurrency and FSM

Tools for cooperative multitasking, state machines, and timing without an RTOS.  
Three concurrency models are available:

| Model | Base class | Overhead | Heap | Notes |
|---|---|---|---|---|
| **CoroScheduler** | `Task<T>` / `co_await` | Medium | No (static pool) | Most ergonomic; C++20 coroutines |
| **ProtoThread** | `Proto<Derived>` | Minimal (2 bytes) | No | Duff's-device stackless threads |
| **ChainTask** | `makeChainTask<T,E>(steps...)` | Minimal | No | Linear pipeline without coroutines |

## Files

### `CoroScheduler.hpp` — `m::CoroScheduler`, `m::Task<T>`, `m::CoroFramePool<SlotSize,Capacity>`
Cooperative coroutine scheduler with a static pool allocator (no heap).  
`Task<T>` is `co_await`-able and move-only. Scheduler is a singleton; dispatches in FIFO order.  
Pool defaults: `slot_size=384`, `capacity=16` — override via `CoroTraits<>` specialization.  
**Key methods (scheduler):** `handle()` (call in main loop), `enqueue(handle)`, `setCoroutineOomCallback(cb)`, `coroutineMemoryStats()`

---

### `CoroYield.hpp` — `m::coroYield()→Awaiter`
Core yield primitive for coroutines. Re-enqueues the current coroutine and suspends it.  
All other `coro*` helpers are built on top of this.

---

### `CoroDelay.hpp` — `m::coroDelay(time, delay)→Task<void>`
Coroutine delay: yields until `time.diff(start) >= delay`. Uses `ITime` + `coroUntil`.

---

### `CoroUntil.hpp` — `m::coroUntil(pred)→Task<void>`, `m::coroWhile(pred)→Task<void>`
Spin-yield helpers — suspend coroutine until predicate becomes true/false.

---

### `CoroMutex.hpp` — `m::CoroMutex` + `m::CoroMutex::Guard`
Cooperative mutex for coroutines. Yields while locked; non-blocking for the scheduler.  
**Key methods:** `lock()→Task<Guard>`, `isLocked()→bool`; `Guard::unlock()`

---

### `ChainTask.hpp` — `m::makeChainTask<T,E>(steps...)`
Cooperative pausable pipeline built from linear steps — no coroutines needed. Zero heap. `std::expected`-based error propagation. O(1) step dispatch.  
**Step builders:** `cstep(lambda)`, `cresult(lambda)`, `cawait(subtask)`  
**Step context API:** `ctx.yield()`, `ctx.done()`, `ctx.err(E)`, `ctx.finish(T)`  
**Task methods:** `handle()→bool`, `result()→const expected<T,E>&`, `reset()`

---

### `ProtoThread.hpp` — `m::Proto<Derived,T=void>`, `m::PtScheduler`
Cooperative stackless threads using Duff's device. No heap, 2 bytes state per instance.  
**Macros:** `PT_BEGIN`, `PT_END`, `PT_YIELD`, `PT_WAIT_UNTIL/WHILE`, `PT_AWAIT(child)`, `PT_SPAWN`, `PT_RETURN`, `PT_RESTART`  
**Scheduler:** `PtScheduler::add(task)`, `handle()`, `allDone()→bool`, `clear()`  
**Enum:** `m::PtStatus{Running,Done}`

---

### `PtDelay.hpp` — `m::PtDelay<TimeT>`
ProtoThread delay primitive. Inherits `Proto<PtDelay<TimeT>>`. Used with `PT_AWAIT`.  
**Usage:** `delay.delay(duration); PT_AWAIT(delay);`

---

### `PeriodicTask.hpp` — `m::PeriodicTask<MsT>`
Calls a callback at a fixed period using `Timer`. Supports pause/resume.  
**Key methods:** `handle()` (in main loop), `resume()`, `pause()`, `running()→bool`

---

### `Timer.hpp` — `m::Timer<TimeUnit>`
One-shot software timer backed by `ITime`.  
**Key methods:** `start(val)→bool`, `restart(val)→bool`, `stop()`, `reset()→bool`, `running()→bool`, `timeOver()→bool`, `updateTimeout(val)→bool`

---

### `Timeout.hpp` — `m::execWithTimeout(time, code, timeout)→bool`
Blocking polling loop with timeout. Returns `true` if `code()` returns `true` before timeout expires.

---

### `Fsm_v4.hpp` — `m::Fsm_v4<Derived, InitState, Transitions...>` *(recommended)*
Type-safe FSM using `std::variant` for state storage. Transitions are template parameters.  
Derived class implements `checkEvent(From,Ev)→bool` and `handleEvent(From,Ev)`.  
Optional hooks: `onEvent(Ev)`, `onStateTransition(State)`.  
**Key methods:** `checkEvents()`, `processEvent(Event)`, `isInState<S>()→bool`  
**Types:** `m::State`, `m::Event`, `m::Transition<From,Ev,To>`

---

### `Fsm_v8.hpp` — `m::Fsm_v8<S, E, Ctx, Transitions...>`
Enum-based FSM with compile-time adjacency table. Requires `S::Count` and `E::Count`.  
Validates no duplicate (state,event) pairs and full enum coverage at compile time. Transitions hold member-function pointers `check/handle`.  
**Key method:** `handle(Ctx&)→bool`, `state()→S`

---

### `Fsm.hpp` — `m::Fsm` *(deprecated — use Fsm_v4)*
First-generation FSM. States own transition tables as `span<IState*>`.

---

### `CoroutineTask.hpp` — `m::CoroutineTask<T>` *(deprecated — use CoroScheduler)*
Legacy manual-resume coroutine wrapper.  
**Key methods:** `resume()→bool`, `done()→bool`, `result()→T`

---

### `Fsm_v5.hpp`
Commented-out C++26 reflection prototype. Not functional — design sketch only.
