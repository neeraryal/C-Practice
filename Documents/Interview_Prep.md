# Memory Management: The Complete Guide

### For Embedded Systems, RTOS and Operating System Interviews

> **How to read this guide.** Every chapter starts from first principles and builds up. Plain explanations come first, then the details, then code. Boxes marked **Interview tip** highlight what interviewers usually look for. Code marked **Write it yourself** is the kind you may be asked to write on a whiteboard. Practise it until you can write it from memory.
>
> The examples use C (plus C++ where interviewers usually expect it). Hardware examples use ARM Cortex-M (STM32), ESP32 (Xtensa and RISC-V) and RISC-V in general. Complete programs with a `main()` compile and run on a normal Linux PC with `gcc -std=c11 -Wall -Wextra file.c`.

---

## Table of Contents

1. [Memory Fundamentals](#1-memory-fundamentals)
2. [Memory Layout of a C Program](#2-memory-layout-of-a-c-program)
3. [The Stack](#3-the-stack)
4. [The Heap and Dynamic Memory Allocation](#4-the-heap-and-dynamic-memory-allocation)
5. [Writing Your Own Allocators](#5-writing-your-own-allocators)
6. [Memory Functions and Pointer Problems](#6-memory-functions-and-pointer-problems)
7. [Alignment Padding and Packing](#7-alignment-padding-and-packing)
8. [Memory Mapped IO and the volatile Keyword](#8-memory-mapped-io-and-the-volatile-keyword)
9. [Contiguous Allocation and Paging](#9-contiguous-allocation-and-paging)
10. [Virtual Memory in Depth](#10-virtual-memory-in-depth)
11. [Page Replacement Algorithms](#11-page-replacement-algorithms)
12. [Cache Memory](#12-cache-memory)
13. [Cache Replacement Coding Problems](#13-cache-replacement-coding-problems)
14. [MMU and MPU](#14-mmu-and-mpu)
15. [Memory Management in FreeRTOS](#15-memory-management-in-freertos)
16. [ESP32 Memory Architecture](#16-esp32-memory-architecture)
17. [Memory Barriers and Ordering](#17-memory-barriers-and-ordering)
18. [Flash and Non Volatile Memory](#18-flash-and-non-volatile-memory)
19. [Memory Bugs and Debugging Tools](#19-memory-bugs-and-debugging-tools)
20. [Linux Memory Management for Embedded Linux](#20-linux-memory-management-for-embedded-linux)
21. [Formula Sheet and Numericals](#21-formula-sheet-and-numericals)
22. [Interview Questions and Answers](#22-interview-questions-and-answers)
23. [Last Minute Revision Sheet](#23-last-minute-revision-sheet)

---

## 1. Memory Fundamentals

### 1.1 What is memory management?

A program needs memory for two things: its **instructions** (code) and its **data** (variables, buffers, objects). Memory is limited, shared and slow compared to the CPU, so someone has to decide:

- **Where** each piece of code and data lives.
- **When** memory is given out (allocated) and taken back (freed).
- **Who** may access which memory (protection).
- **How** to make slow memory look fast (caching).
- **How** to make a small physical memory look big (virtual memory).

That "someone" is a combination of the **compiler and linker** (decide addresses at build time), the **C runtime** (`malloc`/`free`), the **operating system or RTOS** (allocates memory to processes and tasks) and the **hardware** (MMU, MPU, caches).

> **Analogy.** Think of memory as a large hotel. Memory management is the front desk. It assigns rooms (allocation), takes keys back at checkout (freeing), stops guests entering other guests' rooms (protection), and keeps popular items in the lobby so nobody has to walk to the storeroom (caching).

The goals of a good memory manager are:

| Goal | Meaning |
|---|---|
| **Correctness** | Two users never get the same memory at the same time. |
| **Protection** | A bug in one task or process cannot corrupt another one or the kernel. |
| **Efficiency** | Little memory is wasted (low fragmentation) and allocation is fast. |
| **Determinism** (embedded) | Allocation takes a predictable, bounded time. |
| **Transparency** | Programs do not have to care where they physically live. |

### 1.2 The memory hierarchy

No single memory is fast, large and cheap at once. Computers therefore stack several kinds of memory. Small and fast memory sits close to the CPU, and large and slow memory sits far away.

```
          Faster, smaller, more expensive per byte
                          ▲
   ┌──────────────────────┴──────────────────────┐
   │ CPU registers           ~0.3 ns    bytes     │
   │ L1 cache (SRAM)         ~1 ns      32-64 KB  │
   │ L2 cache (SRAM)         ~4 ns      256 KB-2MB│
   │ L3 cache (SRAM)         ~10-20 ns  4-64 MB   │
   │ Main memory (DRAM)      ~60-100 ns GBs       │
   │ SSD / Flash             ~25-100 µs GBs-TBs   │
   │ Hard disk               ~5-10 ms   TBs       │
   └──────────────────────┬──────────────────────┘
                          ▼
          Slower, bigger, cheaper per byte
```

On a microcontroller the hierarchy is shorter. It is usually **registers → (optional cache) → on-chip SRAM → on-chip or external Flash → (optional external PSRAM/SDRAM)**.

The hierarchy works because of the **principle of locality**:

- **Temporal locality.** If you used something recently, you will probably use it again soon (loop counters, a function called in a loop).
- **Spatial locality.** If you used an address, you will probably use the neighbouring addresses soon (walking through an array, executing sequential instructions).

Caches and virtual memory both rely on locality. Without locality, neither would help.

### 1.3 Types of memory

| Memory | Volatile? | Speed | Typical use | Notes |
|---|---|---|---|---|
| **SRAM** (Static RAM) | Yes | Very fast | Caches, MCU on-chip RAM | 6 transistors per bit. No refresh needed. Expensive and low density. |
| **DRAM** (Dynamic RAM) | Yes | Fast | PC main memory, SDRAM on MPUs | 1 transistor + 1 capacitor per bit. Must be **refreshed** every few ms because the capacitor leaks. Dense and cheap. |
| **PSRAM** (Pseudo-static RAM) | Yes | Medium | External RAM for ESP32 | DRAM inside with a built-in refresh controller, so it behaves like SRAM to the user. |
| **ROM / Mask ROM** | No | Fast read | Boot ROM (ESP32 first-stage bootloader) | Written at the factory and cannot be changed. |
| **NOR Flash** | No | Fast random read, slow write | MCU program memory, SPI flash | Supports **XIP** (execute in place). Erased in sectors. |
| **NAND Flash** | No | Fast sequential, page access | SD cards, eMMC, SSD | Denser and cheaper. No XIP. Needs ECC and bad-block management. |
| **EEPROM** | No | Slow write | Small settings and calibration data | Byte-erasable with high endurance (about 1M cycles). |
| **FRAM / MRAM** | No | Fast write | Logging, frequent writes | Near-unlimited endurance. Expensive. |

**Volatile** here means "loses its contents when power is removed". It has nothing to do with the C keyword `volatile`, which is covered in Chapter 8.

### 1.4 Physical and logical addresses

- A **physical address** is the actual address sent on the memory bus to the RAM chip.
- A **logical address** (also called a **virtual address**) is the address the program uses, the value inside a pointer.

On a simple microcontroller **without an MMU**, logical address = physical address. The pointer value `0x20001000` is exactly the SRAM cell at `0x20001000`.

On a system **with an MMU** (Linux, Windows, Android), each process has its own virtual address space. The MMU translates every virtual address to a physical address on every access. Two processes can both use address `0x400000`, and the two addresses map to different physical RAM.

```
 Process A: ptr = 0x00400000 ──► MMU (A's page table) ──► physical 0x1F3A2000
 Process B: ptr = 0x00400000 ──► MMU (B's page table) ──► physical 0x0872C000
```

### 1.5 Address binding

**Address binding** is the step that turns a symbolic name in code (a variable `count`) into an actual address. It can happen at three different times:

| When | How | Example |
|---|---|---|
| **Compile time** | The compiler knows the final address and generates **absolute code**. The program must always load at the same place. | Bare-metal firmware linked to run at `0x08000000`. |
| **Load time** | The compiler generates **relocatable code**. The loader fixes up addresses when the program is loaded. | Old DOS `.EXE` files, some bootloaders that relocate an image. |
| **Execution (run) time** | Addresses are translated on every access by hardware (MMU or base register). The process can move in memory while it runs. | Every modern OS with virtual memory. |

Related terms:

- **Relocatable code.** Code that can be placed at different addresses after being fixed up.
- **Position-independent code (PIC/PIE).** Code that runs correctly at any address **without** fix-ups, because it uses PC-relative addressing. Shared libraries (`.so`) use it, and Linux uses PIE executables so that ASLR works.
- **Static linking.** Library code is copied into the executable at build time.
- **Dynamic linking.** The library is loaded at run time and shared between processes, which saves RAM.

### 1.6 Von Neumann and Harvard architectures

| | Von Neumann | Harvard | Modified Harvard (most MCUs) |
|---|---|---|---|
| Buses | One bus for code and data | Separate buses for code and data | Separate buses, but one unified address space |
| Fetch and data access at the same time? | No: the "von Neumann bottleneck" | Yes | Yes |
| Examples | Classic x86 at the memory level | AVR (separate program memory: `PROGMEM`) | ARM Cortex-M (I-Code, D-Code and System buses), ESP32 (IRAM and DRAM buses) |

**Why it matters in embedded.** On an AVR, `const char msg[] = "hi";` is copied to RAM unless you use `PROGMEM`, because flash lives in a different address space. On the ESP32, the same physical SRAM can appear at two different addresses: one through the **instruction bus** (IRAM) and one through the **data bus** (DRAM). This is why ESP-IDF has `IRAM_ATTR` and `DRAM_ATTR` (Chapter 16).

### 1.7 Endianness

**Endianness** is the order in which the bytes of a multi-byte value are stored in memory.

- **Little-endian.** The least significant byte goes at the lowest address. Used by x86, ARM (by default), RISC-V and ESP32.
- **Big-endian.** The most significant byte goes at the lowest address. Used by network byte order (TCP/IP), older PowerPC and some DSPs.

Storing `uint32_t x = 0x12345678;` at address `0x1000`:

```
Address:          0x1000  0x1001  0x1002  0x1003
Little-endian:      78      56      34      12
Big-endian:         12      34      56      78
```

**Write it yourself: detect endianness at run time**

```c
#include <stdint.h>
#include <stdio.h>

/* Method 1: look at the first byte of a known integer. */
static int is_little_endian(void)
{
    uint16_t x = 0x0001;
    return *(const uint8_t *)&x == 0x01;   /* LSB stored first => little-endian */
}

/* Method 2: a union (common in interviews). */
static int is_little_endian_union(void)
{
    union {
        uint32_t word;
        uint8_t  bytes[4];
    } u = { .word = 0x01020304u };
    return u.bytes[0] == 0x04;
}

/* Swap the byte order of a 32-bit value (0x12345678 -> 0x78563412). */
static uint32_t swap32(uint32_t x)
{
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) <<  8) |
           ((x & 0x00FF0000u) >>  8) |
           ((x & 0xFF000000u) >> 24);
}

int main(void)
{
    printf("little-endian: %d %d\n", is_little_endian(), is_little_endian_union());
    printf("swap32(0x12345678) = 0x%08X\n", (unsigned)swap32(0x12345678u));
    return 0;
}
```

> **Interview tip.** Reading the bytes through an `unsigned char *` (or `uint8_t *`) is always allowed in C, because character types may alias anything. The union method is also well defined in C (but not in C++). In real firmware, prefer `__builtin_bswap32()` or the `htonl()`/`ntohl()` family. On ARM these compile to a single `REV` instruction.

**Where endianness bites you in embedded work:**

- Parsing network packets or protocol frames (Modbus, CAN payloads, BLE).
- Reading multi-byte sensor registers over I2C/SPI (many sensors send MSB first).
- Casting a byte buffer to a struct (`(struct hdr *)buf`). This also breaks **alignment** and **strict aliasing**. Copy the bytes with `memcpy` instead, or build the value with shifts:

```c
/* Portable: build a 16-bit big-endian value from two bytes, regardless of CPU endianness. */
uint16_t be16_to_cpu(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
```

---

## 2. Memory Layout of a C Program

### 2.1 The segments

When a C program is compiled and linked, its contents are grouped into **sections** (also called segments). Each section holds one kind of data and has its own rules about where it lives and whether it can be written.

```
 High addresses
 ┌──────────────────────────┐
 │ Command-line args, env   │   (on an OS)
 ├──────────────────────────┤
 │ STACK                    │   local variables, return addresses
 │   │ grows downward       │
 │   ▼                      │
 │                          │
 │   (free space)           │
 │                          │
 │   ▲                      │
 │   │ grows upward         │
 │ HEAP                     │   malloc/calloc/realloc memory
 ├──────────────────────────┤
 │ .bss                     │   zero-initialised / uninitialised globals and statics
 ├──────────────────────────┤
 │ .data                    │   initialised (non-zero) globals and statics
 ├──────────────────────────┤
 │ .rodata                  │   constants, string literals
 ├──────────────────────────┤
 │ .text                    │   machine code
 └──────────────────────────┘
 Low addresses
```

| Section | Contains | Writable? | Stored in the program file (flash)? | Lifetime |
|---|---|---|---|---|
| `.text` | Compiled instructions | No (read-only/execute) | Yes | Whole program |
| `.rodata` | `const` globals, string literals, jump tables | No | Yes | Whole program |
| `.data` | Globals/statics with a **non-zero** initial value | Yes | **Yes**: the initial values are stored and copied to RAM at start-up | Whole program |
| `.bss` | Globals/statics with **no** initial value or initialised to 0 | Yes | **No**: only its size is recorded, and start-up code fills it with zeros | Whole program |
| Heap | `malloc`'d memory | Yes | No | From `malloc` until `free` |
| Stack | Local variables, function arguments, return addresses, saved registers | Yes | No | Until the function returns |

> **Why is `.bss` separate from `.data`?** To save space. A 10 KB zero-filled array in `.data` would take 10 KB of flash just to store zeros. In `.bss` it costs nothing in flash, because the start-up code simply writes zeros. "BSS" historically stands for **Block Started by Symbol**.

### 2.2 Where does each variable go?

```c
#include <stdlib.h>

int g_init = 10;              /* .data   : global, initialised to non-zero        */
int g_zero = 0;               /* .bss    : initialised to zero                    */
int g_uninit;                 /* .bss    : no initialiser => zero by C rules      */
const int g_const = 5;        /* .rodata : read-only global                       */
static int s_file = 7;        /* .data   : file-scope static (internal linkage)   */

void func(int param)          /* param   : stack (or a register)                 */
{
    int local = 1;            /* stack                                            */
    static int s_local = 3;   /* .data   : keeps its value between calls          */
    static int s_local0;      /* .bss                                             */
    const int c_local = 4;    /* stack   : const does NOT move a local to .rodata */
    char arr[] = "hello";     /* stack   : array of 6 chars copied from .rodata   */
    char *ptr = "world";      /* ptr on stack; "world" itself in .rodata          */
    int *heap = malloc(16);   /* heap pointer on stack; the 16 bytes on the heap  */
    register int r = 5;       /* a register if possible (only a hint)             */

    (void)param; (void)local; (void)s_local; (void)s_local0; (void)c_local;
    (void)arr; (void)ptr; (void)r;
    free(heap);
}
```

| Variable | Section | Why |
|---|---|---|
| `g_init` | `.data` | Global with a non-zero initial value |
| `g_zero`, `g_uninit` | `.bss` | Zero or no initialiser |
| `g_const` | `.rodata` | `const` global (on an MCU this stays in **flash**) |
| `s_local` | `.data` | `static` locals live for the whole program, not on the stack |
| `local`, `c_local`, `arr` | Stack | Automatic storage |
| `"world"` | `.rodata` | String literal |
| `*heap` | Heap | Came from `malloc` |

> **Interview tip: `char arr[] = "hello"` vs `char *ptr = "world"`.**
> `arr` is a **modifiable array** on the stack, initialised by copying the literal. `ptr` is a **pointer to a string literal** in read-only memory. `arr[0] = 'H';` is fine. `ptr[0] = 'W';` is **undefined behaviour**: it crashes with a segmentation fault on Linux, and on an MCU the write to flash is ignored or causes a bus fault. Always declare it as `const char *ptr = "world";` so the compiler catches the mistake.
>
> Also: `sizeof arr == 6` (5 letters + `'\0'`), but `sizeof ptr == 4` or `8` (the size of a pointer).

### 2.3 Inspecting the sections

```bash
gcc -c demo.c -o demo.o
size demo.o            # prints text / data / bss sizes
objdump -h demo.o      # lists every section with its size and address
nm -S --size-sort demo # lists symbols with sizes, biggest last
# For ARM firmware:
arm-none-eabi-size firmware.elf
```

Typical `arm-none-eabi-size` output:

```
   text    data     bss     dec     hex filename
  24576     512    8192   33280    8200 firmware.elf
```

How to read it for a microcontroller:

- **Flash used** = `text + data`. The `text` column already includes `.rodata`. `data` must also be stored in flash because its **initial values** live there.
- **RAM used (static)** = `data + bss`. The heap and stack come on top of this.

### 2.4 What happens before `main()`: start-up code

On a PC, the OS loader sets everything up. On a microcontroller, **your firmware** must prepare memory itself. This is done by the **start-up code** (often `startup_xxx.s` or `Reset_Handler`).

The steps after reset on an ARM Cortex-M:

1. The CPU reads the **initial stack pointer** from address `0x00000000` (the first entry of the vector table) and the **reset vector** (the address of `Reset_Handler`) from `0x00000004`.
2. `Reset_Handler` copies **`.data`** initial values from flash to RAM.
3. It fills **`.bss`** with zeros.
4. It calls `SystemInit()` (clock setup) and `__libc_init_array()` (C++ static constructors and functions marked `__attribute__((constructor))`).
5. It calls `main()`.

**Write it yourself: a minimal `Reset_Handler` in C**

```c
#include <stdint.h>

/* These symbols are defined by the linker script (see 2.5). Only their ADDRESSES matter. */
extern uint32_t _sidata;   /* start of .data initial values in FLASH (load address) */
extern uint32_t _sdata;    /* start of .data in RAM (run address)                   */
extern uint32_t _edata;    /* end of .data in RAM                                    */
extern uint32_t _sbss;     /* start of .bss in RAM                                   */
extern uint32_t _ebss;     /* end of .bss in RAM                                     */

extern void SystemInit(void);
extern void __libc_init_array(void);
extern int  main(void);

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    /* 1. Copy initialised data from flash to RAM. */
    while (dst < &_edata)
        *dst++ = *src++;

    /* 2. Zero the .bss section. */
    for (dst = &_sbss; dst < &_ebss; )
        *dst++ = 0u;

    SystemInit();          /* 3. Clocks, FPU enable, vector table offset ...     */
    __libc_init_array();   /* 4. Static constructors                             */
    (void)main();          /* 5. Application                                     */

    for (;;) { }           /* main() must never return on bare metal             */
}
```

> **Interview tip.** "What happens if you forget to zero `.bss`?" Every global that the C standard promises is zero starts with whatever garbage was in RAM. Typical symptoms are a counter that does not start at 0 or a `NULL` pointer check that fails at random. These bugs often appear only after a cold power-up, because a warm reset leaves the old RAM contents in place.

### 2.5 The linker script

The **linker script** (`.ld` file) tells the linker **what memory exists** and **which section goes where**. Every bare-metal project has one.

```ld
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack         = ORIGIN(RAM) + LENGTH(RAM);  /* initial SP = top of RAM            */
_Min_Heap_Size  = 0x2000;                     /* 8 KB: used only as a size check    */
_Min_Stack_Size = 0x1000;                     /* 4 KB                               */

SECTIONS
{
  .isr_vector :                     /* vector table must be first in flash        */
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))            /* KEEP: never garbage-collect this section   */
  } > FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text*)                       /* code                                        */
    *(.rodata*)                     /* constants and string literals               */
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  _sidata = LOADADDR(.data);        /* where .data's initial values sit in flash   */

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH                /* runs in RAM, stored in FLASH                */

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  .noinit (NOLOAD) :                /* survives a warm reset (see 2.7)             */
  {
    . = ALIGN(4);
    *(.noinit*)
  } > RAM

  ._user_heap_stack :               /* reserve space so the link fails if RAM runs out */
  {
    . = ALIGN(8);
    PROVIDE(end = .);               /* newlib's _sbrk() starts the heap here       */
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM
}
```

Key ideas:

- **VMA (Virtual Memory Address)** is where a section lives **while the program runs**. **LMA (Load Memory Address)** is where it is **stored** in the image. For `.data`, VMA = RAM and LMA = FLASH (that is what `> RAM AT > FLASH` means). For `.text`, both are FLASH.
- `_sdata`, `_ebss` and the other symbols are not variables. They are **addresses**. In C you declare them `extern` and only take their address (`&_sdata`).
- `KEEP()` stops `--gc-sections` from discarding something that nothing appears to reference, such as the vector table.
- The `._user_heap_stack` section reserves nothing at run time. It makes the **link fail** if static data + minimum heap + minimum stack do not fit in RAM.

### 2.6 The map file

Pass `-Wl,-Map=output.map` to the linker to get a **map file**. It lists every section, every symbol, its address and its size, and which object file it came from. Use it to answer:

- "Why did my RAM usage jump by 4 KB?" Search for the large symbol.
- "Is this function in flash or RAM?" Check its address.
- "Which library is pulling in `printf` and its 20 KB?" Look at the **cross-reference** and the archive members list.

Useful commands:

```bash
arm-none-eabi-nm --size-sort -S firmware.elf | tail -20   # 20 biggest symbols
arm-none-eabi-objdump -t firmware.elf | grep my_buffer     # where is my_buffer?
idf.py size-components                                     # ESP-IDF: usage per component
```

### 2.7 Special sections you can create

Using `__attribute__((section("name")))`, you can place a variable or function in a section of your choice:

```c
#include <stdint.h>

/* .noinit: NOT zeroed by start-up code => survives a warm (software/watchdog) reset. */
__attribute__((section(".noinit"))) static uint32_t reset_magic;
__attribute__((section(".noinit"))) static uint32_t crash_count;

void track_resets(void)
{
    if (reset_magic != 0xC0FFEE00u) {   /* cold power-on: RAM holds random data */
        reset_magic = 0xC0FFEE00u;
        crash_count = 0;
    } else {
        crash_count++;                   /* warm reset: the old value survived   */
    }
}

/* Run a function from RAM (needed while flash is busy being erased or written). */
__attribute__((section(".RamFunc"))) void flash_program_word(uint32_t addr, uint32_t data);
```

Common uses:

- Keeping crash information across a reset (`.noinit`).
- Placing DMA buffers in a specific RAM bank (`.dma_buffer`).
- Placing hot code in fast RAM (`.RamFunc`, or `IRAM_ATTR` on ESP32).
- Placing a firmware version header at a fixed flash address.

---

## 3. The Stack

### 3.1 What is the stack?

The **stack** is a region of memory used in **LIFO** (Last In, First Out) order, like a stack of plates. A special register, the **stack pointer (SP)**, always points to the top of the stack.

- Calling a function **pushes** a new **stack frame**.
- Returning from a function **pops** that frame.
- Allocation and deallocation are just "move SP down" and "move SP up", so they are **extremely fast** (one instruction) and **deterministic**.

On ARM, x86 and RISC-V, the stack **grows downward**, from high addresses towards low addresses. ARM Cortex-M uses a **full descending** stack: SP points to the last item pushed, and a push first decrements SP and then stores.

### 3.2 The stack frame

Each active function call owns one **stack frame** (also called an **activation record**). It typically contains:

```
 Higher addresses
 ┌──────────────────────────────┐
 │ Caller's frame               │
 ├──────────────────────────────┤ ◄── frame of the called function starts here
 │ Arguments that did not fit   │     (e.g. 5th+ argument on ARM)
 │ in registers                 │
 ├──────────────────────────────┤
 │ Return address (LR / ra)     │     where to continue after return
 ├──────────────────────────────┤
 │ Saved frame pointer (FP)     │     optional (-fno-omit-frame-pointer)
 ├──────────────────────────────┤
 │ Saved callee-saved registers │     e.g. r4-r11 on ARM, s0-s11 on RISC-V
 ├──────────────────────────────┤
 │ Local variables              │
 │ Temporaries / spilled values │
 ├──────────────────────────────┤ ◄── SP (top of stack)
 Lower addresses
```

The **calling convention** (the ABI) decides which registers carry arguments and which ones a function must preserve:

| | ARM (AAPCS, Cortex-M) | RISC-V (RV32/RV64) | x86-64 (System V, Linux) |
|---|---|---|---|
| Arguments | `r0`-`r3`, the rest on the stack | `a0`-`a7`, the rest on the stack | `rdi, rsi, rdx, rcx, r8, r9`, the rest on the stack |
| Return value | `r0` (`r0:r1` for 64-bit) | `a0` (`a0:a1`) | `rax` |
| Return address | `lr` (`r14`) register | `ra` (`x1`) register | Pushed on the stack by `call` |
| Callee-saved | `r4`-`r11` | `s0`-`s11` | `rbx, rbp, r12`-`r15` |
| Stack alignment | 8 bytes at public interfaces | 16 bytes | 16 bytes before a `call` |

> **Interview tip.** "Why is passing large structs by value slow?" The whole struct gets copied onto the stack (or into registers and memory) on every call. Pass a `const struct x *` instead. On a Cortex-M, passing a 4-byte value is free (`r0`), but passing a 64-byte struct copies 64 bytes.

### 3.3 A function call, step by step

```c
int add(int a, int b) { int sum = a + b; return sum; }

int main(void)
{
    int x = add(2, 3);
    return x;
}
```

What happens on a Cortex-M (simplified):

1. `main` puts `2` in `r0` and `3` in `r1`.
2. `BL add` jumps to `add` and stores the return address in `lr`.
3. If `add` calls other functions it must save `lr` first (`PUSH {r4, lr}`). A **leaf function** (one that calls nothing) does not need to save it.
4. `add` makes room for its locals (`SUB sp, sp, #8`), computes the result and puts it in `r0`.
5. It frees its locals (`ADD sp, sp, #8`) and returns (`BX lr` or `POP {r4, pc}`).
6. `main` finds the result in `r0`.

Notice that **nothing is erased** when a function returns. SP simply moves back up. The old values stay in memory until the next call overwrites them. This is why reading a dangling pointer to a local sometimes appears to "work".

### 3.4 Stacks on a Cortex-M: MSP, PSP and exceptions

Cortex-M has **two stack pointers**:

| | MSP (Main Stack Pointer) | PSP (Process Stack Pointer) |
|---|---|---|
| Used by | Reset code, `main()` before the RTOS starts, **all interrupt and exception handlers** | **RTOS tasks** (each task has its own stack area) |
| Initial value | Word 0 of the vector table | Set by the RTOS when it starts each task |

When an interrupt happens, the hardware **automatically pushes 8 registers** (32 bytes) onto the current stack: `R0, R1, R2, R3, R12, LR, PC, xPSR`. If the FPU is in use, it also pushes `S0`-`S15` and `FPSCR` (another 18 words, so 104 bytes in total, unless lazy stacking defers it).

**Why this matters for stack sizing in an RTOS:**

- Each task's stack must hold its own deepest call chain **plus** one exception frame (8 or 26 words), because an interrupt can arrive at the worst possible moment.
- Interrupt handlers run on the **MSP**, so **nested interrupts** add up on the MSP, not on the task stacks. Size the MSP for the worst-case nesting depth.

### 3.5 Stack overflow

A **stack overflow** happens when the stack grows past the memory reserved for it.

**Common causes:**

- Deep or unbounded **recursion**.
- **Large local arrays**, for example `char buf[4096];` inside a task with a 2 KB stack.
- `printf`/`sprintf` with floating-point formatting, which can use 500+ bytes of stack.
- Many nested interrupts.
- Variable-length arrays (VLAs) or `alloca()` with a size taken from untrusted input.

**What happens.** On a PC, the OS places an unmapped **guard page** below the stack, so an overflow produces an immediate **segmentation fault**. On a microcontroller **without protection, nothing stops it**. The stack silently overwrites whatever lies below it (`.bss`, the heap, another task's stack or its TCB). The program then fails much later, somewhere unrelated. This is one of the hardest classes of embedded bug.

### 3.6 Detecting stack overflow

| Technique | How it works | Cost |
|---|---|---|
| **Stack painting / high-water mark** | Fill the stack with a known pattern (`0xA5A5A5A5`) at start-up. Later, count how many words still hold the pattern. | Detects **after the fact**. Cheap. |
| **Canary** | Put a magic value at the stack's limit and check it periodically (FreeRTOS method 2 checks the last 16 bytes at every context switch). | Cheap. Can miss an overflow that jumps past the canary. |
| **MPU guard region** | Mark a small region (32 bytes or more) at the end of the stack as no-access. Any overflow triggers a MemManage fault **at the exact instruction**. | Precise. Uses an MPU region. |
| **Stack limit registers** | ARMv8-M (Cortex-M23/M33/M55) has `MSPLIM` and `PSPLIM`. Pushing below the limit raises a fault. | Precise. No MPU region needed. |
| **Static analysis** | `gcc -fstack-usage` writes `.su` files with each function's frame size. `-Wstack-usage=256` warns about big frames. Tools such as puncover add up the call graph. | Free. Cannot follow recursion or function pointers. |
| **Guard page** (OS) | Unmapped page below the stack. | Free on systems with an MMU. |

**Write it yourself: stack painting and measuring the high-water mark (bare metal)**

```c
#include <stddef.h>
#include <stdint.h>
#include "cmsis_compiler.h"        /* for __get_MSP() on Cortex-M */

#define STACK_FILL 0xA5A5A5A5u

extern uint32_t _sstack;           /* lowest address of the stack (from the linker script) */
extern uint32_t _estack;           /* highest address = initial SP                         */

/* Call as early as possible in main(). Paints all stack below the current SP. */
void stack_paint(void)
{
    uint32_t *p  = &_sstack;
    uint32_t *sp = (uint32_t *)__get_MSP();

    while (p < sp - 8)             /* leave a small margin below the live SP */
        *p++ = STACK_FILL;
}

/* Bytes at the bottom of the stack that were never touched (headroom left). */
size_t stack_headroom_bytes(void)
{
    const uint32_t *p = &_sstack;

    while (p < &_estack && *p == STACK_FILL)
        p++;
    return (size_t)((uintptr_t)p - (uintptr_t)&_sstack);
}
```

Run the system through its worst-case scenarios (all features on, maximum traffic, error paths), then read the headroom. If it is close to zero, the stack is too small. FreeRTOS does exactly this for every task with `uxTaskGetStackHighWaterMark()` (Chapter 15).

### 3.7 How big should a stack be?

1. Start with the static estimate: the deepest call chain from the `.su` files.
2. Add the exception frame (32 bytes, or 104 with FPU) for each possible interrupt nesting level, on the right stack (MSP or PSP, see 3.4).
3. Add library usage. `printf` family functions are notorious.
4. Measure the real high-water mark under worst-case load.
5. Add a **safety margin of 20-50%**, because untested paths exist.

### 3.8 Which way does the stack grow?

**Write it yourself:**

```c
#include <stdint.h>
#include <stdio.h>

/* noinline: if the compiler inlined this, both locals could share one frame. */
__attribute__((noinline))
static int stack_grows_down(uintptr_t caller_local)
{
    int callee_local;
    return (uintptr_t)&callee_local < caller_local;
}

int main(void)
{
    int main_local;
    printf("Stack grows %s\n",
           stack_grows_down((uintptr_t)&main_local) ? "DOWN" : "UP");
    return 0;
}
```

> **Interview tip.** Mention the caveats yourself. Comparing pointers to **different objects** with `<` is undefined behaviour in C. Converting them to `uintptr_t` makes the comparison legal, but the result is still implementation-specific. The compiler may also inline the function or reorder locals. That is why the check compares the frames of **two different functions** and marks the callee `noinline`. Comparing two locals inside one function tells you nothing, because the compiler may order them however it likes.

### 3.9 Never return the address of a local variable

```c
int *bad(void)
{
    int x = 42;
    return &x;            /* x is destroyed when bad() returns: dangling pointer */
}

int *also_bad(void)
{
    char buf[32];
    snprintf(buf, sizeof buf, "hi");
    return (int *)buf;     /* same problem */
}

/* Correct alternatives */
int *ok_static(void)  { static int x = 42; return &x; }   /* lives forever (not reentrant) */
int *ok_heap(void)    { int *p = malloc(sizeof *p); if (p) *p = 42; return p; } /* caller frees */
void ok_caller(int *out) { *out = 42; }                   /* caller provides the storage    */
```

GCC warns about this (`-Wreturn-local-addr`). The pointer may still read `42` immediately after the call, because nothing has overwritten that memory yet. The next function call reuses the same stack area and the value changes. This makes the bug intermittent and hard to find.

### 3.10 Recursion and the stack

Every recursive call adds a frame. **Stack used = recursion depth × frame size.**

```c
/* Depth n => n frames. factorial(100000) overflows a typical 8 MB Linux stack. */
unsigned long fact(unsigned n) { return n <= 1 ? 1 : n * fact(n - 1); }

/* Iterative version: constant stack usage. Preferred in embedded code. */
unsigned long fact_iter(unsigned n)
{
    unsigned long r = 1;
    while (n > 1)
        r *= n--;
    return r;
}
```

- **Tail recursion** happens when the recursive call is the very last action. Compilers *may* turn it into a loop (`-O2`), but the C standard does not guarantee it.
- **MISRA C (Rule 17.2)** forbids recursion in safety-critical code because its stack usage cannot be bounded statically.

### 3.11 VLAs and `alloca()`

```c
void process(size_t n)
{
    int vla[n];                   /* variable-length array: allocated on the stack */
    int *a = alloca(n * sizeof(int));   /* same idea, non-standard                  */
    /* ... */
}
```

Both allocate on the stack at run time. They are **dangerous**: if `n` is large or comes from outside the program, they overflow the stack silently, and they cannot report failure the way `malloc` returns `NULL`. They are banned by MISRA and by the Linux kernel. Use a fixed maximum size, a memory pool or the heap instead.

### 3.12 Stack vs heap

| | Stack | Heap |
|---|---|---|
| Allocation | Automatic: SP moves | Manual: `malloc`/`free`, `new`/`delete` |
| Speed | Very fast (one instruction) | Slower: free-list search, locking |
| Determinism | Fully deterministic | Usually not (depends on fragmentation) |
| Lifetime | Until the function returns | Until `free` |
| Size | Small and fixed (KBs on an MCU, 8 MB default per Linux thread) | Large (whatever is left) |
| Fragmentation | Never | Possible |
| Failure | Silent overflow (MCU) or segfault (OS) | `malloc` returns `NULL` |
| Thread safety | Each thread or task has its own stack | Shared, so it needs locking |
| Typical bugs | Overflow, returning address of a local | Leaks, double free, use-after-free |

---

## 4. The Heap and Dynamic Memory Allocation

### 4.1 Why do we need a heap?

Stack memory dies when the function returns, and global memory has a fixed size decided at compile time. Sometimes you need memory whose **size is only known at run time**, or which must **outlive the function** that created it: a packet of variable length, a linked-list node, an image buffer. The **heap** is a pool of memory from which such blocks are handed out on request and returned when no longer needed. This is called **dynamic memory allocation**.

### 4.2 The four C functions

```c
#include <stdlib.h>

void *malloc(size_t size);                 /* allocate 'size' bytes, contents garbage   */
void *calloc(size_t count, size_t size);   /* allocate count*size bytes, all zero       */
void *realloc(void *ptr, size_t new_size); /* resize a block (may move it)              */
void  free(void *ptr);                     /* give a block back                         */
```

| Function | Initialises memory? | On failure | Notes |
|---|---|---|---|
| `malloc(n)` | No: contents are **indeterminate** | Returns `NULL` | Returned memory is aligned for any built-in type (`max_align_t`, 8 or 16 bytes) |
| `calloc(n, s)` | Yes: all bits zero | Returns `NULL` | **Checks `n * s` for overflow** |
| `realloc(p, n)` | New part not initialised | Returns `NULL` and **the old block stays valid** | Can grow in place or move the data to a new block |
| `free(p)` | — | — | `free(NULL)` does nothing and is safe |

### 4.3 `malloc` vs `calloc`

| | `malloc` | `calloc` |
|---|---|---|
| Arguments | One: total bytes | Two: element count and element size |
| Contents | Garbage | Zeros |
| Overflow safety | `malloc(n * size)` can **overflow** silently and allocate a tiny block | `calloc` must return `NULL` if `n * size` overflows |
| Speed | Faster (no clearing) | Clearing costs time, **but** on Linux a large `calloc` gets fresh pages from `mmap` that are already zero, so it can skip the `memset` |

```c
/* Dangerous: if count is huge, count * sizeof(struct item) wraps around to a small value. */
struct item *a = malloc(count * sizeof(struct item));

/* Safe: calloc detects the overflow. */
struct item *b = calloc(count, sizeof(struct item));
```

> **Interview tip.** "Is `calloc` the same as `malloc` + `memset`?" Almost, with two differences: the **overflow check** and the **zero-page optimisation** described above. Also note that "all bits zero" is guaranteed to mean `0` for integers. For floating point and pointers it means `0.0` and `NULL` on every mainstream platform, but the C standard does not promise it.

### 4.4 Using `realloc` correctly

```c
/* WRONG: if realloc fails it returns NULL, and the only pointer to the old block is lost (leak). */
buf = realloc(buf, new_size);

/* RIGHT: use a temporary. */
char *tmp = realloc(buf, new_size);
if (tmp == NULL) {
    /* buf is still valid and still owned by us */
    handle_error();
} else {
    buf = tmp;
}
```

What `realloc` can do:

1. **Grow or shrink in place** if there is free space right after the block. The pointer stays the same.
2. **Move**: allocate a new block, copy `min(old, new)` bytes, free the old block and return the new address. **Every other pointer into the old block is now dangling.**
3. `realloc(NULL, n)` behaves exactly like `malloc(n)`.
4. `realloc(p, 0)` is **implementation-defined** (up to C17) and **undefined** in C23. Do not rely on it: call `free(p)` explicitly.

Growing a buffer one element at a time with `realloc` is O(n²), because every move copies everything. Grow it **geometrically** (double the capacity) to get amortised O(1) appends. This is what `std::vector` does.

### 4.5 Rules for `free`

1. Free **only** pointers returned by `malloc`/`calloc`/`realloc`. Never free a pointer to the stack, a global, a string literal or the **middle** of a block (`free(p + 1)`).
2. Free each block **exactly once**. Freeing twice (**double free**) corrupts the allocator's internal lists.
3. Never use a block after freeing it (**use-after-free**).
4. `free(NULL)` is a no-op, so you do not need `if (p) free(p);`.
5. A good habit is to set the pointer to `NULL` after freeing. A later double free then becomes harmless, and a later use crashes immediately instead of corrupting memory.

```c
free(p);
p = NULL;
```

### 4.6 What does `malloc(0)` return?

It is **implementation-defined**. It returns either `NULL` or a unique pointer that must not be dereferenced but must still be passed to `free`. glibc returns a unique minimum-size chunk (16 bytes usable on 64-bit). newlib returns a small valid block. Portable code must handle both cases.

### 4.7 Always check the return value

```c
uint8_t *buf = malloc(len);
if (buf == NULL) {
    /* recover: drop the request, report the error, use a smaller buffer ... */
    return -ENOMEM;
}
```

On **Linux**, `malloc` almost never returns `NULL`, because of **overcommit**: the kernel hands out virtual memory and only assigns physical pages when you first touch them. If physical memory then runs out, the **OOM killer** terminates a process. On a **microcontroller**, the heap is a small fixed region and `malloc` **does** return `NULL` when it runs out, so the check is essential.

### 4.8 How `malloc` works inside

#### Where the heap memory comes from

- **Linux:** the allocator asks the kernel for memory with `brk()`/`sbrk()` (moves the end of the data segment, the **program break**) for small blocks, and with `mmap()` for large ones (glibc's default threshold is 128 KB and adjusts dynamically).
- **Microcontroller with newlib:** `malloc` calls a function named `_sbrk()` that **you** (or your IDE's template) provide. It hands out memory between the end of `.bss` and the stack.

**Write it yourself: `_sbrk` for bare metal (newlib)**

```c
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern uint8_t end;                /* first byte after .bss: PROVIDE(end = .) in the .ld */
extern uint8_t _estack;            /* top of RAM = initial stack pointer                 */
extern uint32_t _Min_Stack_Size;   /* linker symbol: its ADDRESS is the size             */

static uint8_t *heap_end;          /* current end of the heap (the "program break")      */

void *_sbrk(ptrdiff_t increment)
{
    const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;

    if (heap_end == NULL)
        heap_end = &end;           /* first call: the heap starts right after .bss       */

    if ((uint32_t)(heap_end + increment) > stack_limit) {
        errno = ENOMEM;            /* would collide with the reserved stack              */
        return (void *)-1;         /* malloc() then returns NULL                          */
    }

    uint8_t *prev = heap_end;
    heap_end += increment;
    return prev;                   /* start of the newly granted memory                   */
}
```

#### Block headers: how `free` knows the size

`free(p)` receives only a pointer, not a size. The allocator stores a small **header** in front of every block it hands out. The pointer you receive points **just after** the header:

```
            header (hidden)              payload (what you use)
         ┌──────────────────────┬────────────────────────────────┐
         │ size | flags (used?) │  your data ...                  │
         └──────────────────────┴────────────────────────────────┘
         ▲                      ▲
   block start            pointer returned by malloc

 free(p) does: header = (header_t *)p - 1;  then reads header->size
```

This is why:

- Each allocation costs **extra memory** for the header (8-16 bytes), so many tiny allocations waste a lot of memory.
- Writing **before** the start of your buffer (a buffer **underflow**) corrupts the header and crashes a later `malloc` or `free`, far away from the actual bug.
- Allocators often keep size **flags in the low bits** of the size field. Sizes are multiples of 8, so the 3 low bits are always zero and free for flags such as "in use" and "previous block in use".

#### Free lists, splitting and coalescing

The allocator tracks free blocks in a **free list** (a linked list threaded through the free blocks themselves, so it costs no extra memory).

- **Splitting.** If you ask for 100 bytes and the chosen free block has 1000, the allocator splits it: 100 (plus header) for you, and the rest stays free as a new, smaller block.
- **Coalescing.** When a block is freed, the allocator merges it with **adjacent free neighbours** into one larger block. Without coalescing, the heap would turn into many small, useless pieces.
- **Boundary tags.** Allocators often store a copy of the size at the **end** of each block too (a footer). When block B is freed, the allocator can then find the previous block A's header in O(1) (read the footer just before B) and check whether A is free. This makes coalescing O(1).

```
Before free(B):  [A: free 64][B: used 32][C: free 128]
After  free(B):  [A+B+C: free 64+32+128 (+ 2 headers reclaimed)]
```

### 4.9 Placement policies: which free block to use?

| Policy | Rule | Pros | Cons |
|---|---|---|---|
| **First fit** | Use the **first** free block that is big enough | Fast, simple | Small leftover fragments pile up at the start of the list |
| **Next fit** | Like first fit, but continue searching from where the last search stopped | Spreads allocations out | Usually more fragmentation than first fit |
| **Best fit** | Use the **smallest** block that is big enough | Wastes the least space per allocation | Must search the whole list (unless sorted). Leaves tiny useless slivers |
| **Worst fit** | Use the **largest** block | Leftovers are large and reusable | Quickly destroys large blocks. Generally worst in practice |

**Classic exercise (Silberschatz).** Free partitions, in order: **100, 500, 200, 300, 600 KB**. Requests: **212, 417, 112, 426 KB**.

| Request | First fit | Best fit | Worst fit |
|---|---|---|---|
| 212 | 500 → leaves 288 | 300 → leaves 88 | 600 → leaves 388 |
| 417 | 600 → leaves 183 | 500 → leaves 83 | 500 → leaves 83 |
| 112 | 288 → leaves 176 | 200 → leaves 88 | 388 → leaves 276 |
| 426 | **must wait** (largest hole 300) | 600 → leaves 174 | **must wait** (largest hole 300) |

Here **best fit** is the only policy that satisfies every request. In simulations, first fit and best fit both beat worst fit in storage use, and first fit is generally faster.

### 4.10 Fragmentation

**Fragmentation** is memory that is free but **cannot be used**.

**Internal fragmentation** is wasted space **inside** an allocated block. It happens because the allocator rounds sizes up (alignment, minimum block size, power-of-two classes, fixed-size pool blocks, the last partly used page).

```
malloc(13) with 8-byte rounding -> 16-byte block: 3 bytes wasted inside the block.
Fixed pool of 64-byte blocks used for 20-byte messages -> 44 bytes wasted per message.
```

**External fragmentation** is free memory split into many **non-contiguous holes**, none big enough for a request, even though the **total** free memory is.

```
Heap = 100 KB, allocate five 20 KB blocks:  [A][B][C][D][E]
Free B and D:                               [A][ free 20 ][C][ free 20 ][E]
Total free = 40 KB, but malloc(30 KB) FAILS: no single hole of 30 KB exists.
```

| | Internal | External |
|---|---|---|
| Where | Inside allocated blocks | Between allocated blocks |
| Caused by | Rounding up / fixed block sizes | Variable-size allocations freed in random order |
| Found in | Paging (last page), pools, buddy allocator | Variable-size heaps, segmentation, contiguous allocation |
| Fix | Size classes closer to real sizes | Coalescing, compaction, paging, pools, fixed sizes |

**Knuth's 50-percent rule.** With first fit, for every N allocated blocks about 0.5 N blocks are lost to fragmentation, so about **one third of memory may be unusable**.

**Compaction** (moving allocated blocks together to merge the holes) works in garbage-collected languages, but it is **impossible in C**, because the allocator cannot find and update every raw pointer that points into a moved block.

### 4.11 Real allocators, briefly

| Allocator | Where | Key idea |
|---|---|---|
| **ptmalloc2** | glibc (Linux) | Chunks with headers. **tcache** (per-thread cache, 7 chunks per size class), **fastbins** (small, LIFO, not coalesced immediately), **unsorted/small/large bins**, a **top chunk** that grows with `brk`, big blocks via `mmap`, multiple **arenas** for threads |
| **dlmalloc** | Classic, many embedded libcs | Segregated bins, boundary tags |
| **newlib / newlib-nano malloc** | ARM GCC toolchains | Simple free list, small code size |
| **TLSF** (Two-Level Segregated Fit) | RTOSes, **ESP-IDF (since v5.0)** | Two-level bitmap of free lists: **O(1) malloc and free**, low fragmentation, good for real time |
| **jemalloc / tcmalloc / mimalloc** | FreeBSD, Firefox, Google, servers | Per-thread caches and size classes for multi-core speed |
| **Buddy allocator** | Linux page allocator | Power-of-two blocks, fast merging (Chapter 5) |
| **Slab / SLUB** | Linux kernel objects | Caches of same-sized objects (Chapter 5) |

### 4.12 Dynamic allocation in embedded systems

Many embedded coding standards restrict or forbid `malloc` after initialisation. **MISRA C:2012 Rule 21.3** says the allocation functions of `<stdlib.h>` shall not be used. The reasons:

| Problem | Why it matters |
|---|---|
| **Non-deterministic timing** | Searching a free list takes a variable amount of time, which hurts hard real-time deadlines |
| **Fragmentation** | A device that runs for months can fail after weeks, when the heap is too fragmented for one allocation, even though plenty of total memory is free |
| **Failure handling** | When `malloc` returns `NULL` at run time, there is often nothing sensible a device can do |
| **Not thread or ISR safe** | Standard `malloc` must not be called from an ISR. In an RTOS it needs a lock |
| **Hidden memory cost** | Headers and alignment waste RAM that is already scarce |
| **Leaks** | A tiny leak in a loop eventually exhausts the heap, maybe after days |

**Safe strategies:**

1. **Static allocation.** Size everything at compile time (global buffers, FreeRTOS `xTaskCreateStatic`).
2. **Allocate only during initialisation, never free.** The heap then behaves like static memory (FreeRTOS `heap_1` is built for this).
3. **Fixed-size memory pools** (Chapter 5): O(1), deterministic and free of external fragmentation.
4. A **deterministic allocator** such as TLSF if general allocation is truly needed.
5. **Monitor** the free heap and the **minimum ever free** heap, and log the largest free block to detect fragmentation.

### 4.13 C++ `new`/`delete` vs C `malloc`/`free`

| | `malloc`/`free` | `new`/`delete` |
|---|---|---|
| Language | C (also usable in C++) | C++ |
| Constructors and destructors | **Not** called | Called |
| Return type | `void *` (needs a cast in C++) | Correctly typed pointer |
| Size | You compute it in bytes | Computed by the compiler |
| On failure | Returns `NULL` | Throws `std::bad_alloc` (or returns `nullptr` with `new (std::nothrow)`) |
| Arrays | Same function | `new[]` must be paired with `delete[]` |
| Can be overloaded | No | Yes (`operator new`), e.g. to route to an RTOS heap |

**Never mix them**: no `free()` on memory from `new`, no `delete` on memory from `malloc`, no `delete` on a `new[]` array. All three are undefined behaviour.

**Placement new** constructs an object in memory you already own, with no heap involved. Embedded C++ uses it a lot:

```cpp
#include <new>

struct Sensor { explicit Sensor(int id) : id(id) {} int id; };

alignas(Sensor) static unsigned char storage[sizeof(Sensor)];

Sensor *s = new (storage) Sensor(42);   // construct inside the static buffer
s->~Sensor();                           // destroy explicitly; never call delete on it
```

---

## 5. Writing Your Own Allocators

Interviewers love these questions. They test pointers, alignment, data structures and system thinking all at once. Each section below gives the idea, a diagram and a complete, tested program.

### 5.1 A simple `malloc`/`free` (first fit, splitting, coalescing)

**The idea.**

- A static array acts as the heap.
- Every block starts with a header: `size`, `free` flag, `next` pointer. The blocks form a linked list **in address order**.
- `my_malloc` walks the list and takes the **first** free block that is big enough (first fit). If that block is much bigger than needed, it is **split** in two.
- `my_free` marks the block free and **merges** (coalesces) neighbouring free blocks.

```
 heap[]:
 ┌────────┬──────────────┬────────┬────────────┬────────┬──────────────────┐
 │ hdr    │ payload A    │ hdr    │ payload B  │ hdr    │ free space       │
 │size=100│ (in use)     │size=200│ (free)     │size=...│                  │
 │free=0  │              │free=1  │            │free=1  │                  │
 └───┬────┴──────────────┴───┬────┴────────────┴───┬────┴──────────────────┘
     └────── next ──────────►└────── next ─────────►└──► NULL
```

**Write it yourself:**

```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HEAP_SIZE   (16u * 1024u)
#define ALIGNMENT   8u                     /* 8 suits 32-bit MCUs; use 16 on 64-bit PCs */
#define ALIGN_UP(n) (((n) + (ALIGNMENT - 1u)) & ~(size_t)(ALIGNMENT - 1u))

typedef struct block {
    size_t        size;                    /* payload bytes (header not included)    */
    int           free;                    /* 1 = free, 0 = in use                   */
    struct block *next;                    /* next block in ADDRESS order            */
} block_t;

#define HDR_SIZE  ALIGN_UP(sizeof(block_t)) /* keeps every payload aligned          */
#define MIN_SPLIT (HDR_SIZE + ALIGNMENT)    /* smallest leftover worth splitting off */

static _Alignas(ALIGNMENT) uint8_t heap[HEAP_SIZE];
static block_t *head;

static void heap_init(void)
{
    head = (block_t *)heap;                /* one big free block covering everything */
    head->size = HEAP_SIZE - HDR_SIZE;
    head->free = 1;
    head->next = NULL;
}

void *my_malloc(size_t size)
{
    if (size == 0 || size > HEAP_SIZE)
        return NULL;
    if (head == NULL)
        heap_init();

    size = ALIGN_UP(size);

    for (block_t *b = head; b != NULL; b = b->next) {
        if (!b->free || b->size < size)
            continue;                      /* first fit: skip until one fits */

        if (b->size >= size + MIN_SPLIT) { /* split: carve a new free block from the tail */
            block_t *rest = (block_t *)((uint8_t *)b + HDR_SIZE + size);
            rest->size = b->size - size - HDR_SIZE;
            rest->free = 1;
            rest->next = b->next;
            b->size = size;
            b->next = rest;
        }
        b->free = 0;
        return (uint8_t *)b + HDR_SIZE;    /* user pointer = just after the header */
    }
    return NULL;                           /* no block big enough: out of memory or fragmented */
}

void my_free(void *ptr)
{
    if (ptr == NULL)
        return;

    block_t *b = (block_t *)((uint8_t *)ptr - HDR_SIZE);
    if (b->free)
        return;                            /* cheap double-free guard */
    b->free = 1;

    /* Coalesce: merge every pair of neighbouring free blocks. */
    for (block_t *cur = head; cur != NULL && cur->next != NULL; ) {
        if (cur->free && cur->next->free) {
            cur->size += HDR_SIZE + cur->next->size;  /* absorb the neighbour and its header */
            cur->next  = cur->next->next;             /* stay on cur: it may merge again     */
        } else {
            cur = cur->next;
        }
    }
}

void *my_realloc(void *ptr, size_t new_size)
{
    if (ptr == NULL)
        return my_malloc(new_size);
    if (new_size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_t *b = (block_t *)((uint8_t *)ptr - HDR_SIZE);
    if (b->size >= new_size)
        return ptr;                        /* already big enough */

    void *p = my_malloc(new_size);
    if (p != NULL) {
        memcpy(p, ptr, b->size);           /* copy the old contents */
        my_free(ptr);
    }
    return p;                              /* on failure the old block is untouched */
}

void *my_calloc(size_t count, size_t size)
{
    if (size != 0 && count > SIZE_MAX / size)
        return NULL;                       /* count * size would overflow */
    void *p = my_malloc(count * size);
    if (p != NULL)
        memset(p, 0, count * size);
    return p;
}

static void heap_dump(const char *title)
{
    printf("%s\n", title);
    for (block_t *b = head; b != NULL; b = b->next)
        printf("  [%5zu] size=%5zu %s\n",
               (size_t)((uint8_t *)b - heap), b->size, b->free ? "FREE" : "used");
}

int main(void)
{
    char *a = my_malloc(100);
    char *b = my_malloc(200);
    char *c = my_malloc(50);
    heap_dump("after a, b, c:");

    my_free(b);                            /* leaves a hole between a and c */
    char *d = my_malloc(150);              /* first fit reuses b's hole     */
    heap_dump("after free(b), d = malloc(150):");
    printf("d reused b's hole: %s\n", d == b ? "yes" : "no");

    my_free(a);
    my_free(c);
    my_free(d);
    heap_dump("after freeing everything (fully coalesced):");

    void *big = my_malloc(HEAP_SIZE - HDR_SIZE);
    printf("whole-heap allocation after coalescing: %s\n", big ? "OK" : "FAILED");
    return 0;
}
```

**Points to discuss in the interview:**

| Topic | What to say |
|---|---|
| Complexity | `malloc` and `free` are O(n) in the number of blocks. A separate **free list** (only free blocks) speeds up `malloc`. **Boundary tags** (a footer with the size) make coalescing O(1). **Segregated lists** (one list per size class) or TLSF make both O(1). |
| Alignment | The header size and every payload size are rounded up to `ALIGNMENT`, so every returned pointer is aligned. |
| Thread safety | Wrap `my_malloc`/`my_free` in a mutex (RTOS) or a critical section (bare metal). Never call it from an ISR unless it is designed for that. |
| Robustness | Check that `ptr` lies inside `heap[]`. Add a **magic number** in the header to detect corruption and invalid frees. |
| Fragmentation | First fit with coalescing still fragments. Discuss pools for fixed-size objects. |

### 5.2 `aligned_malloc` and `aligned_free`

**Question:** "Write `aligned_malloc(size, align)` that returns memory aligned to `align` bytes (a power of two), using only `malloc`. Write the matching `aligned_free`."

Typical uses: DMA buffers that must start on a cache-line boundary (32 or 64 bytes), SIMD data (16/32 bytes), page-aligned buffers (4096).

**The idea.** Allocate extra bytes, move forward to the next aligned address, and **store the original pointer just before the aligned address** so `aligned_free` can find it.

```
raw (from malloc)                   aligned (returned to user)
│                                   │
▼                                   ▼
┌──────────── padding ─────┬────────┬─────────────────────────────┐
│ (0 .. align-1 bytes)     │ raw ptr│ user data (size bytes)      │
└──────────────────────────┴────────┴─────────────────────────────┘
                            ▲ stored at ((void **)aligned)[-1]
```

**Write it yourself:**

```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* align must be a power of two. Returns NULL on bad input or out of memory. */
void *aligned_malloc(size_t size, size_t align)
{
    if (align == 0 || (align & (align - 1)) != 0)
        return NULL;                                 /* not a power of two */
    if (align < sizeof(void *))
        align = sizeof(void *);                      /* keep the stored pointer aligned */

    size_t extra = (align - 1) + sizeof(void *);     /* worst-case padding + saved pointer */
    if (size > SIZE_MAX - extra)
        return NULL;                                 /* size + extra would overflow */

    void *raw = malloc(size + extra);
    if (raw == NULL)
        return NULL;

    uintptr_t start   = (uintptr_t)raw + sizeof(void *);            /* leave room for the pointer */
    uintptr_t aligned = (start + (align - 1)) & ~(uintptr_t)(align - 1);

    ((void **)aligned)[-1] = raw;                    /* remember what malloc gave us */
    return (void *)aligned;
}

void aligned_free(void *ptr)
{
    if (ptr != NULL)
        free(((void **)ptr)[-1]);                    /* free the ORIGINAL pointer */
}

int main(void)
{
    size_t aligns[] = { 8, 16, 32, 64, 4096 };
    for (size_t i = 0; i < sizeof aligns / sizeof aligns[0]; i++) {
        void *p = aligned_malloc(100, aligns[i]);
        printf("align %4zu -> %p  aligned: %s\n", aligns[i], p,
               ((uintptr_t)p % aligns[i]) == 0 ? "yes" : "NO");
        aligned_free(p);
    }
    return 0;
}
```

**Why `(x + align - 1) & ~(align - 1)` works.** `align` is a power of two, so `align - 1` is a mask of the low bits (e.g. `64 - 1 = 0b111111`). Adding `align - 1` pushes any unaligned address past the next boundary, and `& ~(align - 1)` clears the low bits, rounding **down** to that boundary. An already aligned address does not move.

**Variations interviewers ask:**

- *"Store the offset, not the pointer, to save space."* Store `aligned - raw` in a `uint8_t` (if `align ≤ 256`) or `uint16_t` just before the aligned address, and always allocate at least 1 (or 2) extra bytes.
- *"What standard functions exist?"* C11 `aligned_alloc(align, size)` (size should be a multiple of align), POSIX `posix_memalign(&p, align, size)`, ESP-IDF `heap_caps_aligned_alloc()`. All of them are released with plain `free()`.

### 5.3 Fixed-size block memory pool

**The most important allocator in embedded systems.** All blocks have the same size, so:

- `alloc` and `free` are **O(1)**: pop from or push to a singly linked free list.
- There is **no external fragmentation**: any free block fits any request.
- The behaviour is fully **deterministic**, which suits real-time code.
- The free list is stored **inside the free blocks themselves**, so it costs no extra memory.

```
Initial:  free_list ─► [0] ─► [1] ─► [2] ─► [3] ─► ... ─► [N-1] ─► NULL

alloc():  take [0];   free_list ─► [1] ─► [2] ─► ...
free(p):  push p;     free_list ─► [p] ─► [1] ─► [2] ─► ...
```

**Write it yourself:**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define POOL_BLOCK_SIZE   32u          /* bytes per block (must be >= sizeof(void *)) */
#define POOL_BLOCK_COUNT  8u

typedef union pool_block {
    union pool_block *next;            /* used while the block is FREE              */
    uint8_t data[POOL_BLOCK_SIZE];     /* used while the block is ALLOCATED         */
    max_align_t align;                 /* forces the strongest alignment            */
} pool_block_t;

static pool_block_t  pool[POOL_BLOCK_COUNT];
static pool_block_t *free_list;
static size_t        free_count;

void pool_init(void)
{
    for (size_t i = 0; i < POOL_BLOCK_COUNT - 1u; i++)
        pool[i].next = &pool[i + 1u];  /* chain every block to the next one */
    pool[POOL_BLOCK_COUNT - 1u].next = NULL;
    free_list  = &pool[0];
    free_count = POOL_BLOCK_COUNT;
}

void *pool_alloc(void)
{
    pool_block_t *b = free_list;
    if (b != NULL) {                   /* pop the head: O(1) */
        free_list = b->next;
        free_count--;
    }
    return b;                          /* NULL when the pool is empty */
}

static bool pool_owns(const void *p)   /* is p the start of one of our blocks? */
{
    uintptr_t base = (uintptr_t)pool;
    uintptr_t addr = (uintptr_t)p;
    return addr >= base &&
           addr <  base + sizeof pool &&
           (addr - base) % sizeof(pool_block_t) == 0;
}

void pool_free(void *p)
{
    if (p == NULL || !pool_owns(p))
        return;                        /* ignore foreign pointers */
    pool_block_t *b = p;
    b->next   = free_list;             /* push on the head: O(1) */
    free_list = b;
    free_count++;
}

int main(void)
{
    void *blocks[POOL_BLOCK_COUNT + 1];

    pool_init();
    for (size_t i = 0; i < POOL_BLOCK_COUNT + 1u; i++)
        blocks[i] = pool_alloc();      /* the last request must fail */

    printf("block 0 at %p, block 1 at %p\n", blocks[0], blocks[1]);
    printf("allocation #%u when empty: %s\n", POOL_BLOCK_COUNT + 1u,
           blocks[POOL_BLOCK_COUNT] == NULL ? "NULL (correct)" : "WRONG");

    pool_free(blocks[3]);
    printf("free blocks after one free: %zu\n", free_count);
    printf("next alloc reuses block 3: %s\n", pool_alloc() == blocks[3] ? "yes" : "no");
    return 0;
}
```

**Making it thread-safe or ISR-safe.** The pop and the push are each a few instructions. Protect them with a short critical section:

```c
/* Bare-metal Cortex-M: save PRIMASK, disable interrupts, restore. Safe to call from ISRs. */
void *pool_alloc_safe(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    void *p = pool_alloc();
    __set_PRIMASK(primask);            /* restore the previous state (nesting-safe) */
    return p;
}

/* FreeRTOS task context: taskENTER_CRITICAL() / taskEXIT_CRITICAL().
   FreeRTOS ISR context:  x = taskENTER_CRITICAL_FROM_ISR(); ... taskEXIT_CRITICAL_FROM_ISR(x); */
```

**Several pool sizes.** Real systems keep a few pools, for example 32, 128 and 512 bytes. A request goes to the **smallest pool whose block fits**. This trades a little **internal** fragmentation for zero **external** fragmentation. LwIP's `memp` pools, Zephyr's `k_mem_slab` and ThreadX's block pools all work this way.

### 5.4 The buddy allocator

The **buddy system** manages memory in blocks whose sizes are **powers of two**. The Linux kernel uses it to allocate physical pages (`alloc_pages(gfp, order)` returns 2^order pages).

**Allocation:**

1. Round the request up to the next power of two (e.g. 100 bytes → 128).
2. Find the smallest free block that is at least that big.
3. While the block is bigger than needed, **split it in half**. The two halves are **buddies**. Keep one half, put the other on the free list for its size.

**Freeing:**

1. Find the block's buddy. Its address is `offset XOR block_size`, because buddies differ in exactly one bit.
2. If the buddy is **free and the same size**, merge the two into the parent block and repeat one level up.
3. Stop when the buddy is busy or you reach the top.

**Example.** A 1024-byte heap. Request 100 bytes, which rounds up to 128.

```
1024                                     split 1024 -> 512 + 512
[            512             ][   512   ]   split 512 -> 256 + 256
[    256     ][     256      ][   512   ]   split 256 -> 128 + 128
[128][  128  ][     256      ][   512   ]
  ▲ allocated (A)
Free lists: 128: {128}, 256: {256}, 512: {512}

free(A) at offset 0, size 128: buddy = 0 ^ 128 = 128 -> free -> merge to 256 at 0
                     size 256: buddy = 0 ^ 256 = 256 -> free -> merge to 512 at 0
                     size 512: buddy = 0 ^ 512 = 512 -> free -> merge to 1024. Done.
```

| Pros | Cons |
|---|---|
| Very fast splitting and merging (XOR trick) | **Internal fragmentation**: 129 bytes uses a 256-byte block (almost 50% waste) |
| Simple to implement and reason about | Two adjacent free blocks that are **not** buddies cannot merge |
| Limits external fragmentation | All sizes are powers of two |

**Write it yourself: a compact buddy allocator**

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MIN_ORDER   5                      /* smallest block: 2^5  = 32 bytes   */
#define MAX_ORDER   12                     /* whole heap:     2^12 = 4096 bytes */
#define HEAP_BYTES  (1u << MAX_ORDER)

typedef struct fblock { struct fblock *next; } fblock_t;

static _Alignas(16) uint8_t heap[HEAP_BYTES];
static fblock_t *free_lists[MAX_ORDER + 1];   /* free_lists[k]: free blocks of 2^k bytes */
static int initialized;

static int order_for(size_t size)          /* smallest k with 2^k >= size (k >= MIN_ORDER) */
{
    int k = MIN_ORDER;
    while (((size_t)1 << k) < size)
        k++;
    return k;
}

static void push(int k, void *blk)
{
    fblock_t *b = blk;
    b->next = free_lists[k];
    free_lists[k] = b;
}

static int remove_if_free(int k, void *blk) /* remove blk from list k if it is there */
{
    for (fblock_t **pp = &free_lists[k]; *pp != NULL; pp = &(*pp)->next) {
        if (*pp == blk) {
            *pp = (*pp)->next;
            return 1;
        }
    }
    return 0;
}

void *buddy_alloc(size_t size)
{
    if (!initialized) {
        push(MAX_ORDER, heap);             /* start with one maximal free block */
        initialized = 1;
    }
    if (size == 0 || size > HEAP_BYTES)
        return NULL;

    int want = order_for(size);
    int k = want;
    while (k <= MAX_ORDER && free_lists[k] == NULL)
        k++;                               /* smallest non-empty list that fits */
    if (k > MAX_ORDER)
        return NULL;

    fblock_t *blk = free_lists[k];
    free_lists[k] = blk->next;

    while (k > want) {                     /* split until the block is the right size */
        k--;
        push(k, (uint8_t *)blk + ((size_t)1 << k));   /* upper half = free buddy */
    }
    return blk;
}

/* Like the Linux free_pages(addr, order): the caller passes the size it asked for. */
void buddy_free(void *ptr, size_t size)
{
    if (ptr == NULL)
        return;

    int k = order_for(size);
    size_t off = (size_t)((uint8_t *)ptr - heap);

    while (k < MAX_ORDER) {
        size_t buddy = off ^ ((size_t)1 << k);        /* flip the bit for this size */
        if (!remove_if_free(k, heap + buddy))
            break;                                    /* buddy in use: stop merging */
        off &= ~((size_t)1 << k);                     /* merged block starts at the lower address */
        k++;
    }
    push(k, heap + off);
}

int main(void)
{
    uint8_t *a = buddy_alloc(100);         /* -> 128-byte block */
    uint8_t *b = buddy_alloc(200);         /* -> 256-byte block */
    uint8_t *c = buddy_alloc(30);          /* ->  32-byte block */
    printf("offsets: a=%d b=%d c=%d\n", (int)(a - heap), (int)(b - heap), (int)(c - heap));

    buddy_free(a, 100);
    buddy_free(b, 200);
    buddy_free(c, 30);

    void *all = buddy_alloc(HEAP_BYTES);
    printf("whole heap available after merging: %s\n", all ? "yes" : "no");
    return 0;
}
```

### 5.5 The slab allocator

Kernels allocate the **same kinds of objects** over and over: process descriptors, inodes, network buffers, semaphores. The **slab allocator** (Jeff Bonwick, SunOS, later Linux) keeps a **cache per object type**:

- A **cache** holds **slabs**. A slab is one or more physically contiguous pages, divided into equal-sized slots for one object type.
- Each slab is **full**, **partial** or **empty**. Allocation takes a slot from a partial slab first, then from an empty one, and only asks the page allocator for a new slab when needed.
- Objects can be kept **pre-initialised**, which saves constructor work.

**Benefits:** no fragmentation between objects of different sizes, very fast allocation, good cache behaviour. In Linux, `kmalloc()` itself is built on slab caches of size classes (`kmalloc-32`, `kmalloc-64`, ...), and drivers create their own with `kmem_cache_create()`. Modern Linux uses **SLUB**, a simpler and faster version of the same idea. `cat /proc/slabinfo` shows them.

> **The connection.** A slab cache is just the **fixed-size memory pool** from 5.3, scaled up and managed by the kernel.

### 5.6 Choosing an allocator

| Need | Use |
|---|---|
| Objects of one or a few fixed sizes, real-time | **Memory pool** |
| Allocate at start-up and never free | **Bump allocator** (FreeRTOS `heap_1`) |
| General sizes, long-running, real-time | **TLSF** or FreeRTOS **`heap_4`** |
| Physical pages in a kernel | **Buddy allocator** |
| Many kernel objects of the same type | **Slab/SLUB** |
| Desktop or server applications | The system `malloc` (ptmalloc, jemalloc, ...) |

---

## 6. Memory Functions and Pointer Problems

### 6.1 `memcpy`, `memmove`, `memset`, `memcmp`, `strcpy`

| Function | Does | Overlap allowed? | Stops at `'\0'`? |
|---|---|---|---|
| `memcpy(dst, src, n)` | Copies exactly `n` bytes | **No**: undefined behaviour if the regions overlap | No |
| `memmove(dst, src, n)` | Copies exactly `n` bytes **as if** through a temporary buffer | **Yes** | No |
| `memset(dst, c, n)` | Fills `n` bytes with the byte `(unsigned char)c` | — | No |
| `memcmp(a, b, n)` | Compares `n` bytes. Returns <0, 0 or >0 | — | No |
| `strcpy(dst, src)` | Copies a string **including** the terminating `'\0'` | No | Yes |
| `strncpy(dst, src, n)` | Copies at most `n` chars. **Does not add `'\0'` if `src` is too long.** Pads with `'\0'` if shorter | No | Yes |

**Why does overlap matter?** Consider shifting an array one place to the right with a forward copy:

```
memcpy(buf + 1, buf, 4) on "ABCDE", copying forward byte by byte:
  step 1: buf[1] = buf[0] -> "AACDE"
  step 2: buf[2] = buf[1] -> "AAADE"   <- buf[1] was already overwritten!
  result: "AAAAA"  (expected "AABCD")

memmove notices dst > src and copies BACKWARD (last byte first):
  buf[4]=buf[3], buf[3]=buf[2], buf[2]=buf[1], buf[1]=buf[0] -> "AABCD"  correct
```

Rule: if `dst < src`, copy **forward**. If `dst > src`, copy **backward**.

**Write it yourself: all the classics, with tests**

```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void *my_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--)
        *d++ = *s++;
    return dst;                         /* standard functions return dst */
}

void *my_memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;

    if (d == s || n == 0)
        return dst;
    if ((uintptr_t)d < (uintptr_t)s) {  /* destination below source: copy forward */
        while (n--)
            *d++ = *s++;
    } else {                            /* destination above source: copy backward */
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

void *my_memset(void *dst, int value, size_t n)
{
    uint8_t *d = dst;
    while (n--)
        *d++ = (uint8_t)value;          /* only the low byte of value is used */
    return dst;
}

int my_memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *p = a, *q = b;
    for (; n--; p++, q++)
        if (*p != *q)
            return *p - *q;             /* compared as unsigned char */
    return 0;
}

char *my_strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++) != '\0')
        ;                               /* copies the '\0' too, then stops */
    return ret;
}

/* Safe copy: always terminates, returns strlen(src) so truncation can be detected. */
size_t my_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size != 0) {
        size_t n = (len >= size) ? size - 1 : len;
        memcpy(dst, src, n);
        dst[n] = '\0';
    }
    return len;                         /* len >= size means the result was truncated */
}

int main(void)
{
    char a[] = "ABCDE";
    my_memmove(a + 1, a, 4);            /* overlapping shift right */
    printf("memmove right : %s (expect AABCD)\n", a);

    char b[] = "ABCDE";
    my_memmove(b, b + 1, 4);            /* overlapping shift left */
    printf("memmove left  : %s (expect BCDEE)\n", b);

    char c[8];
    my_memcpy(c, "hello", 6);
    printf("memcpy        : %s\n", c);

    int arr[4];
    my_memset(arr, 0, sizeof arr);
    printf("memset 0      : %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3]);
    my_memset(arr, 1, sizeof arr);      /* classic trap: sets BYTES, not ints */
    printf("memset 1      : %d (0x%08X), not 1!\n", arr[0], (unsigned)arr[0]);

    printf("memcmp        : %d %d\n", my_memcmp("abc", "abd", 3) < 0, my_memcmp("abc", "abc", 3));

    char d[16];
    my_strcpy(d, "copy me");
    printf("strcpy        : %s\n", d);

    char e[4];
    size_t need = my_strlcpy(e, "truncated", sizeof e);
    printf("strlcpy       : \"%s\" (needed %zu, truncated: %s)\n", e, need, need >= sizeof e ? "yes" : "no");
    return 0;
}
```

> **Interview traps:**
> - `memset(arr, 1, sizeof arr)` on an `int` array sets every **byte** to `0x01`, so each int becomes `0x01010101 = 16843009`. Only `0` and `-1` (all bits `0xFF`) work as "fill every int" values.
> - `memset(p, 0, sizeof p)` where `p` is a **pointer** clears only 4 or 8 bytes (the size of the pointer), not the buffer. Use the buffer size.
> - `strcpy` has no bounds check, which makes it the classic **buffer overflow**. Use `snprintf`, `strlcpy` or check the length first.
> - Comparing structs with `memcmp` is unreliable, because **padding bytes** hold garbage (Chapter 7).

**A faster `memcpy` (word copy).** Real implementations copy a machine word (4 or 8 bytes) at a time instead of one byte:

```c
#include <stddef.h>
#include <stdint.h>

void *fast_memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    const uintptr_t mask = sizeof(uint32_t) - 1u;

    /* Words can be used only if both pointers have the SAME misalignment. */
    if ((((uintptr_t)d ^ (uintptr_t)s) & mask) == 0u) {
        while (((uintptr_t)d & mask) != 0u && n > 0u) {   /* 1. head: bytes until aligned */
            *d++ = *s++;
            n--;
        }
        uint32_t *dw = (uint32_t *)d;                     /* 2. body: whole words        */
        const uint32_t *sw = (const uint32_t *)s;
        while (n >= sizeof(uint32_t)) {
            *dw++ = *sw++;
            n -= sizeof(uint32_t);
        }
        d = (uint8_t *)dw;
        s = (const uint8_t *)sw;
    }
    while (n--)                                            /* 3. tail: leftover bytes      */
        *d++ = *s++;
    return dst;
}
```

Caveats to mention: accessing a `uint8_t` buffer through `uint32_t *` technically breaks C's **strict aliasing** rule. Real libraries use compiler attributes such as `__attribute__((may_alias))` or assembly. On cores that cannot do unaligned access (Cortex-M0), this is also why the alignment check matters. Production versions also unroll the loop or use `LDM`/`STM` (multiple-register load/store) on ARM.

### 6.2 Pointer essentials for memory questions

**Pointer arithmetic is scaled by the pointed-to type:**

```c
int  arr[4] = {10, 20, 30, 40};
int *p = arr;            /* array "decays" to a pointer to its first element */
p + 1;                   /* address + 1 * sizeof(int) = +4 bytes, points to arr[1] */
*(p + 2) == arr[2];      /* a[i] is defined as *(a + i)                          */
(char *)p + 1;           /* +1 byte                                              */
```

**`const` and pointers.** Read the declaration **right to left**:

| Declaration | Meaning | Can change the data? | Can change the pointer? |
|---|---|---|---|
| `const int *p` (same as `int const *p`) | Pointer **to const int** | No | Yes |
| `int *const p` | **Const pointer** to int | Yes | No |
| `const int *const p` | Const pointer to const int | No | No |
| `volatile uint32_t *const reg` | Const pointer to a volatile register | Yes | No (typical for hardware registers) |

**Array vs pointer:**

```c
void f(int arr[])           /* the parameter is really 'int *arr'                 */
{
    sizeof arr;             /* size of a POINTER (4 or 8), not of the array       */
}

int a[10];
sizeof a;                   /* 40: the whole array                                */
sizeof a / sizeof a[0];     /* 10 elements: works only where 'a' is a real array  */
&a + 1;                     /* jumps over the WHOLE array (+40 bytes)             */
a + 1;                      /* next element (+4 bytes)                            */
```

**Pointer problems by name:**

| Name | Meaning | Example |
|---|---|---|
| **NULL pointer** | Points to nothing; dereferencing it is undefined behaviour | `int *p = NULL; *p = 1;` |
| **Wild pointer** | Never initialised; contains garbage | `int *p; *p = 1;` |
| **Dangling pointer** | Points to memory that has been freed or gone out of scope | `free(p); *p = 1;` or returning `&local` |
| **Void pointer** | Generic pointer, cannot be dereferenced or used in arithmetic without a cast (GCC allows arithmetic as an extension) | `void *v = &x;` |

### 6.3 Allocating inside a function: why you need a double pointer

```c
#include <stdlib.h>

void alloc_wrong(int *p)
{
    p = malloc(sizeof *p);   /* changes only the LOCAL copy of the pointer: caller sees nothing, memory leaks */
}

void alloc_right(int **pp)
{
    *pp = malloc(sizeof **pp);  /* writes through the address of the caller's pointer */
}

int *alloc_return(void)
{
    return malloc(sizeof(int)); /* often the cleanest option */
}

/* Usage:
   int *x = NULL;
   alloc_wrong(&x)  -> does not compile (int ** passed as int *); alloc_wrong(x) leaves x == NULL
   alloc_right(&x)  -> x now points to the new block
*/
```

C passes **everything by value**, pointers included. To change the caller's pointer, you must pass the **address of that pointer**.

### 6.4 Dynamic 2D arrays

**Question:** "Allocate a `rows × cols` matrix of `int` dynamically, then free it."

```c
#include <stdio.h>
#include <stdlib.h>

/* Method 1: array of row pointers. Rows are separate blocks (not contiguous). */
int **alloc_rows(size_t rows, size_t cols)
{
    int **m = malloc(rows * sizeof *m);
    if (m == NULL)
        return NULL;
    for (size_t r = 0; r < rows; r++) {
        m[r] = malloc(cols * sizeof **m);
        if (m[r] == NULL) {                /* roll back everything allocated so far */
            while (r--)
                free(m[r]);
            free(m);
            return NULL;
        }
    }
    return m;
}

void free_rows(int **m, size_t rows)
{
    for (size_t r = 0; r < rows; r++)
        free(m[r]);
    free(m);
}

/* Method 2: one contiguous data block + row pointers. Only 2 allocations, cache friendly. */
int **alloc_contig(size_t rows, size_t cols)
{
    int **m   = malloc(rows * sizeof *m);
    int *data = malloc(rows * cols * sizeof *data);
    if (m == NULL || data == NULL) {
        free(m);
        free(data);
        return NULL;
    }
    for (size_t r = 0; r < rows; r++)
        m[r] = data + r * cols;            /* each row points into the block */
    return m;
}

void free_contig(int **m)
{
    if (m != NULL) {
        free(m[0]);                        /* the data block */
        free(m);                           /* the row pointer array */
    }
}

int main(void)
{
    size_t rows = 3, cols = 4;

    int **a = alloc_rows(rows, cols);
    int **b = alloc_contig(rows, cols);

    /* Method 3: pointer to a variable-length array type (C99): ONE allocation, real 2D indexing. */
    int (*c)[cols] = malloc(rows * sizeof *c);

    /* Method 4: flat 1D array with manual indexing: element (r, k) is d[r * cols + k]. */
    int *d = malloc(rows * cols * sizeof *d);

    if (a == NULL || b == NULL || c == NULL || d == NULL)
        return 1;

    for (size_t r = 0; r < rows; r++)
        for (size_t k = 0; k < cols; k++) {
            int v = (int)(r * 10 + k);
            a[r][k] = v;
            b[r][k] = v;
            c[r][k] = v;
            d[r * cols + k] = v;
        }

    printf("a[2][3]=%d b[2][3]=%d c[2][3]=%d d[2*4+3]=%d\n", a[2][3], b[2][3], c[2][3], d[2 * cols + 3]);

    free_rows(a, rows);
    free_contig(b);
    free(c);
    free(d);
    return 0;
}
```

| Method | Allocations | Contiguous? | Indexing | Best for |
|---|---|---|---|---|
| 1. Row pointers | rows + 1 | No | `m[r][c]` | Jagged arrays (rows of different length) |
| 2. Contiguous + row pointers | 2 | Yes | `m[r][c]` | Matrices passed to C APIs |
| 3. Pointer to VLA | 1 | Yes | `m[r][c]` | Clean C99 code (VLA *types* are fine even where VLA *objects* are banned) |
| 4. Flat array | 1 | Yes | `m[r * cols + c]` | Embedded code, image buffers, DMA |

### 6.5 Flexible array members

A struct can end with an array of unspecified size. You allocate the header and the data in **one** block:

```c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t len;
    uint8_t  type;
    uint8_t  payload[];      /* flexible array member: must be last, adds 0 to sizeof */
} packet_t;

packet_t *packet_new(uint8_t type, const uint8_t *data, uint16_t len)
{
    packet_t *p = malloc(sizeof *p + len);   /* header + payload in one allocation */
    if (p != NULL) {
        p->len = len;
        p->type = type;
        memcpy(p->payload, data, len);
    }
    return p;                                /* one free(p) releases everything */
}
```

This gives one allocation instead of two, better locality and simpler freeing. It is used everywhere for network packets and messages.

### 6.6 Strict aliasing and safe type punning

The **strict aliasing rule** says you may access an object only through its own type (or a compatible one) or through a **character type**. Reading a `float`'s bits through a `uint32_t *` breaks the rule, and the optimiser may then produce wrong code.

```c
float f = 1.0f;
uint32_t bits_bad  = *(uint32_t *)&f;      /* undefined behaviour under strict aliasing */

uint32_t bits_good;
memcpy(&bits_good, &f, sizeof bits_good);  /* well defined; compiles to a single move */
```

Same rule for parsing buffers: do not cast `uint8_t *rx` to `struct header *`. `memcpy` into a local struct instead. This also avoids **unaligned access** faults (Chapter 7).

---

## 7. Alignment Padding and Packing

### 7.1 What is alignment?

A value is **naturally aligned** when its address is a multiple of its size. A 4-byte `int` at address `0x1000` or `0x1004` is aligned; at `0x1001` it is **misaligned**.

| Type | Typical size | Typical alignment |
|---|---|---|
| `char`, `int8_t` | 1 | 1 |
| `short`, `int16_t` | 2 | 2 |
| `int`, `int32_t`, `float` | 4 | 4 |
| `long` | 4 (32-bit, Windows) / 8 (64-bit Linux) | same as size |
| `long long`, `int64_t`, `double` | 8 | 8 (but **4 inside structs on 32-bit x86 Linux**) |
| Pointer | 4 (32-bit) / 8 (64-bit) | same as size |

### 7.2 Why hardware cares

Memory is connected to the CPU through a bus that is 4 or 8 bytes wide, and it delivers **aligned chunks**. An aligned 4-byte read is one bus transaction. A misaligned 4-byte read crosses a chunk boundary, so it needs **two** transactions plus shifting and merging, or the CPU refuses to do it at all.

```
 Bus words:   | 0x1000 - 0x1003 | 0x1004 - 0x1007 |
 int at 0x1000: [####]                              -> 1 access
 int at 0x1002:      [####]  spans both words       -> 2 accesses or a fault
```

| CPU | Unaligned access |
|---|---|
| **Cortex-M0/M0+** (ARMv6-M) | **Always faults** (HardFault) |
| **Cortex-M3/M4/M7** (ARMv7-M) | Allowed for single `LDR`/`STR`/`LDRH`/`STRH` (slower). **Faults** for `LDRD`/`STRD`/`LDM`/`STM`. Can be made to always fault with `SCB->CCR.UNALIGN_TRP`. Never allowed on Device/Strongly-ordered memory |
| **RISC-V** | The ISA allows an implementation to support it in hardware, trap, or emulate it in a trap handler (very slow). Never rely on it |
| **ESP32 (Xtensa)** | `LoadStoreAlignment` exception for misaligned 16/32-bit accesses |
| **x86** | Allowed (small penalty), except aligned SSE/AVX instructions |

**How misaligned pointers get created.** Usually by casting a byte buffer to a wider type:

```c
uint8_t rx[16];                         /* received bytes                          */
uint32_t v = *(uint32_t *)&rx[1];       /* misaligned (and strict-aliasing UB)     */

uint32_t w;
memcpy(&w, &rx[1], sizeof w);           /* safe on every CPU; the compiler picks the best code */
```

### 7.3 Structure padding rules

The compiler inserts invisible **padding bytes** so that every member is aligned. Three rules:

1. **Each member** starts at an offset that is a multiple of **its own alignment**.
2. The **whole struct's alignment** is the **largest alignment** of any member.
3. The **struct's size** is rounded up to a multiple of its alignment (**tail padding**), so that every element of an **array** of structs is still aligned.

**Worked example:**

```c
struct A { char c; int i; char d; };
```

```
offset: 0    1    2    3    4    5    6    7    8    9    10   11
       [c ][pad][pad][pad][ i  i  i  i  ][d ][pad][pad][pad]
        c at 0; i needs a multiple of 4 -> 4; d at 8; size rounds up to 12 (multiple of 4)
sizeof(struct A) = 12, of which 6 bytes are padding
```

Reordering the same members **largest first** removes most of the padding:

```c
struct B { int i; char c; char d; };   /* 4 + 1 + 1 + 2 tail pad = 8 */
```

**Write it yourself / predict the output:**

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct A { char c; int i; char d; };            /* 12 */
struct B { int i; char c; char d; };            /*  8 */
struct C { char c; double d; int i; };          /* 24 on x86-64 and ARM (16 on 32-bit x86 Linux) */
struct D { double d; int i; char c; };          /* 16 */
struct E { char a; short b; char c; int d; };   /* 12 */
struct F { char a; char c; short b; int d; };   /*  8 */
struct G { char a; char b; char c; };           /*  3: alignment 1, no padding needed */
struct H { struct B inner; char x; };           /* 12: inner has alignment 4 */

struct __attribute__((packed)) P { char c; int i; };   /* 5: no padding at all */

int main(void)
{
    printf("A=%zu B=%zu C=%zu D=%zu E=%zu F=%zu G=%zu H=%zu P=%zu\n",
           sizeof(struct A), sizeof(struct B), sizeof(struct C), sizeof(struct D),
           sizeof(struct E), sizeof(struct F), sizeof(struct G), sizeof(struct H),
           sizeof(struct P));
    printf("offsets in A: c=%zu i=%zu d=%zu\n",
           offsetof(struct A, c), offsetof(struct A, i), offsetof(struct A, d));
    printf("offsets in C: c=%zu d=%zu i=%zu\n",
           offsetof(struct C, c), offsetof(struct C, d), offsetof(struct C, i));
    printf("alignof(struct C)=%zu alignof(struct P)=%zu\n",
           _Alignof(struct C), _Alignof(struct P));
    return 0;
}
```

Expected on x86-64 Linux or ARM: `A=12 B=8 C=24 D=16 E=12 F=8 G=3 H=12 P=5`.

> **Interview tip.** To minimise padding, **sort members by alignment, largest first** (doubles and pointers, then ints, then shorts, then chars). Group small members together. On memory-starved MCUs this can save kilobytes across large arrays of structs.

### 7.4 Packed structures

```c
struct __attribute__((packed)) frame {   /* GCC/Clang */
    uint8_t  id;
    uint32_t value;                       /* offset 1: misaligned! */
    uint16_t crc;
};                                        /* sizeof = 7 */

#pragma pack(push, 1)                     /* GCC, Clang, MSVC */
struct frame2 { uint8_t id; uint32_t value; uint16_t crc; };
#pragma pack(pop)
```

**Why packing is used:** to match an exact **wire format** or **file format** (protocol frames, BMP headers, flash records), or to match a hardware layout.

**What it costs:**

- The compiler must generate **slow byte-by-byte** code for misaligned members on CPUs that cannot do unaligned access.
- **Taking the address of a packed member is dangerous.** `uint32_t *p = &f.value;` produces a misaligned pointer, and later code that uses `p` assumes alignment and can fault. GCC warns with `-Waddress-of-packed-member`.
- It is not portable: endianness and padding still differ between compilers and CPUs.

**The better practice** for protocols is to keep normal structs inside the program and **serialise and deserialise explicitly** with shifts or `memcpy`. That handles both alignment and endianness.

### 7.5 `offsetof` and `container_of`

`offsetof(type, member)` from `<stddef.h>` gives the byte offset of a member. The classic hand-made version:

```c
#define MY_OFFSETOF(type, member)  ((size_t)&(((type *)0)->member))
/* "Pretend a struct sits at address 0; the address of the member IS its offset."
   Technically undefined behaviour, which is why <stddef.h> provides the real one (often __builtin_offsetof). */
```

`container_of` (Linux kernel, also used in RTOSes) goes **from a pointer to a member back to the enclosing struct**. It is the basis of the kernel's intrusive linked lists:

```c
#include <stddef.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

struct list_node { struct list_node *next, *prev; };

struct device {
    int id;
    struct list_node node;          /* embedded list hook */
    char name[16];
};

/* Given a pointer to the embedded 'node', recover the whole device: */
struct device *dev_from_node(struct list_node *n)
{
    return container_of(n, struct device, node);
}
```

### 7.6 Alignment macros

**Write it yourself** (`a` must be a power of two):

```c
#include <stdint.h>

#define ALIGN_UP(x, a)    (((uintptr_t)(x) + ((uintptr_t)(a) - 1u)) & ~((uintptr_t)(a) - 1u))
#define ALIGN_DOWN(x, a)  ((uintptr_t)(x) & ~((uintptr_t)(a) - 1u))
#define IS_ALIGNED(x, a)  (((uintptr_t)(x) & ((uintptr_t)(a) - 1u)) == 0u)
#define IS_POWER_OF_2(n)  ((n) != 0 && (((n) & ((n) - 1)) == 0))

/* ALIGN_UP(13, 8) = 16, ALIGN_UP(16, 8) = 16, ALIGN_DOWN(13, 8) = 8, IS_ALIGNED(0x1004, 4) = 1 */
```

> **A subtle bug interviewers love.** `#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))` used with a **64-bit** `x` and an **unsigned int** `a = 8u`: `~(8u - 1u)` is computed in **32 bits** (`0xFFFFFFF8`), then zero-extended to 64 bits (`0x00000000FFFFFFF8`), which **wipes out the upper 32 bits** of the address. Always cast the mask to the width of `x` (as above with `uintptr_t`).

**Requesting alignment:**

```c
#include <stdalign.h>

alignas(32) static uint8_t dma_buf[512];                        /* C11 / C++11              */
static uint8_t dma_buf2[512] __attribute__((aligned(32)));      /* GCC/Clang                */
_Static_assert(alignof(max_align_t) >= 8, "unexpected max alignment");
```

### 7.7 Bit fields

```c
struct status {
    unsigned int ready : 1;
    unsigned int error : 1;
    unsigned int mode  : 3;
    unsigned int       : 3;     /* unnamed: padding bits */
    unsigned int count : 8;
};                               /* sizeof is usually 4 (one unsigned int "storage unit") */
```

- Bit fields save memory for flags.
- **Almost everything about their layout is implementation-defined**: bit order (LSB-first or MSB-first), whether fields may straddle storage units, the signedness of plain `int` fields and padding.
- You **cannot take the address** of a bit field.
- Writing one field is a **read-modify-write** of the whole storage unit, which is **not atomic**. If an ISR updates another field in the same word, an update can be lost.
- Avoid bit fields for **hardware registers** and **wire formats** in portable code. Use masks and shifts. Vendor headers (CMSIS) define `_Pos`/`_Msk` macros for exactly this reason.

---

## 8. Memory Mapped IO and the volatile Keyword

### 8.1 Memory-mapped I/O

In **memory-mapped I/O (MMIO)**, peripheral registers (GPIO, UART, timers) appear at **ordinary addresses** in the CPU's address space. Reading or writing those addresses talks to the hardware instead of RAM. The same load/store instructions work for both.

The ARM Cortex-M address map (fixed by the architecture, 4 GB):

```
0xFFFFFFFF ┌──────────────────────────────┐
           │ Vendor specific              │
0xE0100000 ├──────────────────────────────┤
           │ Private Peripheral Bus (PPB) │  NVIC, SysTick, SCB, MPU, debug
0xE0000000 ├──────────────────────────────┤
           │ External device              │
0xA0000000 ├──────────────────────────────┤
           │ External RAM                 │  FMC/SDRAM, QSPI
0x60000000 ├──────────────────────────────┤
           │ Peripherals                  │  GPIO, UART, SPI, timers ...
0x40000000 ├──────────────────────────────┤
           │ SRAM                         │  on-chip RAM
0x20000000 ├──────────────────────────────┤
           │ Code                         │  flash, boot memory, vector table
0x00000000 └──────────────────────────────┘
```

| | Memory-mapped I/O | Port-mapped I/O |
|---|---|---|
| Address space | Shared with memory | Separate I/O space |
| Instructions | Normal loads/stores (`LDR`/`STR`, `MOV`) | Special instructions (`IN`/`OUT` on x86) |
| Used by | ARM, RISC-V, Xtensa, almost all MCUs | x86 legacy devices |
| Pros | Every addressing mode and C pointer works | Does not use up memory address space |

### 8.2 Accessing a register from C

**Method 1: a macro that dereferences a volatile pointer**

```c
#include <stdint.h>

#define GPIOA_BASE  0x40020000u                                   /* STM32F4 */
#define GPIOA_ODR   (*(volatile uint32_t *)(GPIOA_BASE + 0x14u))  /* output data register */

void led_on(void)  { GPIOA_ODR |=  (1u << 5); }
void led_off(void) { GPIOA_ODR &= ~(1u << 5); }
```

Reading the cast from the inside out: take the number `0x40020014`, treat it as the **address of a volatile 32-bit unsigned integer**, then **dereference** it. The result can be read and assigned like a variable.

**Method 2: a struct overlay (CMSIS style)**

```c
#include <stdint.h>

typedef struct {
    volatile uint32_t MODER;        /* 0x00 mode                                  */
    volatile uint32_t OTYPER;       /* 0x04 output type                           */
    volatile uint32_t OSPEEDR;      /* 0x08 speed                                 */
    volatile uint32_t PUPDR;        /* 0x0C pull-up / pull-down                   */
    volatile const uint32_t IDR;    /* 0x10 input data (read-only => const)       */
    volatile uint32_t ODR;          /* 0x14 output data                           */
    volatile uint32_t BSRR;         /* 0x18 bit set / reset (write-only)          */
    volatile uint32_t LCKR;         /* 0x1C lock                                  */
    volatile uint32_t AFR[2];       /* 0x20, 0x24 alternate function              */
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40020000u)

void led_toggle_safe(void)
{
    if (GPIOA->ODR & (1u << 5))
        GPIOA->BSRR = 1u << (5 + 16);   /* reset pin 5: a single write, atomic   */
    else
        GPIOA->BSRR = 1u << 5;          /* set pin 5                              */
}
```

The struct gives readable names, and the compiler computes the offsets. The layout **must exactly** match the reference manual. Add `_Static_assert(offsetof(GPIO_TypeDef, BSRR) == 0x18, "layout");` to catch mistakes. CMSIS writes `__IO` for `volatile` and `__I` for `volatile const`.

### 8.3 What `volatile` really means

`volatile` tells the compiler: **"this object can change, or be observed, in ways you cannot see. Perform every read and every write exactly as written in the source, in that order."**

Without `volatile`, the optimiser assumes that only the code it can see changes a variable. It may then:

1. **Keep the value in a register** and never read memory again.
2. **Delete writes** that seem to hav
