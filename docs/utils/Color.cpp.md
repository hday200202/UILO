# Color.cpp

`include/utils/Color.cpp`

[← index](../README.md)

## Functions

- [`hexByte(std::string_view s, size_t pos)`](#hexbyte)
- [`fromHex(std::string_view hex)`](#fromhex)

---

### hexByte

```cpp
hexByte(std::string_view s, size_t pos)
```

**Parameters**

- `std::string_view s`
- `size_t pos`

**Returns** — static uint8_t

Parses two hex digits into a byte, returning 0 for anything that is not a hex pair so a malformed string yields black rather than garbage.

---

### fromHex

```cpp
fromHex(std::string_view hex)
```

**Parameters**

- `std::string_view hex`

**Returns** — [Color](Color.hpp.md#color)

Strip leading '#'
