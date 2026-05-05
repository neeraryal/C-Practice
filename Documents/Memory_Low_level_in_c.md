# Memory & Low-Level C — Complete Firmware Engineer Reference

# PART 1 — MEMORY SEGMENTS

## 1.1 The Five Segments (Mental Map)

When your C program is compiled and loaded onto a microcontroller or system, memory is divided into distinct regions. Understanding what lives where is non-negotiable.

```
HIGH ADDRESS
┌─────────────────────────────┐
│           STACK             │  ← grows downward
│  (local vars, return addr)  │
├─────────────────────────────┤
│             ↓               │
│        (free space)         │
│             ↑               │
├─────────────────────────────┤
│            HEAP             │  ← grows upward (malloc/free)
├─────────────────────────────┤
│            BSS              │  ← uninitialized globals/statics (zero-filled)
├─────────────────────────────┤
│            DATA             │  ← initialized globals/statics
├─────────────────────────────┤
│            TEXT             │  ← compiled machine code (read-only)
├─────────────────────────────┤
│           RODATA            │  ← string literals, const globals
LOW ADDRESS
```

---

## 1.2 Each Segment — Deep Dive

### `.text` — Code Segment

- Contains: compiled machine instructions
- Permissions: read + execute (no write)
- Lifetime: entire program
- Lives in: **FLASH** on microcontrollers (MCU)
- Size: fixed at compile time

```c
void foo(void) { ... }   // foo's instructions live in .text
```

> On MCUs, code executes directly from Flash (XIP — Execute In Place), or is copied to RAM for speed (time-critical ISRs).

---

### `.rodata` — Read-Only Data

- Contains: `const` globals, string literals
- Permissions: read-only
- Lives in: **FLASH** on MCUs
- Writing to it = **UNDEFINED BEHAVIOR** (may trigger HardFault on Cortex-M)

```c
const int MAX = 100;             // .rodata
const char *msg = "hello";       // "hello" in .rodata, msg pointer in .data
char arr[] = "hello";            // "hello" copied to .data (writable copy)
```

**Critical distinction:**

```c
const char *p = "firmware";   // string literal in .rodata — do NOT modify via p
char buf[] = "firmware";      // stack copy — safe to modify
```

---

### `.data` — Initialized Data Segment

- Contains: global and static variables **with non-zero initializers**
- Permissions: read + write
- Stored in: FLASH (initial values), **copied to RAM at startup**
- Lifetime: entire program

```c
int x = 42;               // .data
static int count = 10;    // .data
```

**Startup sequence:** The startup code (before `main()`) copies `.data` from Flash to RAM using the load address (LMA) vs virtual address (VMA) in the linker script.

---

### `.bss` — Block Started by Symbol

- Contains: global and static variables **with zero or no initializer**
- Permissions: read + write
- Stored in: **NOT stored in Flash** (only size is recorded)
- Initialized to: zero, by startup code
- Lives in: RAM

```c
int y;              // .bss  (zero-initialized)
static int z = 0;  // .bss  (explicitly zero — same result)
```

**Why BSS exists:** No need to store zeros in Flash. Saves Flash space. Startup code zeroes the BSS region before `main()`.

---

### Stack

- Contains: local variables, function parameters, return addresses, saved registers
- Grows: **downward** (high → low address) on most architectures (ARM, x86)
- Size: fixed and small (typically 1–8 KB on MCUs)
- Managed: automatically by the compiler (push/pop)
- Lifetime: until function returns

```c
void foo(void) {
    int local = 5;      // stack
    char buf[64];       // stack — 64 bytes consumed
}                       // all gone when foo returns
```

**Stack frame layout (ARM Cortex-M, simplified):**

```
┌─────────────────┐  ← SP before call
│   return addr   │
│   saved regs    │
│   local vars    │
│   padding       │
└─────────────────┘  ← SP after function entry
```

**Stack overflow:** No OS protection on bare-metal MCUs. Overflowing the stack silently corrupts adjacent memory. Detection requires watermarking or MPU configuration.

---

### Heap

- Contains: dynamically allocated memory (`malloc`, `calloc`, `realloc`)
- Grows: upward (low → high address)
- Managed: manually (you must `free()` everything you `malloc()`)
- Size: whatever remains between stack and BSS/data

**Why heap is risky in firmware:**
- Non-deterministic allocation time
- Fragmentation over time
- No guaranteed success
- Leaks are fatal in long-running systems
- Most safety standards (MISRA, DO-178C) ban heap allocation

> **Firmware rule:** Prefer static or stack allocation. If heap is needed, use fixed-size memory pools instead.

---

## 1.3 Segment Cheat Sheet

| Segment  | Stored In (MCU) | Lives In (MCU) | Writable | Lifetime    | Contents                        |
|----------|-----------------|-----------------|----------|-------------|---------------------------------|
| `.text`  | Flash           | Flash (XIP)     | No       | Program     | Machine code                    |
| `.rodata`| Flash           | Flash           | No       | Program     | `const` globals, string literals|
| `.data`  | Flash (init)    | RAM             | Yes      | Program     | Initialized globals/statics     |
| `.bss`   | —               | RAM             | Yes      | Program     | Zero-initialized globals/statics|
| Stack    | —               | RAM             | Yes      | Per-function| Local vars, return addresses    |
| Heap     | —               | RAM             | Yes      | Manual      | `malloc`/`free` allocations     |

---

# PART 2 — `volatile` KEYWORD

## 2.1 What `volatile` Really Means

> **`volatile` tells the compiler: "Do NOT optimize reads/writes to this variable. Every access MUST go to memory."**

Without `volatile`, the compiler is free to:
- Cache a variable in a register and never re-read it
- Eliminate "redundant" reads or writes it thinks are unnecessary
- Reorder accesses for optimization

This is correct behavior for normal variables. It is **catastrophically wrong** for:
- Memory-mapped hardware registers
- Variables shared between ISRs and main code
- Variables modified by DMA
- Delay loops

---

## 2.2 Case 1 — Memory-Mapped I/O Registers

Hardware registers are not normal memory. Reading or writing them has **side effects**.

```c
// WRONG — compiler may optimize away the repeated reads
uint32_t *status_reg = (uint32_t *)0x40001000;
while (*status_reg == 0) { }   // compiler: "it was 0 before, it's still 0" → infinite loop or no loop

// CORRECT
volatile uint32_t *status_reg = (volatile uint32_t *)0x40001000;
while (*status_reg == 0) { }   // every iteration re-reads from hardware
```

**Real HAL pattern:**

```c
#define UART_SR   (*(volatile uint32_t *)0x40011000)
#define UART_DR   (*(volatile uint32_t *)0x40011004)

// Wait for transmit buffer empty
while (!(UART_SR & (1 << 7))) { }
UART_DR = 'A';
```

---

## 2.3 Case 2 — ISR-Shared Variables

An ISR can modify a variable at any time. Without `volatile`, the main loop may never see the update.

```c
// WRONG
uint8_t flag = 0;

void UART_IRQHandler(void) {
    flag = 1;
}

void main(void) {
    while (!flag) { }   // compiler caches flag in register → never sees ISR update
}

// CORRECT
volatile uint8_t flag = 0;

void UART_IRQHandler(void) {
    flag = 1;
}

void main(void) {
    while (!flag) { }   // re-reads flag from RAM every iteration
}
```

---

## 2.4 Case 3 — DMA Buffers

DMA writes to RAM without the CPU knowing. The CPU may read stale cached values.

```c
volatile uint8_t dma_buffer[64];  // DMA writes here; CPU must not cache reads
```

> **Note:** On systems with data caches (Cortex-M7, A-series), `volatile` alone is NOT enough. You also need cache invalidation. `volatile` only prevents compiler optimization — it does not handle hardware cache coherency.

---

## 2.5 Case 4 — Delay Loops

```c
// WRONG — compiler eliminates this as "useless"
for (int i = 0; i < 10000; i++) { }

// CORRECT
for (volatile int i = 0; i < 10000; i++) { }
```

---

## 2.6 What `volatile` Does NOT Do

| Misconception | Truth |
|---|---|
| `volatile` makes access atomic | FALSE — a 32-bit read on an 8-bit bus is still non-atomic |
| `volatile` provides memory barriers | FALSE — use `__DMB()`, `__DSB()`, `__ISB()` for barriers |
| `volatile` replaces mutexes | FALSE — use critical sections for multi-access protection |
| `volatile` handles cache coherency | FALSE — requires explicit cache flush/invalidate on cached systems |

---

## 2.7 `volatile` + `const` Together

```c
const volatile uint32_t *RO_REG = (const volatile uint32_t *)0x40002000;
```

**Meaning:** Hardware register that:
- Changes on its own (`volatile`) — must always be re-read
- Must not be written (`const`) — read-only hardware register

This is the correct declaration for status registers.

---

# PART 3 — `static` KEYWORD

## 3.1 The Three Completely Different Meanings

`static` in C has three distinct meanings depending on context. This is one of the most misunderstood keywords.

---

## 3.2 Meaning 1 — Static Storage Duration (inside a function)

```c
void counter(void) {
    static int count = 0;   // initialized ONCE, persists across calls
    count++;
    printf("%d\n", count);
}

counter();  // prints 1
counter();  // prints 2
counter();  // prints 3
```

**Memory:** Lives in `.data` or `.bss` (not stack). Zero-initialized if no initializer given.

**Use in firmware:**
```c
void debounce(void) {
    static uint32_t last_tick = 0;
    uint32_t now = get_tick();
    if (now - last_tick > 50) {
        last_tick = now;
        // process button press
    }
}
```

---

## 3.3 Meaning 2 — Internal Linkage (file scope)

```c
// file: uart.c

static int baud_rate = 115200;   // ONLY visible inside uart.c

static void uart_reset(void) {   // ONLY callable inside uart.c
    // ...
}
```

**Effect:** The symbol is NOT exported. Other `.c` files cannot access it even with `extern`.

**Why this matters:**
- Encapsulation in C (no classes, use `static` for module-private symbols)
- Prevents symbol name collisions across files
- Compiler can optimize more aggressively (knows all callers)
- **Always use `static` for helper functions not part of your module's API**

---

## 3.4 Meaning 3 — Static in a struct/array declaration (size specification — rare)

```c
// C99 — function parameter
void process(int arr[static 10]);  // caller MUST pass array of at least 10 elements
```

This is rare but valid. The `static` here is an annotation for the compiler, not a storage specifier.

---

## 3.5 `static` in Firmware — Canonical Pattern

```c
// sensor.c — complete module encapsulation

typedef struct {
    uint16_t raw_value;
    float    calibrated;
    bool     initialized;
} SensorState;

static SensorState s_sensor;          // module-private state
static bool        s_initialized = false;

static uint16_t read_adc(void) {      // private helper
    return ADC1->DR;
}

// Public API (declared in sensor.h)
bool sensor_init(void) {
    s_sensor.initialized = true;
    s_initialized = true;
    return true;
}

float sensor_read(void) {
    if (!s_initialized) return 0.0f;
    s_sensor.raw_value = read_adc();
    s_sensor.calibrated = s_sensor.raw_value * 0.0033f;
    return s_sensor.calibrated;
}
```

> **Convention:** Prefix module-static variables with `s_` to make their scope visually obvious in code reviews.

---

# PART 4 — BIT MANIPULATION

## 4.1 The Six Fundamental Operations

These are the bread and butter of firmware. Every register operation uses these.

```c
uint32_t reg = 0xABCD1234;
int n = 5;   // bit position (0-indexed from LSB)

// SET bit n
reg |= (1U << n);

// CLEAR bit n
reg &= ~(1U << n);

// TOGGLE bit n
reg ^= (1U << n);

// READ / TEST bit n
if (reg & (1U << n)) { /* bit is set */ }

// WRITE a specific value (0 or 1) to bit n
reg = (reg & ~(1U << n)) | ((value & 1U) << n);

// EXTRACT a multi-bit field
// bits [7:4] — 4 bits starting at position 4
uint32_t field = (reg >> 4) & 0xF;
```

---

## 4.2 Multi-Bit Field Operations

```c
#define FIELD_SHIFT   4
#define FIELD_MASK    (0xF << FIELD_SHIFT)   // 4-bit field at bits [7:4]

// Read field
uint32_t val = (reg & FIELD_MASK) >> FIELD_SHIFT;

// Write field (without touching other bits)
reg = (reg & ~FIELD_MASK) | ((new_val << FIELD_SHIFT) & FIELD_MASK);
```

**Why `& FIELD_MASK` after shifting:** Prevents out-of-range `new_val` from corrupting adjacent bits.

---

## 4.3 Bit Manipulation in Real Register Access

```c
// STM32-style GPIO register manipulation
// Enable GPIOA clock (bit 0 of RCC_AHB1ENR)
RCC->AHB1ENR |= (1U << 0);

// Set PA5 as output (bits [11:10] = 01 in GPIOA_MODER)
GPIOA->MODER &= ~(0x3U << 10);   // clear
GPIOA->MODER |=  (0x1U << 10);   // set to output mode

// Toggle PA5
GPIOA->ODR ^= (1U << 5);

// Read PA5 input
if (GPIOA->IDR & (1U << 5)) { /* high */ }
```

---

## 4.4 Bit Fields in Structs

C allows defining struct members at bit-level resolution.

```c
typedef struct {
    uint32_t enable   : 1;   // 1 bit
    uint32_t mode     : 2;   // 2 bits
    uint32_t speed    : 3;   // 3 bits
    uint32_t reserved : 26;  // padding to 32 bits
} ControlReg;

ControlReg ctrl = {0};
ctrl.enable = 1;
ctrl.mode   = 2;
ctrl.speed  = 5;
```

**WARNING — Bit field caveats for firmware:**

| Issue | Detail |
|---|---|
| Bit ordering | Not guaranteed by C standard — compiler-specific |
| Padding/layout | Not guaranteed across compilers or ABIs |
| Atomicity | Multi-bit fields are NOT atomically read/written |
| `volatile` | `volatile` on bit fields has implementation-defined behavior |

> **Rule:** Never use bit fields for hardware register mapping. Use manual bit manipulation with masks. Bit fields are acceptable for internal software state that is not memory-mapped.

---

## 4.5 Useful Bit Tricks

```c
// Check if value is a power of 2
bool is_pow2 = (n > 0) && ((n & (n - 1)) == 0);

// Round up to next power of 2
uint32_t next_pow2(uint32_t v) {
    v--;
    v |= v >> 1; v |= v >> 2; v |= v >> 4;
    v |= v >> 8; v |= v >> 16;
    return v + 1;
}

// Count set bits (popcount)
int count_bits(uint32_t v) {
    int count = 0;
    while (v) { count += v & 1; v >>= 1; }
    return count;
}

// Swap bytes (manual bswap)
uint32_t bswap32(uint32_t v) {
    return ((v & 0xFF000000) >> 24) |
           ((v & 0x00FF0000) >>  8) |
           ((v & 0x0000FF00) <<  8) |
           ((v & 0x000000FF) << 24);
}

// Rotate left (32-bit)
uint32_t rotl32(uint32_t v, int n) {
    return (v << n) | (v >> (32 - n));
}
```

---

## 4.6 Signed vs Unsigned in Bit Operations

**Always use unsigned types for bit manipulation.**

```c
// WRONG — undefined behavior: shifting into sign bit
int x = 1;
x << 31;   // UB

// CORRECT
uint32_t x = 1U;
x << 31;   // fine — result is 0x80000000
```

**Rule:** Use `1U` not `1` when shifting. Use `uint8_t`, `uint16_t`, `uint32_t` for register types.

---

# PART 5 — ENDIANNESS

## 5.1 What Endianness Is

Endianness describes the byte order in which multi-byte values are stored in memory.

**Little-Endian:** LSB (Least Significant Byte) at the lowest address.
**Big-Endian:** MSB (Most Significant Byte) at the lowest address.

```
Value: 0x12345678

Address:     0x00  0x01  0x02  0x03
Little-End:  0x78  0x56  0x34  0x12   ← ARM Cortex-M default, x86
Big-End:     0x12  0x34  0x56  0x78   ← Network byte order, some protocols
```

---

## 5.2 Detecting Endianness at Runtime

```c
bool is_little_endian(void) {
    uint32_t val = 1;
    return *(uint8_t *)&val == 1;   // LSB is at lowest address
}
```

**At compile time (GCC):**

```c
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // little endian path
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    // big endian path
#endif
```

---

## 5.3 Where Endianness Matters in Firmware

**Protocol parsing** — Serial protocols (UART, SPI, I2C) send bytes one at a time. How you reassemble them depends on both the protocol's byte order and your CPU's byte order.

```c
// Received bytes: [0x12, 0x34] over UART (big-endian protocol, e.g., Modbus)
uint8_t buf[2] = {0x12, 0x34};

// WRONG on little-endian ARM — just casting
uint16_t val = *(uint16_t *)buf;   // gives 0x3412 on ARM — wrong!

// CORRECT — manual reconstruction respects protocol byte order
uint16_t val = ((uint16_t)buf[0] << 8) | buf[1];   // 0x1234 — correct
```

---

## 5.4 Byte Swapping

```c
#include <stdint.h>

// Manual byte swap
uint16_t bswap16(uint16_t v) {
    return (v >> 8) | (v << 8);
}

uint32_t bswap32(uint32_t v) {
    return ((v & 0xFF000000) >> 24) |
           ((v & 0x00FF0000) >>  8) |
           ((v & 0x0000FF00) <<  8) |
           ((v & 0x000000FF) << 24);
}

// GCC built-ins (compile to single instruction on ARM)
uint16_t x = __builtin_bswap16(val);
uint32_t y = __builtin_bswap32(val);
```

---

## 5.5 Network Byte Order Macros

Standard POSIX defines these:

```c
#include <arpa/inet.h>

htons(x)   // host to network (16-bit)
htonl(x)   // host to network (32-bit)
ntohs(x)   // network to host (16-bit)
ntohl(x)   // network to host (32-bit)
```

On bare-metal embedded without POSIX, define these yourself.

---

## 5.6 Endianness and Structs

**NEVER rely on struct layout to define a protocol frame.** Padding, alignment, and endianness all conspire against you.

```c
// WRONG — non-portable, alignment and endian dependent
typedef struct {
    uint16_t length;
    uint32_t id;
    uint8_t  payload[10];
} Frame;
Frame *f = (Frame *)rx_buffer;   // UB risk, endian risk

// CORRECT — explicit deserialization
uint16_t length = ((uint16_t)rx_buffer[0] << 8) | rx_buffer[1];
uint32_t id     = ((uint32_t)rx_buffer[2] << 24) |
                  ((uint32_t)rx_buffer[3] << 16) |
                  ((uint32_t)rx_buffer[4] <<  8) |
                              rx_buffer[5];
```

---

# PART 6 — INTEGER TYPES

## 6.1 Why `int` Is Dangerous in Firmware

The size of `int` is **platform-dependent**:
- 16-bit MCU: `int` = 2 bytes
- 32-bit MCU: `int` = 4 bytes
- 64-bit host: `int` = 4 bytes (usually)

Code that assumes `int` is 32 bits will silently break on 16-bit targets.

---

## 6.2 Fixed-Width Types — Always Use These

```c
#include <stdint.h>

int8_t    // signed  8-bit
uint8_t   // unsigned 8-bit
int16_t   // signed 16-bit
uint16_t  // unsigned 16-bit
int32_t   // signed 32-bit
uint32_t  // unsigned 32-bit
int64_t   // signed 64-bit
uint64_t  // unsigned 64-bit
```

---

## 6.3 Special Types — When to Use Each

```c
#include <stddef.h>
#include <stdint.h>

size_t      // size of objects — result of sizeof, malloc argument
            // unsigned, platform-width (32-bit on 32-bit systems)

ptrdiff_t   // difference between two pointers (signed)

uintptr_t   // large enough to hold a pointer as an integer
intptr_t    // signed version

// Use uintptr_t when converting between pointers and integers
uintptr_t addr = (uintptr_t)some_pointer;
```

---

## 6.4 Integer Promotion and Implicit Conversion — The Hidden Bugs

C automatically promotes small integer types to `int` in expressions. This causes subtle bugs.

```c
uint8_t a = 200;
uint8_t b = 100;
uint8_t result = a + b;   // a and b promoted to int, sum = 300
                           // then truncated to uint8_t → result = 44 (overflow, no warning)

// Safe explicit approach
uint8_t result = (uint8_t)(a + b);   // explicit truncation — intent is clear
```

---

## 6.5 Signed vs Unsigned Comparison — Silent Bug

```c
int len = -1;
size_t buf_size = 100;

if (len < buf_size) {        // WRONG — len is converted to huge unsigned number
    // never executes!
}

// Correct
if (len < 0 || (size_t)len < buf_size) { }
```

**Rule:** Never compare signed and unsigned integers directly. Enable `-Wsign-compare` in your compiler flags.

---

## 6.6 Overflow Behavior

| Type | Overflow Behavior |
|---|---|
| Unsigned | Wraps around (defined by C standard) |
| Signed | **Undefined behavior** — compiler may assume it never happens |

```c
uint8_t x = 255;
x++;         // x = 0 (defined wraparound)

int8_t y = 127;
y++;         // UNDEFINED BEHAVIOR — do not do this
```

---

## 6.7 Compiler Flags to Enable

```makefile
CFLAGS += -Wall -Wextra
CFLAGS += -Wsign-compare
CFLAGS += -Wconversion
CFLAGS += -Woverflow
CFLAGS += -funsigned-char       # make char unsigned by default
```

---

# PART 7 — MEMORY-MAPPED I/O

## 7.1 The Core Concept

Microcontrollers don't have separate I/O instructions (unlike x86 `in`/`out`). Instead, peripheral registers live at specific addresses in the normal memory map.

**Accessing a peripheral = reading/writing to a specific address via a pointer.**

```
Memory Map (STM32F4 example, simplified):
0x00000000 – 0x1FFFFFFF  → Flash / ITCM
0x20000000 – 0x3FFFFFFF  → SRAM
0x40000000 – 0x5FFFFFFF  → Peripheral registers ← you write here
0xE0000000 – 0xFFFFFFFF  → Cortex-M internal (SysTick, NVIC, SCB)
```

---

## 7.2 Accessing a Register Directly

```c
// Blink an LED on PA5 (STM32 style, raw register access)

// Step 1: Enable GPIOA clock
*((volatile uint32_t *)0x40023830) |= (1U << 0);   // RCC_AHB1ENR

// Step 2: Set PA5 as output
*((volatile uint32_t *)0x40020000) &= ~(0x3U << 10);  // GPIOA_MODER clear
*((volatile uint32_t *)0x40020000) |=  (0x1U << 10);  // GPIOA_MODER set output

// Step 3: Toggle PA5
*((volatile uint32_t *)0x40020014) ^= (1U << 5);   // GPIOA_ODR
```

---

## 7.3 The Proper Way — Peripheral Structs

HAL libraries define peripheral register blocks as structs. This is the standard approach.

```c
// Simplified GPIO struct (as defined in CMSIS/ST headers)
typedef struct {
    volatile uint32_t MODER;    // offset 0x00
    volatile uint32_t OTYPER;   // offset 0x04
    volatile uint32_t OSPEEDR;  // offset 0x08
    volatile uint32_t PUPDR;    // offset 0x0C
    volatile uint32_t IDR;      // offset 0x10
    volatile uint32_t ODR;      // offset 0x14
    volatile uint32_t BSRR;     // offset 0x18
    volatile uint32_t LCKR;     // offset 0x1C
    volatile uint32_t AFR[2];   // offset 0x20–0x24
} GPIO_TypeDef;

#define GPIOA  ((GPIO_TypeDef *)0x40020000)
#define GPIOB  ((GPIO_TypeDef *)0x40020400)

// Usage — clean and readable
GPIOA->MODER |= (1U << 10);
GPIOA->ODR   ^= (1U << 5);
```

**Key:** Every member is `volatile uint32_t`. The struct layout must exactly match the hardware's register layout.

---

## 7.4 Why Every Register Must Be `volatile`

Without `volatile`:
```c
// Polling loop — compiler may optimize away repeated reads
while (!(UART->SR & (1 << 7))) { }   // may become infinite loop or disappear
```

With `volatile`:
```c
// Every iteration generates an actual memory load instruction
volatile uint32_t *sr = &UART->SR;
while (!(*sr & (1 << 7))) { }   // correct
```

---

## 7.5 Read-Modify-Write Hazard

Be aware that `|=`, `&=`, `^=` are **not atomic** on most MCUs. Between the read and the write, an ISR could fire and corrupt the register.

```c
// NOT ATOMIC — risky if ISR also touches GPIOA->ODR
GPIOA->ODR |= (1U << 5);    // read → modify → write

// ATOMIC for GPIO — use BSRR (Bit Set/Reset Register) on STM32
GPIOA->BSRR = (1U << 5);           // set PA5 atomically
GPIOA->BSRR = (1U << (5 + 16));    // clear PA5 atomically
```

> Many peripherals provide set/clear registers for exactly this reason. Always prefer atomic operations when available.

---

# PART 8 — LINKER SCRIPTS & STARTUP CODE

## 8.1 What a Linker Script Does

The linker script tells the linker:
- Where in memory to place each section (`.text`, `.data`, `.bss`, etc.)
- The start address of RAM, Flash, and their sizes
- Where the stack starts

Without a linker script, code won't boot correctly on bare metal.

---

## 8.2 Minimal Linker Script (ARM Cortex-M)

```ld
/* Memory regions */
MEMORY {
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Entry point */
ENTRY(Reset_Handler)

SECTIONS {
    /* Code goes to Flash */
    .text : {
        KEEP(*(.isr_vector))    /* Vector table MUST be first */
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        _etext = .;             /* Symbol: end of text */
    } > FLASH

    /* .data initial values stored in Flash, loaded to RAM at startup */
    .data : {
        _sdata = .;             /* Symbol: start of data in RAM */
        *(.data)
        *(.data.*)
        _edata = .;             /* Symbol: end of data in RAM */
    } > RAM AT > FLASH          /* VMA = RAM, LMA = Flash */

    _sidata = LOADADDR(.data);  /* Symbol: load address of .data in Flash */

    /* .bss gets zeroed at startup — not stored in Flash */
    .bss : {
        _sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        _ebss = .;
    } > RAM

    /* Stack at top of RAM */
    _estack = ORIGIN(RAM) + LENGTH(RAM);
}
```

---

## 8.3 Startup Code — What Happens Before `main()`

```c
// startup.c — simplified ARM Cortex-M startup

extern uint32_t _sidata;   // start of .data init values in Flash
extern uint32_t _sdata;    // start of .data in RAM
extern uint32_t _edata;    // end of .data in RAM
extern uint32_t _sbss;     // start of .bss
extern uint32_t _ebss;     // end of .bss
extern uint32_t _estack;   // top of stack

void Reset_Handler(void) {
    // Step 1: Copy .data from Flash to RAM
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    // Step 2: Zero out .bss
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    // Step 3: Call system init (clocks, FPU enable, etc.)
    SystemInit();

    // Step 4: Call main
    main();

    // Should never reach here
    while (1) { }
}
```

---

## 8.4 `__attribute__` for Placing Code/Data

GCC attributes allow you to control where specific functions or variables are placed.

```c
// Place function in specific section (e.g., ITCM RAM for speed)
__attribute__((section(".itcm_code")))
void fast_isr(void) { ... }

// Place variable in specific section
__attribute__((section(".ccmram")))
static uint8_t dma_buf[1024];

// Prevent function from being optimized away or renamed
__attribute__((used, noinline))
void debug_hook(void) { ... }

// Align variable to specific boundary
__attribute__((aligned(32)))
uint8_t cache_aligned_buf[64];

// Packed struct — no padding (use carefully)
typedef struct __attribute__((packed)) {
    uint8_t  cmd;
    uint32_t addr;
    uint16_t len;
} Frame;   // sizeof = 7, not 8
```

---

## 8.5 `#pragma pack`

```c
// Equivalent to __attribute__((packed)) — more portable
#pragma pack(push, 1)
typedef struct {
    uint8_t  cmd;
    uint32_t addr;
    uint16_t len;
} Frame;
#pragma pack(pop)
```

**When to use packed:**
- Protocol frames sent over serial/network (exact byte layout needed)
- Reading binary file headers

**When NOT to use packed:**
- Normal data structures (unaligned access faults on ARM Cortex-M0/M0+)
- Performance-critical code (unaligned reads are slow even where supported)

---

# PART 9 — COMMON FIRMWARE MEMORY BUGS

## 9.1 Returning Pointer to Local Variable

```c
// CATASTROPHICALLY WRONG
int *get_value(void) {
    int local = 42;
    return &local;    // local is destroyed when function returns
}

// Caller reads garbage or crashes
int *p = get_value();
printf("%d\n", *p);   // undefined behavior
```

**Fix options:**
```c
// Option 1: Static local
int *get_value(void) {
    static int val = 42;
    return &val;   // safe — static lifetime
}

// Option 2: Caller provides buffer
void get_value(int *out) {
    *out = 42;
}

// Option 3: Return by value (when small)
int get_value(void) {
    return 42;
}
```

---

## 9.2 Stack Overflow (Bare Metal)

```c
// DANGER — 4096 bytes on stack, MCU stack may only be 2048 bytes total
void process(void) {
    uint8_t buf[4096];   // silent stack overflow
    // ...
}
```

**Detection:**
```c
// Stack watermarking — fill stack with pattern at startup
void fill_stack_pattern(void) {
    extern uint32_t _stack_end;
    uint32_t *p = &_stack_end;
    while (p < (uint32_t *)__get_MSP()) {
        *p++ = 0xDEADBEEF;
    }
}

// Later — check how much was consumed
uint32_t check_stack_usage(void) {
    extern uint32_t _stack_end;
    uint32_t *p = &_stack_end;
    while (*p == 0xDEADBEEF) p++;
    return (uint32_t)((uint8_t *)__get_MSP() - (uint8_t *)p);  // bytes used
}
```

---

## 9.3 Uninitialized Variables

```c
// On bare metal, uninitialized locals contain WHATEVER was on the stack before
void foo(void) {
    int x;            // indeterminate value — NOT zero
    if (x > 0) { }   // undefined behavior
}

// Always initialize
int x = 0;
```

**Note:** Global and static variables ARE zero-initialized (by startup code clearing `.bss`). Local variables are NOT.

---

## 9.4 Buffer Overflow

```c
uint8_t buf[8];
uint8_t len = receive_data(buf, sizeof(buf));  // safe with length limit

// WRONG
void parse(uint8_t *data, uint16_t len) {
    uint8_t local[8];
    memcpy(local, data, len);   // no bounds check — if len > 8, stack corrupted
}

// CORRECT
void parse(uint8_t *data, uint16_t len) {
    uint8_t local[8];
    if (len > sizeof(local)) return;  // guard
    memcpy(local, data, len);
}
```

---

## 9.5 ISR-Stack Variable Lifetime

```c
// WRONG — pointing to stack var from ISR context
void some_function(void) {
    uint8_t local_buf[32];
    register_dma_buffer(local_buf);   // DMA keeps pointer after function returns
}                                      // local_buf GONE — DMA writing to dead stack

// CORRECT — use static or global for ISR/DMA buffers
static uint8_t dma_buf[32];

void some_function(void) {
    register_dma_buffer(dma_buf);    // safe — static lifetime
}
```

---

# PART 10 — ALIGNMENT AND PADDING (DEEP DIVE)

## 10.1 What Alignment Is

A variable of type T is **naturally aligned** if its address is a multiple of `sizeof(T)`.

```
uint8_t  → any address (align 1)
uint16_t → even address (align 2): 0x2000, 0x2002, 0x2004
uint32_t → address % 4 == 0:       0x2000, 0x2004, 0x2008
uint64_t → address % 8 == 0:       0x2000, 0x2008, 0x2010
```

---

## 10.2 Consequences of Misalignment

| Architecture | Unaligned Access |
|---|---|
| ARM Cortex-M0/M0+ | **HardFault** — no unaligned support |
| ARM Cortex-M3/M4/M7 | Supported (slow, transparent) |
| x86/x64 | Supported (minor penalty) |
| MIPS, older ARM | **Crash** or incorrect data |

---

## 10.3 `alignof` and `_Alignas`

```c
#include <stdalign.h>

// Query alignment requirement
size_t a = alignof(uint32_t);   // 4
size_t b = alignof(double);     // 8

// Enforce alignment
_Alignas(32) uint8_t dma_buffer[128];   // 32-byte aligned (for cache lines)
_Alignas(4)  uint8_t force_aligned;     // 4-byte aligned uint8_t
```

---

## 10.4 Struct Padding Recap

```c
struct Bad {
    char   a;   // 1 byte @ 0
                // 3 bytes padding @ 1–3
    int    b;   // 4 bytes @ 4
    char   c;   // 1 byte @ 8
                // 3 bytes padding @ 9–11 (tail padding)
};              // sizeof = 12

struct Good {
    int    b;   // 4 bytes @ 0
    char   a;   // 1 byte @ 4
    char   c;   // 1 byte @ 5
                // 2 bytes padding @ 6–7 (tail padding to align to 4)
};              // sizeof = 8
```

---

# PART 11 — CRITICAL RULES SUMMARY

## The Non-Negotiable Laws

```
SEGMENT LAW:
    Never return pointer to stack. Static for ISR/DMA buffers.

VOLATILE LAW:
    All hardware registers must be volatile.
    All ISR-shared variables must be volatile.
    volatile ≠ atomic. volatile ≠ memory barrier.

STATIC LAW:
    Use static for module-private symbols (internal linkage).
    Use static local for persistent state across calls.
    Prefix static globals with s_ for visibility.

BIT MANIPULATION LAW:
    Always use unsigned types (uint32_t, 1U).
    Never use bit fields for hardware register mapping.
    Never assume bit field layout is portable.

ENDIAN LAW:
    Never cast raw byte buffers to multi-byte struct types.
    Always deserialize byte-by-byte for protocol data.
    Use __builtin_bswap for performance-critical swaps.

TYPE LAW:
    Never use plain int/char for hardware or protocol code.
    Use uint8_t / uint32_t / uintptr_t explicitly.
    Never mix signed and unsigned in comparisons.

ALIGNMENT LAW:
    Never assume struct layout matches protocol layout.
    Never cast unaligned pointer to wider type.
    Use __attribute__((packed)) only when necessary, never for performance code.
```

---

# PART 12 — INTERVIEW QUICK-FIRE REFERENCE

| Question | Answer |
|---|---|
| Where do initialized globals live? | `.data` (init values in Flash, variable in RAM) |
| Where do zero globals live? | `.bss` (RAM only — not stored in Flash) |
| Where does `const int x = 5` live? | `.rodata` in Flash |
| What does `volatile` prevent? | Compiler caching/reordering of memory accesses |
| Does `volatile` give atomicity? | No |
| Does `volatile` give memory barriers? | No |
| Three meanings of `static`? | Storage duration, internal linkage, array parameter annotation |
| `p->x` equivalent? | `(*p).x` |
| What is `offsetof`? | Byte offset of struct member from struct start, compile-time |
| What is `container_of`? | Recover enclosing struct pointer from member pointer |
| Little-endian stores 0x1234 as? | `0x34` at low addr, `0x12` at high addr |
| `uint32_t` set bit 5? | `reg \|= (1U << 5)` |
| `uint32_t` clear bit 5? | `reg &= ~(1U << 5)` |
| `uint32_t` toggle bit 5? | `reg ^= (1U << 5)` |
| `uint32_t` test bit 5? | `reg & (1U << 5)` |
| Signed integer overflow? | Undefined behavior |
| Unsigned integer overflow? | Wraps around (defined) |
| Unaligned access on Cortex-M0? | HardFault |
| What does startup code do before main? | Copy `.data` Flash→RAM, zero `.bss`, call `SystemInit` |
| Stack grows in which direction? | Downward (high to low address) on ARM/x86 |
| What is stack overflow symptom on bare metal? | Silent memory corruption, eventually HardFault |

---

*End of File*