# Logic/Utility/ — Utilities

Small general-purpose utilities. No dependencies on other library modules.

## Files

### `FinalAction.hpp` — `m::FinalAction<F>`, `m::finally(f)`
RAII scope-exit guard (similar to GSL `finally`). Non-copyable, non-movable.  
**Usage:** `auto g = m::finally([&]{ cleanup(); });`

---

### `ForEachType.hpp` — `m::for_each_type<TList>(func)`, `m::TypeList<Ts...>`
Compile-time iteration over a list of types in `std::tuple` or `TypeList`.  
Calls `func.template operator()<T>()` for each type `T`.

---

### `TSerDes.hpp` — `m::serialize`, `m::deserialize<Ts...>`
Template binary serialization/deserialization via `memcpy`. Packs/unpacks multiple POD values into/from a `span<uint8_t>`.  
**Functions:** `serialize(span, args...)→size_t`, `deserialize<Ts...>(span)→tuple<Ts...>`

---

### `TupleContains.hpp` — `m::tuple_contains<T, Tuple>`
Compile-time concept: checks whether type `T` is a member of `std::tuple<Args...>`.

---

### `short_alloc.hpp` — `arena<N, alignment>`, `short_alloc<T, N, Align>`
Stack-based allocator (Howard Hinnant, MIT). Allocates from a fixed-size buffer on the stack; falls back to `::operator new` on overflow.  
Use with STL containers to avoid heap allocation for small, known-size collections.
