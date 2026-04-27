# Logic/Containers/ — Compile-Time Containers

Zero-overhead compile-time data structures for type-safe hardware register access and heterogeneous storage.

## Files

### `Reg.hpp` — `m::Reg<Storage, Fields...>`, `m::BitField<Derived, Size>`, `m::UnusedField<Size>`
Compile-time bit-field register abstraction for hardware registers.  
`BitField<Derived, Size>` — base for named bit fields via CRTP. `UnusedField<Size>` — padding.  
`Reg<Storage, Fields...>` — assembles fields into a typed register with a raw storage type.  
**Key methods:** `get<Field>()→value`, `set<Field>(val)`, `getRaw()→Storage`, `setRaw(Storage)`  
**Concepts:** `CBitField`, `CRegStorage`

---

### `StaticMap.hpp` — `m::StaticMap<ValueType, Pairs...>`, `m::Pair<Key, Value>`
Compile-time type-to-value map (e.g., register type → hardware address).  
No runtime overhead. Narrowing-conversion checked at compile time.  
**Key method:** `static constexpr value<Key>()→ValueType`  
**Concept:** `CStaticMap`

---

### `TaggedStorage.hpp` — `m::TaggedStorage<Tags...>`, `m::Tag<Type, DefaultValue>`
Type-safe heterogeneous key-value storage with compile-time keys (tags).  
Sequential in-memory layout — no map overhead. Default values defined in tag types.  
**Key methods:** `get<Tag>()→value`, `set<Tag>(val)`, `getRef<Tag>()→value&`  
**Concepts:** `CTag`, `CIsStorageTag`, `CIsTagValueType`
