# Logic/Hash/ — Hash Functions

## Files

### `HashFAQ6.hpp` — `m::HashFAQ6` : `IHash<4>`
FAQ6 (Jenkins-family) 32-bit non-cryptographic hash function.  
**Key methods:** `calc(data)→Hash`, `check(data,hash)→bool`  
Suitable for data integrity checks in embedded systems (settings CRC, flash verification).
