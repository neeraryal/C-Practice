# C Structs — Complete Mental Model (Systems / Firmware)

---

## 1. What a Struct Really Is

A struct is:
- A compile-time defined layout
- A contiguous block of memory
- With fixed byte offsets for each member
- Possibly containing padding for alignment

A struct is **NOT**:
- An object
- A class
- A runtime entity
- A container with behavior

**Mental equation:**
```
struct_instance = base_address + known_offsets
```

---

## 2. Struct Memory Layout (The Only Rules)

- **RULE 1:** Members are laid out in declaration order.
- **RULE 2:** Each member starts at an address aligned for its type.
- **RULE 3:** Padding may be inserted between members.
- **RULE 4:** The total struct size is padded to the MAX alignment of any member (tail padding).

Alignment is ABI-dependent.

**Typical 64-bit LP64 ABI:**

| Type   | Size | Align |
|--------|------|-------|
| `char` | 1    | 1     |
| `int`  | 4    | 4     |
| `long` | 8    | 8     |
| `ptr`  | 8    | 8     |

---

## 3. How to Compute Layout (Algorithm)

```
offset = 0
for each member:
    offset = round_up(offset, alignment(member))
    place member at offset
    offset += sizeof(member)

final_size = round_up(offset, max_alignment_in_struct)
```

---

## 4. Example: Memory Layout

```c
struct Q {
    int  a;
    char b;
    long c;
    char d;
    int  e;
    int  f;
};
```

**64-bit system (LP64):**

```
a  @  0– 3
b  @  4
   pad @ 5–7
c  @  8–15
d  @ 16
   pad @ 17–19
e  @ 20–23
f  @ 24–27
   tail pad @ 28–31

sizeof(struct Q) = 32 bytes
```

**Key insight:**
- Adding a 4-byte field can increase size by 8 bytes.
- Size is governed by MAX alignment, not the last field.

---

## 5. Why Member Order Matters

Bad order = more padding. Good order = fewer gaps.

> **Rule:** Place high-alignment members first, low-alignment members last.

This affects:
- Memory usage
- Cache behavior
- DMA correctness
- Binary compatibility

---

## 6. Dot (`.`) vs Arrow (`->`)

Given:
```c
struct S s;
struct S *p = &s;
```

`s.x` means:
```c
*(type *)((char *)&s + offsetof(struct S, x))
```

`p->x` means:
```c
*(type *)((char *)p + offsetof(struct S, x))
```

**Identity:**
```c
p->x  ==  (*p).x
```

**Rule:**
- Use `.` when you have the object
- Use `->` when you have a pointer

---

## 7. `offsetof` — Critical Tool

```c
#include <stddef.h>

offsetof(struct S, member)
```

Returns:
- Byte offset of `member` from struct start
- Compile-time constant
- Padding-aware
- ABI-correct

> Never hardcode offsets manually.

---

## 8. `container_of` Pattern

**Problem:** Given a pointer to a struct member, recover pointer to the containing struct.

**Canonical macro:**

```c
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
```

Used in:
- Kernels
- Drivers
- Linked lists
- Callback systems
- RTOS internals

---

## 9. Const + Struct Pointers (Non-Negotiable)

```c
const struct S *p
    // Pointer can change
    // Struct data is READ-ONLY

struct S * const p
    // Pointer cannot change
    // Struct data is mutable

const struct S * const p
    // Neither pointer nor data can change
```

> **Rule:** `const` protects DATA, not execution.

Calling a function via a `const` struct pointer is allowed.

---

## 10. Structs + Function Pointers (Interfaces)

Structs can define behavior contracts.

```c
typedef struct {
    int (*init)(void);
    int (*read)(int addr);
    int (*write)(int addr, int val);
} DriverOps;
```

**Usage:**
- Multiple implementations
- Same interface
- Dynamic dispatch in C

> Always pass interface tables as: `const DriverOps *`

---

## 11. Struct Lifetime (Most Bugs Live Here)

**Three lifetimes:**

**1) Automatic (stack)**
- Lives until function returns
- Fast, dangerous
- **NEVER return a pointer to it**

**2) Static**
- Exists for entire program
- Zero-initialized
- Preferred in firmware

**3) Dynamic (heap)**
- Lives until `free()`
- Manual ownership
- Often avoided in embedded

> **Golden rule:** A pointer must **NEVER** outlive the object it points to.

---

## 12. Common Lifetime Bugs

- ❌ Returning pointer to stack struct
- ❌ Storing pointer to temporary object
- ❌ Use-after-free
- ❌ Undocumented ownership
- ❌ ISR accessing stack data

---

## 13. Ownership Rules

- If a function **TAKES** a pointer → caller owns the object unless stated otherwise
- If a function **RETURNS** a pointer → ownership **MUST** be documented

> Ambiguous ownership = guaranteed bug.

---

## 14. Firmware-Specific Reality

- Stack is small
- Heap is risky or disabled
- Static/global lifetime preferred
- ISR-accessed data must be static
- `volatile` may be required (separate topic)

**Typical pattern:**

```c
static Device dev;
dev.ops = &driver_ops;
```

---

## 15. Final Mental Checklist

Before using or passing a struct pointer:

1. Where was it allocated?
2. What is its lifetime?
3. Who owns it?
4. Can an ISR access it?
5. Is `const` used correctly?
6. Is layout ABI-safe?

---

*End of File*