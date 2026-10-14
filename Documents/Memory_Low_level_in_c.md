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
2. **Delete writes** that seem to have no effect (writing the same register twice).
3. **Merge or remove reads** (read once and reuse the value).
4. **Delete whole loops** whose results seem unused.

**Example 1: a flag set by an interrupt**

```c
#include <stdbool.h>

static volatile bool data_ready = false;   /* written in the ISR, read in main */

void USART1_IRQHandler(void)
{
    data_ready = true;
}

int main(void)
{
    while (!data_ready) {
        /* wait. Without volatile, -O2 may load data_ready once and loop forever */
    }
    /* ... */
}
```

**Example 2: polling a status register**

```c
#define UART_SR  (*(volatile uint32_t *)0x40011000u)
#define TXE      (1u << 7)

while ((UART_SR & TXE) == 0) { }   /* every iteration must READ the hardware register */
```

**When to use `volatile`:**

1. **Memory-mapped hardware registers.**
2. **Global variables shared between an ISR and normal code.**
3. Memory written by **DMA** or another bus master that the CPU then reads.
4. Local variables modified between `setjmp` and `longjmp`.
5. Delay loops you do not want optimised away (better: use a hardware timer).

### 8.4 What `volatile` does NOT do

| Myth | Reality |
|---|---|
| "volatile makes it atomic" | **No.** `counter++` on a volatile is still load, add, store: three steps that an ISR can interrupt |
| "volatile is a memory barrier" | **No.** The compiler may still move **non-volatile** accesses across volatile ones, and the CPU may reorder accesses on multi-core systems (Chapter 17) |
| "volatile makes code thread-safe" | **No.** Use a mutex, a critical section or C11 atomics |
| "volatile is needed for variables shared between RTOS tasks" | Normally not, when the variable is protected by a mutex or semaphore. RTOS calls act as compiler barriers |

**The lost-update race:**

```c
volatile uint32_t events;               /* volatile, but still racy */

void TIM2_IRQHandler(void) { events++; }

void task(void)
{
    events++;    /* LDR r0,[events] ... (IRQ fires here and increments) ... ADD; STR -> one increment LOST */
}

/* Fix: make the read-modify-write indivisible */
void task_fixed(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    events++;
    __set_PRIMASK(primask);
}
/* Or with C11 atomics on cores that have LDREX/STREX (Cortex-M3 and up):
   atomic_fetch_add(&events_atomic, 1); */
```

**`const volatile`** means "my code must not write it, but it can change by itself". That describes a read-only status or input register: `const volatile uint32_t *const STATUS = (const volatile uint32_t *)0x40011000u;`

### 8.5 Read-modify-write hazards and their fixes

`GPIOA->ODR |= (1u << 5);` is **read, modify, write**. If an interrupt changes another bit of `ODR` between the read and the write, that change is lost when the stale value is written back.

**Fixes:**

1. **Set/reset registers.** STM32 `BSRR` and similar registers on other MCUs: writing `1` to bit *n* sets pin *n*, and writing to bit *n+16* resets it. **Writing 0 has no effect**, so a single write changes only the bits you want and is atomic.
2. **Critical section** around the read-modify-write.
3. **Bit-banding** (Cortex-M3/M4 only, **not** M0, M7 or M33). Every bit in the first 1 MB of SRAM (`0x20000000`) and of the peripheral region (`0x40000000`) has its own 32-bit **alias word**. Writing to the alias word changes exactly one bit, atomically:

```c
/* alias_address = alias_base + (byte_offset * 32) + (bit_number * 4) */
#define BITBAND_PERIPH(addr, bit) \
    (*(volatile uint32_t *)(0x42000000u + (((uint32_t)(addr) - 0x40000000u) * 32u) + ((bit) * 4u)))
#define BITBAND_SRAM(addr, bit) \
    (*(volatile uint32_t *)(0x22000000u + (((uint32_t)(addr) - 0x20000000u) * 32u) + ((bit) * 4u)))

BITBAND_PERIPH(&GPIOA->ODR, 5) = 1;   /* set ODR bit 5 atomically */
```

**Write-1-to-clear (W1C) registers.** Many interrupt status registers clear a flag when you write `1` to it. **Never use `|=` on them**: `REG |= FLAG_A` reads every flag that is currently set and writes them all back as 1, which **clears every pending flag**, not just `FLAG_A`. Write only the bit you mean: `REG = FLAG_A;`.

### 8.6 Register bit manipulation toolkit

```c
#define BIT(n)                    (1u << (n))
#define SET_BIT(reg, n)           ((reg) |=  BIT(n))
#define CLEAR_BIT(reg, n)         ((reg) &= ~BIT(n))
#define TOGGLE_BIT(reg, n)        ((reg) ^=  BIT(n))
#define READ_BIT(reg, n)          (((reg) >> (n)) & 1u)

/* Multi-bit field: write 'val' into the field described by mask/pos, leaving other bits alone. */
#define FIELD_MASK(width, pos)    (((1u << (width)) - 1u) << (pos))
#define WRITE_FIELD(reg, mask, pos, val) \
    ((reg) = ((reg) & ~(mask)) | (((uint32_t)(val) << (pos)) & (mask)))

/* Example: set GPIO pin 5 mode (2 bits at position 10) to "output" (0b01). */
/* WRITE_FIELD(GPIOA->MODER, FIELD_MASK(2, 10), 10, 1u); */
```

---

## 9. Contiguous Allocation and Paging

This chapter and the next two are classic **operating systems** theory. Linux-capable processors (Cortex-A, x86, RISC-V with Sv39) do all of this in hardware. Microcontrollers usually do not, but interviewers still ask, and the ideas explain why MCUs work the way they do.

### 9.1 The problem

Many processes share one physical RAM. The OS must:

1. **Relocate.** A program is compiled once but may be loaded anywhere.
2. **Protect.** Process A must not read or corrupt B or the kernel.
3. **Share.** Several processes should share one copy of common code, such as libc.
4. **Use memory efficiently.** No waste, and run programs that are larger than RAM.

### 9.2 Base and limit registers: the simplest MMU

Each process gets **one contiguous block** of physical memory. Two CPU registers describe it:

- **Base (relocation) register:** where the block starts in physical memory.
- **Limit register:** how big the block is.

```
 logical address ──► [ addr < limit ? ] ──no──► TRAP: addressing error
                            │ yes
                            ▼
                     physical = addr + base ──► RAM
```

**Example:** base = 300040, limit = 120900. Logical address 100 → physical 300140. Logical address 130000 → **trap**, because 130000 ≥ 120900.

Only the kernel (privileged mode) may change these registers. On a context switch, the OS loads the next process's base and limit.

### 9.3 Partitioning schemes

| Scheme | How | Problem |
|---|---|---|
| **Fixed partitions** | Memory is divided into fixed-size partitions at boot. One process per partition | **Internal fragmentation** (a small process wastes the rest of its partition). The number of processes is fixed |
| **Variable (dynamic) partitions** | Each process gets exactly the size it needs from a free **hole** | **External fragmentation** (holes between processes). Needs a placement policy: first, best or worst fit (Chapter 4.9) |

**Compaction** shuffles processes together to merge the holes into one large hole. It is possible only with **run-time binding** (base registers), and it is expensive because whole processes must be copied.

### 9.4 Swapping

**Swapping** moves a whole process from RAM to disk (the **backing store**) to make room, and brings it back later, possibly to a different address if binding happens at run time.

- **Cost:** transferring a 100 MB process at 50 MB/s takes 2 s out and 2 s back in. That is too slow for normal operation.
- Modern systems swap **pages**, not whole processes (paging, below).
- **Mobile and embedded systems** usually **do not swap** at all. Flash has limited write endurance and is slow. Android and iOS instead ask apps to release memory and kill background apps under pressure. Embedded Linux often uses **zram** (a compressed swap device in RAM).

### 9.5 Paging: the core idea

Paging eliminates external fragmentation by giving up the requirement that a process be contiguous in **physical** memory.

- **Logical memory** is divided into fixed-size blocks called **pages**.
- **Physical memory** is divided into blocks of the **same size** called **frames**.
- **Any page can go into any free frame.** A **page table** per process records which frame holds each page.

```
 Logical memory           Page table          Physical memory (frames)
 (process view)           page -> frame
 ┌──────────┐              ┌───┬───┐          frame 0 ┌──────────┐
 │ page 0   │──────────────│ 0 │ 1 │─┐                │          │
 ├──────────┤              ├───┼───┤ └──────► frame 1 │ page 0   │
 │ page 1   │──────────────│ 1 │ 4 │──┐               ├──────────┤
 ├──────────┤              ├───┼───┤  │       frame 2 │ page 2   │◄─┐
 │ page 2   │──────────────│ 2 │ 2 │──┼───────────────┘          │  │
 ├──────────┤              ├───┼───┤  │       frame 3 │ (other)  │  │
 │ page 3   │──────────────│ 3 │ 7 │  │               ├──────────┤  │
 └──────────┘              └───┴───┘  └─────► frame 4 │ page 1   │  │
                                                       ...
 Contiguous for the process    Scattered in RAM; no external fragmentation
```

**Properties:**

- **No external fragmentation.** Every free frame is usable.
- **Internal fragmentation** only in the **last page** of each process, on average **half a page** per process.
- Page sizes are powers of two, typically **4 KB**, with **2 MB/1 GB huge pages** on x86-64 and 64 KB options on ARM.
- The OS keeps a **frame table** that records which frames are free and who owns the used ones.

### 9.6 Address translation

The page size is a power of two (2ⁿ bytes), so splitting an address needs **no division**, only bit fields:

```
 m-bit logical address
 ┌─────────────────────────┬──────────────────┐
 │ page number p           │ page offset d    │
 │ (upper m - n bits)      │ (lower n bits)   │
 └─────────────────────────┴──────────────────┘

 physical address = (frame_number << n) | d
```

**Worked example (hex).** 32-bit addresses, 4 KB pages (n = 12). Logical address `0x12345678`:

- Page number = `0x12345678 >> 12` = **`0x12345`**
- Offset = `0x12345678 & 0xFFF` = **`0x678`**
- If page `0x12345` sits in frame `0x00ABC`, the physical address = `0x00ABC000 | 0x678` = **`0x00ABC678`**

**Worked example (decimal).** Page size 1024 bytes, logical address 10000:

- Page = 10000 / 1024 = **9** (9 × 1024 = 9216)
- Offset = 10000 − 9216 = **784**
- If page 9 is in frame 3, physical = 3 × 1024 + 784 = **3856**

**Write it yourself: simulate a page table lookup**

```c
#include <stdint.h>
#include <stdio.h>

#define PAGE_SHIFT  12u                        /* 4 KB pages                      */
#define PAGE_SIZE   (1u << PAGE_SHIFT)
#define OFFSET_MASK (PAGE_SIZE - 1u)
#define NUM_PAGES   16u                        /* tiny address space for the demo */

typedef struct {
    uint32_t frame;                            /* physical frame number           */
    uint8_t  valid;                            /* 1 = page is in physical memory  */
    uint8_t  writable;                         /* 1 = writes allowed              */
} pte_t;

static const pte_t page_table[NUM_PAGES] = {
    [0] = { 5, 1, 0 },                         /* page 0 -> frame 5, read-only (code) */
    [1] = { 9, 1, 1 },                         /* page 1 -> frame 9, read/write       */
    [2] = { 2, 1, 1 },                         /* the remaining pages are invalid     */
};

/* 0 = success, -1 = page fault, -2 = protection fault */
static int translate(uint32_t vaddr, int is_write, uint32_t *paddr)
{
    uint32_t page   = vaddr >> PAGE_SHIFT;     /* upper bits: page number         */
    uint32_t offset = vaddr & OFFSET_MASK;     /* lower 12 bits: offset           */

    if (page >= NUM_PAGES || !page_table[page].valid)
        return -1;
    if (is_write && !page_table[page].writable)
        return -2;

    *paddr = (page_table[page].frame << PAGE_SHIFT) | offset;
    return 0;
}

int main(void)
{
    const struct { uint32_t va; int write; } tests[] = {
        { 0x00000ABCu, 0 },                    /* page 0 read   -> frame 5          */
        { 0x00001234u, 1 },                    /* page 1 write  -> frame 9          */
        { 0x00000010u, 1 },                    /* page 0 write  -> protection fault */
        { 0x00005000u, 0 },                    /* page 5        -> page fault       */
    };

    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        uint32_t pa = 0;
        int r = translate(tests[i].va, tests[i].write, &pa);
        if (r == 0)
            printf("VA 0x%08X -> PA 0x%08X\n", (unsigned)tests[i].va, (unsigned)pa);
        else
            printf("VA 0x%08X -> %s\n", (unsigned)tests[i].va,
                   r == -1 ? "PAGE FAULT" : "PROTECTION FAULT");
    }
    return 0;
}
```

### 9.7 The page table entry (PTE)

Besides the frame number, every PTE carries control bits that the hardware checks or updates on **every** access:

| Bit | Meaning | Used for |
|---|---|---|
| **Valid / Present** | The page is in RAM | Clear → **page fault** on access |
| **Read / Write / Execute** | Allowed access types | Protection, **NX (no-execute)** for the stack and heap |
| **User / Supervisor** | User code may access it? | Kernel memory protection |
| **Accessed (Referenced)** | Set by hardware on any access | Page replacement (LRU approximations, Chapter 11) |
| **Dirty (Modified)** | Set by hardware on a write | Replacement: a clean page need not be written back to disk |
| **Cache disable / Write-through** | Caching policy for this page | Memory-mapped I/O pages must not be cached |
| **Global** | Do not flush from the TLB on a context switch | Kernel pages shared by all processes |

The page table lives **in RAM**. A CPU register points to the current process's table: **CR3** on x86, **TTBR0/TTBR1** on ARM, **satp** on RISC-V. A context switch changes this register.

**The problem:** without help, **every** memory access becomes **two** accesses, one to read the PTE and one for the data. The **TLB** (Chapter 10) solves this.

### 9.8 Page table size and why it is a problem

**Calculation (memorise this one).** 32-bit logical address, 4 KB pages, 4-byte PTEs:

- Offset bits = log₂(4096) = 12
- Page number bits = 32 − 12 = 20 → **2²⁰ = 1,048,576 pages**
- Page table size = 2²⁰ × 4 B = **4 MB per process**, and it must be contiguous

With 100 processes that is 400 MB just for page tables, mostly describing **unused** address space. With 64-bit addresses a flat table is impossible: 2⁵² entries × 8 B = 32 PB.

The solutions are **hierarchical**, **hashed** and **inverted** page tables.

### 9.9 Hierarchical (multi-level) page tables

**Idea:** page the page table itself. A small **outer** table points to **inner** tables, and inner tables are created **only for regions that are actually used**.

**Two-level paging on 32-bit x86 (10 / 10 / 12):**

```
 32-bit virtual address
 ┌────────────┬────────────┬──────────────┐
 │ p1 (10 bit)│ p2 (10 bit)│ offset (12)  │
 └─────┬──────┴─────┬──────┴──────┬───────┘
       │            │             │
 CR3 ──► Page directory (1024 entries, 4 KB)
       │  entry[p1] ──► Page table (1024 entries, 4 KB)
       │                  entry[p2] ──► frame number
       │                                   │
       └─────────────► physical address = frame | offset
```

```c
uint32_t p1     = (va >> 22) & 0x3FF;   /* bits 31..22: page directory index */
uint32_t p2     = (va >> 12) & 0x3FF;   /* bits 21..12: page table index     */
uint32_t offset =  va        & 0xFFF;   /* bits 11..0                        */
/* va = 0x12345678 -> p1 = 0x048, p2 = 0x345, offset = 0x678 */
```

**Saving:** a process that uses 8 MB of code and data at the bottom and an 8 MB stack at the top needs the directory (4 KB) plus 4 page tables (each covers 4 MB) = **20 KB instead of 4 MB**.

**Cost:** a TLB miss now needs **one memory access per level** (a "page walk"). This is why the TLB matters so much.

| Architecture | Scheme | Levels | Split (bits) |
|---|---|---|---|
| x86 32-bit (no PAE) | 2-level, 4 KB pages | 2 | 10 / 10 / 12 |
| x86-64 | 4-level paging (48-bit VA) | 4 | 9 / 9 / 9 / 9 / 12 |
| x86-64 with LA57 | 5-level (57-bit VA) | 5 | 9 × 5 / 12 |
| ARMv7-A short descriptor | L1: 4096 × 1 MB sections, L2: 256 × 4 KB pages | 2 | 12 / 8 / 12 |
| ARMv8-A, 4 KB granule | 48-bit VA | 4 | 9 / 9 / 9 / 9 / 12 |
| RISC-V **Sv32** (RV32) | 4-byte PTEs, 34-bit physical | 2 | 10 / 10 / 12 |
| RISC-V **Sv39** (RV64) | 8-byte PTEs, 512 per table | 3 | 9 / 9 / 9 / 12 |
| RISC-V **Sv48 / Sv57** | | 4 / 5 | 9 per level / 12 |

**How many levels? (a common numerical).** 48-bit virtual address, 4 KB pages, 8-byte PTEs:

- A 4 KB table holds 4096 / 8 = 512 = 2⁹ entries, so each level resolves **9 bits**.
- Bits to translate = 48 − 12 = 36 → 36 / 9 = **4 levels**.

**Large pages fall out naturally.** If a higher-level entry maps memory directly instead of pointing to another table, you get a **huge page**: 2 MB (one level up) or 1 GB (two levels up) on x86-64, 1 MB "sections" on ARMv7-A.

### 9.10 Hashed and inverted page tables

**Hashed page table.** The virtual page number is hashed into a table, and each bucket holds a chain of (virtual page, frame) pairs. This suits large and sparse address spaces. A variant, the **clustered page table**, stores several consecutive pages per entry.

**Inverted page table.** Instead of one table per process, there is **one table for the whole system** with **one entry per physical frame**. Each entry records **which (process id, virtual page)** lives in that frame.

| | Normal page table | Inverted page table |
|---|---|---|
| Entries | One per **virtual page**, per process | One per **physical frame**, system-wide |
| Size | Grows with the virtual address space | Fixed, proportional to RAM |
| Lookup | Direct index, fast | Must **search** for (pid, page): slow, so a hash table is added |
| Shared memory | Easy: two PTEs point to one frame | Hard: a frame records only one owner |
| Used by | x86, ARM, RISC-V | PowerPC, IA-64 (Itanium), UltraSPARC |

### 9.11 Shared pages and protection

- **Shared pages.** Read-only **reentrant** code (libc, the text of an editor run by 40 users) can be mapped into many processes' page tables, pointing at the **same frames**. Forty users of a 150 KB program then need one copy instead of forty.
- **Protection.** The valid bit catches accesses outside the process. R/W/X bits catch writes to code and **execution of data** (the NX/XN bit stops code injected into the stack or heap from running).

### 9.12 Choosing the page size

| Smaller pages (e.g. 4 KB) | Larger pages (e.g. 2 MB) |
|---|---|
| Less internal fragmentation (half a page per process on average) | Smaller page tables |
| Load only what is needed (finer granularity) | Fewer TLB misses (each entry covers more memory: larger **TLB reach**) |
| More page faults, bigger tables, more TLB misses | More internal fragmentation, slower to copy and swap one page |

The trend is toward larger pages and **mixed** page sizes. Databases and JVMs use 2 MB or 1 GB **huge pages** to cut TLB misses.

---

## 10. Virtual Memory in Depth

### 10.1 The TLB (Translation Lookaside Buffer)

Paging turns every memory access into **two**: one for the page table entry and one for the data. Multi-level tables make it **five** on x86-64. The fix is a small, very fast cache of recent translations inside the MMU, the **TLB**.

- It is **associative memory**: all entries are compared with the page number **in parallel**, in a fraction of a cycle.
- Each entry holds `page number → frame number` plus the permission bits (and often an ASID, see below).
- It is small: typically **32-128 entries** in L1 and up to ~1,500-2,000 in L2 on desktop CPUs.

```
           virtual address [ p | d ]
                   │
                   ▼
           ┌───────────────┐   hit (≈99%)
           │      TLB      │──────────────► frame f ──► physical [ f | d ] ──► memory
           └───────┬───────┘
                   │ miss
                   ▼
       Page-table walk in memory (1 access per level)
                   │
           ┌───────┴────────┐
        PTE valid        PTE invalid
           │                │
   load entry into TLB   PAGE FAULT (trap to the OS)
   and retry
```

**Who handles a TLB miss?**

- **Hardware-managed** (x86, ARM, most RISC-V): a hardware **page-table walker** reads the tables by itself.
- **Software-managed** (MIPS, some older RISCs): the miss traps to the OS, which loads the TLB entry with special instructions.

### 10.2 Effective access time with a TLB

```
EAT = h × (t_TLB + t_mem) + (1 − h) × (t_TLB + 2 × t_mem)      (single-level page table)

h = TLB hit ratio, t_TLB = TLB lookup time, t_mem = memory access time
```

**Example.** TLB lookup 20 ns, memory access 100 ns, hit ratio 80%:

- Hit: 20 + 100 = 120 ns
- Miss: 20 + 100 (page table) + 100 (data) = 220 ns
- EAT = 0.8 × 120 + 0.2 × 220 = 96 + 44 = **140 ns**, a 40% slowdown compared with 100 ns.

With a 98% hit ratio: 0.98 × 120 + 0.02 × 220 = **122 ns**. Real hit ratios exceed 99%, thanks to locality.

> If the question ignores the TLB lookup time (newer textbooks do), use `EAT = h × t_mem + (1 − h) × 2 × t_mem`. With an *n*-level table, a miss costs `(n + 1) × t_mem`.

### 10.3 The TLB and context switches

The TLB holds translations for the **current** process. Process B's page 5 is not process A's page 5. On a context switch there are two options:

1. **Flush the whole TLB.** Simple, but the next process starts with a "cold" TLB and suffers many misses.
2. **Tag entries with an ASID** (Address Space Identifier). Each entry records which process it belongs to, so entries from several processes can coexist and **no flush is needed**. ARM calls it ASID, x86 calls it PCID, RISC-V puts an ASID field in `satp`.

**Global pages** (kernel mappings, identical in every process) are marked so that they are never flushed.

**TLB shootdown (multi-core).** When the OS changes or removes a mapping, **every core** that might have cached it must invalidate its TLB entry. The OS sends an **inter-processor interrupt (IPI)** to the other cores and waits for them. This is expensive, which is one reason `munmap` and `mprotect` are not free.

### 10.4 Segmentation

**Segmentation** divides a program into **logical units** of different sizes, as a programmer sees it: code, data, stack, heap, symbol table. A logical address is a pair **(segment number, offset)**.

```
 logical address (s, d)
        │
        ▼
 Segment table (one entry per segment)
 ┌─────┬────────┬────────┐
 │ seg │ base   │ limit  │
 ├─────┼────────┼────────┤
 │  0  │ 1400   │ 1000   │
 │  1  │ 6300   │  400   │
 │  2  │ 4300   │  400   │
 │  3  │ 3200   │ 1100   │
 └─────┴────────┴────────┘
 if d < limit[s]:  physical = base[s] + d   else: TRAP (segmentation fault)
```

**Examples with the table above:**

- (2, 53) → 4300 + 53 = **4353**
- (3, 852) → 3200 + 852 = **4052**
- (0, 1222) → 1222 ≥ 1000 → **trap**

This is where the name **"segmentation fault"** comes from.

| | Paging | Segmentation |
|---|---|---|
| Block size | Fixed (pages) | Variable (segments) |
| Visible to the programmer? | No | Yes: matches logical units |
| Fragmentation | Internal (last page) | **External** |
| Address | Single linear number, split by hardware | Two parts: (segment, offset) |
| Protection and sharing | Per page | Per logical unit (natural: "code segment read-only") |
| Table | Page table (can be huge) | Segment table (small) |

**Segmentation with paging.** Classic 32-bit x86 used **both**: a segment selector produced a linear address, and paging then translated it. **64-bit x86 mostly disables segmentation** (flat model, base = 0), except for the FS and GS registers used for **thread-local storage**. ARM and RISC-V use paging only.

### 10.5 Virtual memory

**Virtual memory** separates the memory a program **sees** from the physical memory it **gets**. Only the parts in active use need to be in RAM. The rest can stay on disk or not exist yet.

**Benefits:**

1. Programs can be **larger than physical memory**.
2. More programs fit in memory at once, giving better CPU utilisation.
3. Each process has a **private, simple, contiguous** address space starting near 0.
4. **Isolation and protection** between processes.
5. Efficient **sharing**: shared libraries, shared memory and copy-on-write `fork`.
6. **Less I/O** at start-up: only the pages that are used get loaded.

**Typical Linux process address space (x86-64):**

```
0x7FFF_FFFF_FFFF ┌──────────────────────────┐
                 │ stack (grows down)       │  + guard page below it
                 │ ...                      │
                 │ memory-mapped region     │  shared libraries, mmap(), thread stacks
                 │ ...                      │
                 │ heap (grows up, brk)     │
                 │ .bss / .data             │
                 │ .rodata / .text          │
0x0000_0040_0000 │ (PIE: randomised base)   │
0x0000_0000_0000 └──────────────────────────┘  page 0 unmapped: NULL dereference -> SIGSEGV
Upper half (0xFFFF_8000_0000_0000 and above): kernel space, not accessible from user mode
```

You can see a real one with `cat /proc/self/maps`.

### 10.6 Demand paging

**Demand paging** loads a page **only when it is first accessed**, never in advance. It is a "lazy" approach.

- When a process starts, its page table entries are marked **invalid**. A page that is not yet in memory looks exactly like an illegal address until the OS checks.
- The first access to each page causes a **page fault**, and the OS loads the page.
- **Pure demand paging** starts a process with **zero** pages in memory.
- **Prepaging** loads some pages in advance (for example the working set of a process being swapped back in), betting that they will be used.

Hardware needed: a valid-invalid bit in each PTE, a backing store (**swap space**), and the ability to **restart an instruction** after a fault.

### 10.7 Handling a page fault, step by step

```
 1. Process accesses page → PTE says "invalid" → MMU raises a PAGE FAULT trap
 2. OS checks its records (e.g. the process's VMAs on Linux):
      illegal address?  → kill the process (SIGSEGV: "Segmentation fault")
      legal, just not loaded → continue
 3. Find a FREE FRAME (if none: run page replacement, Chapter 11;
      if the victim is DIRTY, write it to disk first)
 4. Schedule a disk read of the page into that frame
      (the process blocks; the CPU runs other processes)
 5. Disk read completes → update the PTE: frame number, valid = 1
 6. RESTART the instruction that caused the fault; it now succeeds
```

**Kinds of faults on Linux:**

| Fault | Meaning | Cost |
|---|---|---|
| **Minor (soft) fault** | Page already in RAM, e.g. in the page cache or shared with another process. Only the PTE needs updating | Microseconds |
| **Major (hard) fault** | Page must be read from disk or flash | Milliseconds (HDD), tens to hundreds of µs (SSD) |
| **Invalid fault** | Address not mapped, or permission violated | Process gets `SIGSEGV` (or `SIGBUS`) |

You can count them with `/usr/bin/time -v ./program` ("Major/Minor page faults").

### 10.8 Effective access time with page faults

```
EAT = (1 − p) × t_mem + p × t_fault          p = page-fault rate (0 ≤ p ≤ 1)
```

**Classic example.** Memory access 200 ns, page-fault service time 8 ms = 8,000,000 ns:

- EAT = (1 − p) × 200 + p × 8,000,000 = 200 + 7,999,800 × p ns
- With **one fault per 1,000 accesses** (p = 0.001): EAT ≈ 200 + 8,000 = **8.2 µs**, which is **40 times slower**.
- For less than **10% slowdown** (EAT < 220 ns): 7,999,800 × p < 20 → **p < 0.0000025**, i.e. fewer than **1 fault per 400,000 accesses**.

**Lesson:** the page-fault rate must be **extremely** low. This is why page replacement and locality matter so much.

### 10.9 Copy-on-write (COW)

`fork()` creates a child that is a copy of the parent. Copying every page would be slow and usually wasted, because the child often calls `exec()` right away and throws the copy away.

**Copy-on-write:**

1. After `fork`, parent and child **share all pages**, and those pages are marked **read-only** in both page tables.
2. When either process **writes** a shared page, the write causes a protection fault.
3. The kernel then **copies only that page**, gives the writer its own private copy (now writable), and restarts the write.
4. Pages that are never written are **never copied**.

```
 After fork():                         After child writes page B:
 Parent PT ──► [A][B][C] (read-only)   Parent PT ──► [A][B ][C]
 Child  PT ──┘   shared                Child  PT ──► [A][B'][C]   (only B copied)
```

**Write it yourself: prove that virtual addresses are per-process**

```c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int value = 100;
    pid_t pid = fork();                  /* child shares parent's pages copy-on-write */

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {                      /* child */
        value = 200;                     /* first write: the kernel copies just this page */
        printf("child : value=%d at %p\n", value, (void *)&value);
        exit(0);
    }
    wait(NULL);
    printf("parent: value=%d at %p\n", value, (void *)&value);
    return 0;
}
```

Both lines print the **same address** but **different values**: the same virtual address maps to two different physical frames once COW has copied the page.

- **`vfork()`** is an older, faster variant. The child **borrows** the parent's address space (no copy at all) and the parent is suspended until the child calls `exec` or `_exit`. It is risky and mostly replaced by COW `fork` and `posix_spawn`.
- **Zero-fill-on-demand**: new heap and stack pages are all mapped to one shared **zero page** (read-only). A real page is allocated and zeroed only on the first write.

### 10.10 Memory-mapped files

`mmap()` maps a file into the address space. After that, **reading memory reads the file** (via page faults and the page cache) and writing memory writes the file. No `read()`/`write()` calls and no extra copy into a user buffer.

```c
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "/etc/hostname";

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0 || st.st_size == 0) {
        close(fd);
        return 1;
    }

    /* Map the whole file. Pages are loaded lazily on first touch (demand paging). */
    const char *data = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);                                   /* the mapping stays valid after close */
    if (data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    size_t lines = 0;
    for (off_t i = 0; i < st.st_size; i++)
        if (data[i] == '\n')
            lines++;
    printf("%s: %lld bytes, %zu lines\n", path, (long long)st.st_size, lines);

    munmap((void *)data, (size_t)st.st_size);
    return 0;
}
```

| Flag | Meaning |
|---|---|
| `MAP_SHARED` | Writes go to the file and are visible to every other process mapping it |
| `MAP_PRIVATE` | Writes are **copy-on-write** private copies; the file does not change |
| `MAP_ANONYMOUS` | No file; zero-filled memory. This is how `malloc` gets big blocks |
| `MAP_FIXED` | Use exactly this address (dangerous: replaces existing mappings) |

**Embedded Linux use:** `mmap` on `/dev/mem` or a UIO device maps **hardware registers** into a user-space program. This gives quick prototyping without a kernel driver.

### 10.11 Shared memory between processes

The fastest IPC: two processes map the **same physical frames**, so data needs no copying at all. Synchronisation is your job (semaphores, mutexes in shared memory).

```c
/* Process A (creator). Link with -lrt on older glibc. */
int fd = shm_open("/sensor_data", O_CREAT | O_RDWR, 0600);
ftruncate(fd, 4096);
struct shared *s = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
s->temperature = 25;                    /* visible to process B immediately */

/* Process B: same name, without O_CREAT, then mmap with MAP_SHARED. */
/* Clean-up: munmap(), close(), and shm_unlink("/sensor_data") once nobody needs it. */
```

### 10.12 Kernel space vs user space

- The virtual address space is split into a **user part** and a **kernel part**. The kernel part is mapped into **every** process but accessible only in **privileged mode** (the U/S bit in the PTEs).
- 32-bit Linux classically used **3 GB user / 1 GB kernel**. 64-bit Linux uses the lower and upper halves of the 48-bit space.
- System calls switch the CPU to kernel mode, so the kernel can reach both its own memory and the caller's (through `copy_from_user`, Chapter 20).
- **KPTI** (Kernel Page-Table Isolation): after the Meltdown attack (2018), x86 Linux unmaps most of the kernel while running user code, at some performance cost.

### 10.13 Security features built on paging

| Feature | What it does |
|---|---|
| **NX / XN bit** (DEP) | Pages that are writable are not executable (W^X), so injected shellcode cannot run |
| **ASLR** | Randomises the positions of the stack, heap, libraries and (with PIE) the executable, so attackers cannot predict addresses |
| **Guard pages** | Unmapped pages below each stack: overflow → immediate fault instead of silent corruption |
| **Page 0 unmapped** | `NULL` dereference → `SIGSEGV` (`vm.mmap_min_addr`) |
| **Stack canaries** (compiler, `-fstack-protector`) | A random value before the return address is checked before `ret`; a smashed stack is detected |

---

## 11. Page Replacement Algorithms

### 11.1 Why replacement is needed

With demand paging, RAM eventually fills up. When a page fault occurs and **no frame is free**, the OS must choose a **victim** page to evict:

1. Find the faulting page on disk.
2. Find a free frame. If there is none, run the **page replacement algorithm** to pick a victim.
3. If the victim is **dirty** (modified), write it back to disk first. A **clean** page can simply be dropped. The dirty bit halves the I/O for clean pages.
4. Read the new page into the frame and update both page table entries.
5. Restart the faulting instruction.

**Goal:** the **lowest page-fault rate**. Algorithms are compared by running them on a **reference string** (the sequence of page numbers accessed) with a given number of frames and counting the faults.

The same algorithms are used wherever a small fast store caches a large slow one: CPU caches, TLBs, database buffer pools, web caches, flash translation layers and the LRU/LFU coding questions in Chapter 13.

### 11.2 FIFO (First In, First Out)

**Rule:** replace the page that has been in memory the **longest**, regardless of how often or how recently it was used. Implemented with a queue (or a circular pointer).

**Trace:** reference string `1 2 3 4 1 2 5 1 2 3 4 5`, **3 frames**:

| Ref | 1 | 2 | 3 | 4 | 1 | 2 | 5 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Frame 1 | 1 | 1 | 1 | **4** | 4 | 4 | **5** | 5 | 5 | 5 | 5 | 5 |
| Frame 2 | | 2 | 2 | 2 | **1** | 1 | 1 | 1 | 1 | **3** | 3 | 3 |
| Frame 3 | | | 3 | 3 | 3 | **2** | 2 | 2 | 2 | 2 | **4** | 4 |
| Fault? | F | F | F | F | F | F | F | – | – | F | F | – |

**9 page faults.**

Pros: trivial to implement. Cons: it evicts heavily used pages just because they are **old** (a page holding a loop or a global variable). It also suffers from Belady's anomaly.

### 11.3 Belady's anomaly

**Common sense says more frames means fewer faults.** For FIFO this is **not always true**. Same string, **4 frames**:

| Ref | 1 | 2 | 3 | 4 | 1 | 2 | 5 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Frame 1 | 1 | 1 | 1 | 1 | 1 | 1 | **5** | 5 | 5 | 5 | **4** | 4 |
| Frame 2 | | 2 | 2 | 2 | 2 | 2 | 2 | **1** | 1 | 1 | 1 | **5** |
| Frame 3 | | | 3 | 3 | 3 | 3 | 3 | 3 | **2** | 2 | 2 | 2 |
| Frame 4 | | | | 4 | 4 | 4 | 4 | 4 | 4 | **3** | 3 | 3 |
| Fault? | F | F | F | F | – | – | F | F | F | F | F | F |

**10 page faults with 4 frames vs 9 with 3 frames.** That is **Belady's anomaly**: for some algorithms, the fault rate can **increase** when frames are added.

**Stack algorithms never show it.** An algorithm is a *stack algorithm* if the set of pages in memory with *n* frames is always a **subset** of the set with *n + 1* frames. **LRU and OPT are stack algorithms. FIFO and Clock are not.**

### 11.4 Optimal (OPT / MIN / Belady's algorithm)

**Rule:** replace the page that **will not be used for the longest time in the future**.

| Ref | 1 | 2 | 3 | 4 | 1 | 2 | 5 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Frame 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | **3** | **4** | 4 |
| Frame 2 | | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 2 |
| Frame 3 | | | 3 | **4** | 4 | 4 | **5** | 5 | 5 | 5 | 5 | 5 |
| Fault? | F | F | F | F | – | – | F | – | – | F | F | – |

**7 page faults**, the minimum possible for 3 frames. At reference `4`, the future uses are 1 (next at position 5), 2 (position 6) and 3 (position 10), so 3 is evicted because it is needed last.

- **Provably optimal**, but **impossible to implement**, because the OS cannot know the future.
- Used as a **benchmark**: "LRU is within 10% of optimal on this workload".

### 11.5 LRU (Least Recently Used)

**Rule:** replace the page that has **not been used for the longest time**. It uses the recent past as a prediction of the near future (temporal locality).

| Ref | 1 | 2 | 3 | 4 | 1 | 2 | 5 | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Frame 1 | 1 | 1 | 1 | **4** | 4 | 4 | **5** | 5 | 5 | **3** | 3 | 3 |
| Frame 2 | | 2 | 2 | 2 | **1** | 1 | 1 | 1 | 1 | 1 | **4** | 4 |
| Frame 3 | | | 3 | 3 | 3 | **2** | 2 | 2 | 2 | 2 | 2 | **5** |
| Fault? | F | F | F | F | F | F | F | – | – | F | F | F |

**10 page faults** with 3 frames and **8** with 4 frames. More frames never hurt LRU.

This particular string is unkind to LRU. On the classic textbook string below, LRU (12) beats FIFO (15) clearly.

**Implementing exact LRU:**

| Method | How | Cost |
|---|---|---|
| **Counters / timestamps** | Each PTE gets a "time of last use" field, written on **every** memory access. Evict the smallest | A write on every access, and a search on every fault |
| **Stack (doubly linked list)** | On every access, move the page to the top. The bottom is the LRU page | Several pointer updates on every access, but no search |

Both need **hardware help on every memory reference**, which is far too expensive for real page tables. Real OSes therefore use **LRU approximations** based on the hardware **reference bit**. In software caches, exact LRU **is** practical: hash map + doubly linked list (Chapter 13).

### 11.6 LRU approximation algorithms

The hardware sets the **reference (accessed) bit** in a PTE whenever the page is used. The OS can read and clear it.

**1. Additional-reference-bits (aging).** Each page keeps an 8-bit history byte. At every timer interrupt, the OS shifts the byte right by one and copies the reference bit into the top bit, then clears the reference bit.

```
history = (history >> 1) | (R << 7);   R = 0;
```

The history byte, read as an unsigned number, ranks the pages: **the smallest value is the least recently used**. A page with `11000100` (196) was used more recently than one with `01110111` (119), so the second one is evicted. `00000000` means unused for 8 periods.

**2. Second chance (the Clock algorithm).** FIFO, but a page with its reference bit set gets a **second chance**:

```
             ┌──► page A (R=1) ──┐
             │                   ▼
       page F (R=0)          page B (R=0)  ◄── victim if the hand reaches it with R=0
             ▲                   │
             │                   ▼
       page E (R=1)          page C (R=1)
             ▲                   │
             └──── page D (R=0) ◄┘

 The frames form a circle. The "hand" points at the oldest page.
 On a page fault:
   while (page at hand has R == 1) { R = 0; advance hand; }   // second chance
   replace the page at hand; set R = 1 for the new page; advance hand
```

- If every page has R = 1, the hand goes all the way round, clearing every bit, and the algorithm **degenerates to FIFO**.
- Cheap: no work on hits, except the hardware setting R.

**3. Enhanced second chance (R, M pairs).** Consider the **modify bit** too, and prefer evicting pages that need no disk write:

| Class (R, M) | Meaning | Eviction preference |
|---|---|---|
| (0, 0) | Not recently used, clean | **Best** victim |
| (0, 1) | Not recently used, dirty (must be written out first) | Second |
| (1, 0) | Recently used, clean | Third |
| (1, 1) | Recently used and dirty | **Worst** victim |

The clock hand may sweep several times to find a page in the lowest non-empty class. Tanenbaum's **NRU (Not Recently Used)** picks a random page from the lowest non-empty class. Classic Mac OS used enhanced second chance.

### 11.7 Counting-based algorithms

- **LFU (Least Frequently Used):** evict the page with the **smallest use count**. Problem: a page used heavily at start-up keeps a high count forever. Fix: decay the counts (shift right periodically).
- **MFU (Most Frequently Used):** evict the page with the **largest** count, on the theory that a page with a small count was just brought in and is about to be used.

Neither approximates OPT well, and both are expensive, so they are rarely used for page replacement. **LFU is very common for software caches** (Chapter 13).

### 11.8 Page buffering

OSes keep a **pool of free frames**. On a fault, the new page is read into a free frame **immediately**, while the victim is written out afterwards and its frame then joins the pool. The OS also remembers **which page** was in each free frame. If that page is needed again before the frame is reused, it is recovered **without any disk I/O**. Linux's page cache and "reclaim" work this way.

### 11.9 Results on the classic textbook string

Reference string (Silberschatz): `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1`

| Algorithm | 3 frames | 4 frames |
|---|---|---|
| FIFO | **15** | 10 |
| LRU | **12** | 8 |
| OPT | **9** | 8 |
| Clock (second chance) | 14 | 9 |

And for `1 2 3 4 1 2 5 1 2 3 4 5`:

| Algorithm | 3 frames | 4 frames |
|---|---|---|
| FIFO | 9 | **10** (Belady's anomaly) |
| LRU | 10 | 8 |
| OPT | 7 | 6 |
| Clock | 9 | **10** (Belady's anomaly too) |

**Write it yourself: a page replacement simulator (FIFO, LRU, OPT, Clock)**

```c
#include <stdio.h>

#define MAXF 16                                  /* maximum frames for this demo */

static int find(const int *frames, int nf, int page)
{
    for (int i = 0; i < nf; i++)
        if (frames[i] == page)
            return i;
    return -1;
}

int fifo_faults(const int *refs, int n, int nf)
{
    int frames[MAXF], next = 0, faults = 0;
    for (int i = 0; i < nf; i++) frames[i] = -1;

    for (int r = 0; r < n; r++) {
        if (find(frames, nf, refs[r]) >= 0)
            continue;                            /* hit */
        frames[next] = refs[r];                  /* replace the oldest (circular queue) */
        next = (next + 1) % nf;
        faults++;
    }
    return faults;
}

int lru_faults(const int *refs, int n, int nf)
{
    int frames[MAXF], last[MAXF], faults = 0;    /* last[i] = time of last use */
    for (int i = 0; i < nf; i++) { frames[i] = -1; last[i] = -1; }

    for (int r = 0; r < n; r++) {
        int idx = find(frames, nf, refs[r]);
        if (idx < 0) {                           /* fault: evict smallest last-use time */
            idx = 0;                             /* (empty frames have -1, so they go first) */
            for (int i = 1; i < nf; i++)
                if (last[i] < last[idx]) idx = i;
            frames[idx] = refs[r];
            faults++;
        }
        last[idx] = r;                           /* record this use */
    }
    return faults;
}

int opt_faults(const int *refs, int n, int nf)
{
    int frames[MAXF], faults = 0;
    for (int i = 0; i < nf; i++) frames[i] = -1;

    for (int r = 0; r < n; r++) {
        if (find(frames, nf, refs[r]) >= 0)
            continue;
        faults++;
        int victim = 0, farthest = -1;
        for (int i = 0; i < nf; i++) {
            if (frames[i] == -1) { victim = i; break; }   /* free frame */
            int next = n;                                 /* n means "never used again" */
            for (int k = r + 1; k < n; k++)
                if (refs[k] == frames[i]) { next = k; break; }
            if (next > farthest) { farthest = next; victim = i; }
        }
        frames[victim] = refs[r];
    }
    return faults;
}

int clock_faults(const int *refs, int n, int nf)
{
    int frames[MAXF], refbit[MAXF], hand = 0, faults = 0;
    for (int i = 0; i < nf; i++) { frames[i] = -1; refbit[i] = 0; }

    for (int r = 0; r < n; r++) {
        int idx = find(frames, nf, refs[r]);
        if (idx >= 0) { refbit[idx] = 1; continue; }      /* hit: hardware sets R */

        faults++;
        while (refbit[hand]) {                           /* second chance */
            refbit[hand] = 0;
            hand = (hand + 1) % nf;
        }
        frames[hand] = refs[r];
        refbit[hand] = 1;
        hand = (hand + 1) % nf;
    }
    return faults;
}

int main(void)
{
    const int refs[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    const int n = (int)(sizeof refs / sizeof refs[0]);

    for (int f = 3; f <= 4; f++)
        printf("%d frames: FIFO=%d LRU=%d OPT=%d CLOCK=%d\n", f,
               fifo_faults(refs, n, f), lru_faults(refs, n, f),
               opt_faults(refs, n, f), clock_faults(refs, n, f));
    return 0;
}
```

### 11.10 Allocating frames to processes

**Minimum frames.** Each process needs at least as many frames as **one instruction can touch** (instruction + operands + indirect pointers), otherwise it can never complete an instruction.

**Equal allocation.** With *m* frames and *n* processes, each gets *m / n*.

**Proportional allocation.** Allocate by size:

```
a_i = (s_i / S) × m        s_i = size of process i,  S = Σ s_i,  m = total frames
```

**Example:** 62 frames, process P1 = 10 pages, P2 = 127 pages, S = 137:

- P1 gets 10/137 × 62 ≈ **4** frames
- P2 gets 127/137 × 62 ≈ **57** frames

Priorities can be mixed in, giving more frames to high-priority processes.

**Global vs local replacement:**

| | Global replacement | Local replacement |
|---|---|---|
| Victim chosen from | **All** frames in the system | Only the faulting **process's own** frames |
| Pros | Higher throughput, flexible | A process's performance does not depend on other processes |
| Cons | One process can steal frames and slow the others | Unused frames of one process cannot help another |
| Used by | Most general-purpose OSes (Linux) | Real-time and isolation-focused systems |

### 11.11 Thrashing

**Thrashing** happens when a process (or the whole system) spends **more time paging than executing**. Each process has fewer frames than it actively needs, so it faults, evicts a page that it needs almost immediately, faults again, and so on.

**The vicious circle:**

1. Processes fault a lot and wait for the disk, so **CPU utilisation drops**.
2. The scheduler sees idle CPU and **admits more processes** (raises the degree of multiprogramming).
3. Each process now gets even fewer frames, so faults increase further.
4. CPU utilisation collapses while the disk stays 100% busy.

```
 CPU
 utilisation
   ▲
   │          ●●●●
   │       ●●      ●●
   │     ●            ●      ← thrashing begins
   │   ●                ●
   │  ●                   ●●
   │ ●                       ●●●●
   └─────────────────────────────────► degree of multiprogramming
```

**Why it happens: the locality model.** A process moves from one **locality** (a set of pages used together, such as a function and its data) to another. If it gets fewer frames than its current locality, it thrashes.

**Fixes:**

**1. Working-set model.** Pick a window Δ (e.g. the last 10,000 references). The **working set** WS(t, Δ) is the set of distinct pages referenced in the last Δ references.

```
 Δ = 5
 references:  ... 1 2 1 3 2 | 4 4 4 5 4
                          ▲ t1        ▲ t2
 WS(t1) = {1, 2, 3}  (size 3)      WS(t2) = {4, 5}  (size 2)
```

- Total demand D = Σ WSSᵢ (sum of all working-set sizes).
- If **D > m** (total frames), thrashing is coming, so **suspend** (swap out) a process and give its frames to the others.
- If D is well below m, a new process can be admitted.
- Tracking exact working sets is expensive. It is approximated with the reference bits and a timer interrupt.

**2. Page-fault frequency (PFF).** A more direct method: measure each process's fault rate.

- Rate **above** an upper bound: the process needs more frames, so give it one (or suspend a process if none are free).
- Rate **below** a lower bound: it has too many, so take one away.

**3. In practice:** add RAM, run fewer programs, fix the program's locality. Linux also has the **OOM killer** as a last resort.

---

## 12. Cache Memory

### 12.1 Why caches exist

A modern CPU core can execute several instructions per nanosecond, but a DRAM access takes 60-100 ns. Without help, the CPU would spend most of its time waiting. A **cache** is a small, fast SRAM memory between the CPU and main memory that keeps **copies of recently used memory**. Thanks to **locality** (Chapter 1.2), most accesses find their data in the cache.

Even microcontrollers have caches: the Cortex-M7 has L1 I-cache and D-cache, the STM32F4 has the **ART accelerator** (a small flash cache with prefetch), and the ESP32 caches its external flash and PSRAM.

### 12.2 Basic terms

| Term | Meaning |
|---|---|
| **Cache line (block)** | The unit of transfer between cache and memory: typically **32 bytes** (Cortex-M7) or **64 bytes** (x86, Cortex-A). Reading 1 byte loads the whole line |
| **Hit** | The data is in the cache |
| **Miss** | It is not, so the line must be fetched from the next level |
| **Hit time** | Time to access the cache on a hit (about 1-4 cycles for L1) |
| **Miss penalty** | Extra time to fetch the line from the next level |
| **Hit ratio** *h* | hits / total accesses. Miss ratio = 1 − *h* |

**Average memory access time (AMAT):**

```
AMAT = hit_time + miss_rate × miss_penalty
```

**Example:** hit time 1 cycle, miss rate 5%, miss penalty 100 cycles → AMAT = 1 + 0.05 × 100 = **6 cycles**.

**Two levels:**

```
AMAT = HT_L1 + MR_L1 × (HT_L2 + MR_L2 × MP_L2)
```

**Example:** L1 hit 1 cycle, L1 miss rate 5%, L2 hit 10 cycles, L2 local miss rate 20%, memory 100 cycles → AMAT = 1 + 0.05 × (10 + 0.2 × 100) = 1 + 0.05 × 30 = **2.5 cycles**.

### 12.3 Cache mapping: where can a memory block go?

The cache has far fewer lines than memory has blocks, so each memory block must be **mapped** to a cache position. Every address is split into three fields:

```
 ┌───────────────────┬──────────────────┬──────────────────┐
 │       TAG         │      INDEX       │   BLOCK OFFSET   │
 └───────────────────┴──────────────────┴──────────────────┘
   identifies which     selects the set    selects the byte
   memory block is      (or line) to look   inside the line
   stored there         in
```

- **Offset bits** = log₂(line size)
- **Index bits** = log₂(number of sets)
- **Tag bits** = address bits − index bits − offset bits

Each line also stores a **valid** bit and, for write-back caches, a **dirty** bit.

#### Direct-mapped cache

Each memory block can go into **exactly one** line: `line = block_number mod number_of_lines`.

```
 Memory blocks:  0  1  2  3  4  5  6  7  8  9 10 11 ...
 Cache (4 lines):
   line 0 ◄── blocks 0, 4, 8, 12 ...
   line 1 ◄── blocks 1, 5, 9, 13 ...
   line 2 ◄── blocks 2, 6, 10, 14 ...
   line 3 ◄── blocks 3, 7, 11, 15 ...
```

- **Pros:** simplest and fastest. Only one tag comparison.
- **Cons:** **conflict misses**. If a program alternates between blocks 0 and 4, they keep evicting each other even if the rest of the cache is empty ("cache thrashing").

#### Fully associative cache

A block can go into **any** line. All tags are compared **in parallel**. There is no index field.

- **Pros:** no conflict misses.
- **Cons:** expensive hardware (one comparator per line) and slower. Practical only for small structures such as **TLBs**.

#### N-way set-associative cache

The compromise used by almost all real caches. Lines are grouped into **sets** of *N* lines. A block maps to **one set** (`set = block_number mod number_of_sets`) but can occupy **any of the N lines** (ways) in that set.

```
 2-way set associative, 4 sets (8 lines):
   set 0: [way 0][way 1]  ◄── blocks 0, 4, 8, 12 ... (any 2 of them at once)
   set 1: [way 0][way 1]  ◄── blocks 1, 5, 9 ...
   set 2: [way 0][way 1]
   set 3: [way 0][way 1]
```

- Direct-mapped is "1-way". Fully associative is "N-way with one set".
- Typical: L1 = 4- to 8-way, L2/L3 = 8- to 16-way. The Cortex-M7 D-cache is 4-way, its I-cache 2-way.

**Quick check:** memory block 12, cache of 8 lines:

- Direct-mapped: line 12 mod 8 = **4**
- 2-way (4 sets): set 12 mod 4 = **0**, either way
- Fully associative: **any** line

#### Worked example: tag, index and offset bits

**32-bit addresses, 32 KB cache, 64-byte lines, 4-way set associative.**

1. Lines = 32 KB / 64 B = 512
2. Sets = 512 / 4 = 128
3. Offset = log₂ 64 = **6 bits**
4. Index = log₂ 128 = **7 bits**
5. Tag = 32 − 7 − 6 = **19 bits**

The same cache **direct-mapped**: 512 sets → index 9 bits, tag 32 − 9 − 6 = **17 bits**.
**Fully associative**: index 0 bits, tag 32 − 6 = **26 bits**.

Doubling the associativity removes one index bit and adds one tag bit.

**Total storage** (direct-mapped variant): each line stores 64 B of data + 17 tag bits + 1 valid bit (+ 1 dirty bit for write-back), so the real SRAM is somewhat larger than the "32 KB" data capacity.

**Decoding an address** (same 4-way cache): address `0x12345678`:

- Offset = `0x678 & 0x3F` = `0x38`
- Index = `(0x12345678 >> 6) & 0x7F` = `0x59`
- Tag = `0x12345678 >> 13` = `0x91A2`

### 12.4 Comparing the mappings

| | Direct-mapped | N-way set associative | Fully associative |
|---|---|---|---|
| Places a block can go | 1 | N | All |
| Tag comparators | 1 | N | One per line |
| Hit time | Fastest | Medium | Slowest |
| Conflict misses | Most | Few | None |
| Replacement policy needed? | No (no choice) | Yes, within the set | Yes, over the whole cache |
| Typical use | Simple MCU caches | L1/L2/L3 caches | TLBs, small buffers |

### 12.5 Why misses happen: the 3 (or 4) C's

| Miss type | Cause | Reduced by |
|---|---|---|
| **Compulsory** (cold) | First access to a block: it was never in the cache | Larger lines, **prefetching** |
| **Capacity** | The working set is bigger than the cache | Bigger cache, better locality (blocking/tiling loops) |
| **Conflict** | Too many blocks map to the same set | Higher associativity, changing array sizes or alignment |
| **Coherence** (4th C, multi-core) | Another core wrote the line and invalidated our copy | Avoid sharing, avoid **false sharing** |

### 12.6 Replacement policies (within a set)

| Policy | Idea | Notes |
|---|---|---|
| **LRU** | Evict the least recently used way | Exact LRU is cheap for 2-way (one bit per set), expensive for 8+ ways |
| **Pseudo-LRU (tree PLRU)** | A binary tree of bits points away from recently used ways | N − 1 bits per set; used by most real CPUs |
| **FIFO / round-robin** | Evict in rotation | Simple; common in small MCU caches |
| **Random** | Pick any way | Surprisingly close to LRU for large caches; no state to keep |

### 12.7 Write policies

**When the CPU writes, when does memory get updated?**

| | Write-through | Write-back |
|---|---|---|
| On a write hit | Update the cache **and** memory | Update **only the cache** and set the line's **dirty bit** |
| Memory updated | Immediately | When the dirty line is **evicted** |
| Memory traffic | High (every write) | Low (one write-back per line) |
| Consistency | Memory always up to date | Memory can be **stale**: dangerous with DMA |
| Typical use | Simple systems, some L1s | Most modern caches |

A **write buffer** lets write-through caches avoid stalling: writes are queued and drained to memory in the background.

**On a write miss, do we load the line first?**

| | Write-allocate (fetch on write) | No-write-allocate (write-around) |
|---|---|---|
| On a write miss | Load the line into the cache, then write it | Write straight to memory, leave the cache alone |
| Usually paired with | Write-back | Write-through |

### 12.8 Multi-level caches

```
 Core 0                     Core 1
 ┌─────────┬─────────┐      ┌─────────┬─────────┐
 │ L1 I$   │ L1 D$   │      │ L1 I$   │ L1 D$   │   ~32-64 KB each, private, split I/D
 └────┬────┴────┬────┘      └────┬────┴────┬────┘
      └────┬────┘                └────┬────┘
      ┌────┴────┐                ┌────┴────┐
      │   L2    │                │   L2    │          256 KB-2 MB, private, unified
      └────┬────┘                └────┬────┘
           └───────────┬──────────────┘
                 ┌─────┴─────┐
                 │    L3     │                        shared by all cores, MBs
                 └─────┬─────┘
                       │
                     DRAM
```

- **Split L1** (separate instruction and data caches): the CPU can fetch an instruction and access data in the same cycle (a Harvard arrangement at L1).
- **Inclusive** caches keep everything from L1 in L2 as well, which simplifies coherence. **Exclusive** caches keep a line in only one level, which gives more total capacity.

### 12.9 Cache coherence in multi-core systems

Each core has a private cache. If core 0 and core 1 both cache variable `x`, and core 0 writes it, core 1's copy becomes **stale**. **Cache coherence** protocols keep the copies consistent.

- **Snooping:** every cache watches ("snoops") a shared bus and reacts to other cores' reads and writes. Simple, but it does not scale beyond a moderate number of cores.
- **Directory-based:** a directory records which caches hold each line, and point-to-point messages go only to those caches. This scales to many cores.

**The MESI protocol** gives every cache line one of four states:

| State | Meaning | Matches memory? | Other caches have it? |
|---|---|---|---|
| **M**odified | Only this cache has it, and it has been **changed** | No (dirty) | No |
| **E**xclusive | Only this cache has it, unchanged | Yes | No |
| **S**hared | Possibly in several caches, unchanged | Yes | Maybe |
| **I**nvalid | The line holds no valid data | — | — |

**Key transitions:**

- **Read miss:** if another cache has the line, it becomes **S** everywhere (an M holder writes it back first). Otherwise it becomes **E**.
- **Write to S:** broadcast an **invalidate** so the other copies become I, then become **M**.
- **Write to E:** go to **M** silently, with no bus traffic. This is the advantage of having an E state.
- **Another core reads our M line:** supply or write back the data, and both copies become S.
- **Another core writes:** our copy becomes **I**.

Variants: **MOESI** (AMD, adds *Owned*: dirty but shared, so it need not be written back yet) and **MESIF** (Intel, adds *Forward*: one sharer answers requests).

### 12.10 False sharing

**False sharing** happens when two cores write **different variables that live in the same cache line**. The variables are independent, but coherence works on **whole lines**, so the line "ping-pongs" between the cores and each write invalidates the other core's copy. Performance can drop by 10× or more.

```c
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdalign.h>
#include <stdio.h>
#include <time.h>

#define ITERS 50000000UL

static struct { volatile unsigned long a, b; } same_line;              /* a and b share a line */
static struct { alignas(64) volatile unsigned long a;
                alignas(64) volatile unsigned long b; } own_lines;     /* each on its own line */

static void *worker(void *arg)
{
    volatile unsigned long *counter = arg;
    for (unsigned long i = 0; i < ITERS; i++)
        (*counter)++;
    return NULL;
}

static double run(volatile unsigned long *x, volatile unsigned long *y)
{
    struct timespec t0, t1;
    pthread_t ta, tb;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_create(&ta, NULL, worker, (void *)x);
    pthread_create(&tb, NULL, worker, (void *)y);
    pthread_join(ta, NULL);
    pthread_join(tb, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    return (double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;
}

int main(void)
{
    printf("same cache line (false sharing): %.3f s\n", run(&same_line.a, &same_line.b));
    printf("separate cache lines           : %.3f s\n", run(&own_lines.a, &own_lines.b));
    return 0;
}
```

Run it on a multi-core machine. The slowdown appears only when the two threads really run **at the same time on different cores**, and its size depends heavily on the CPU (from a few percent to more than 10×).

**Fixes:** align or pad per-thread or per-core data to the cache line size (`alignas(64)`, Linux's `____cacheline_aligned`), keep per-thread counters in thread-local storage and combine them at the end, or group data by the core that writes it.

### 12.11 Writing cache-friendly code

C stores 2D arrays in **row-major** order: `m[r][0], m[r][1], ...` are adjacent in memory. Walking along a row uses every byte of each loaded cache line. Walking down a column touches a **new line on every access**.

```c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>

#define N 4096
static int m[N][N];                                 /* 64 MB: far bigger than any cache */

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

int main(void)
{
    for (int r = 0; r < N; r++)                     /* fill so the reads cannot be optimised away */
        for (int c = 0; c < N; c++)
            m[r][c] = r ^ c;

    long sum = 0;
    double t0 = now();
    for (int r = 0; r < N; r++)                     /* row by row: sequential addresses */
        for (int c = 0; c < N; c++)
            sum += m[r][c];
    double t1 = now();
    for (int c = 0; c < N; c++)                     /* column by column: 16 KB jumps */
        for (int r = 0; r < N; r++)
            sum += m[r][c];
    double t2 = now();

    printf("row-major %.3f s, column-major %.3f s (sum=%ld)\n", t1 - t0, t2 - t1, sum);
    return 0;
}
```

The column-major loop is typically **several times slower**, even though both loops do exactly the same arithmetic.

**Rules of thumb:**

- Access memory **sequentially**. Prefer arrays to linked lists (each list node can be a cache miss).
- Put fields that are used together **next to each other** (**hot/cold splitting**: move rarely used fields to a separate struct).
- **Blocking (tiling):** process big matrices in tiles that fit in the cache.
- Keep loops and hot data **small**. On MCUs, place critical code and data in **TCM** or IRAM.

### 12.12 Caches and DMA: the classic embedded bug

A **DMA controller** reads and writes **RAM directly** and does not look in the CPU's data cache. With a **write-back D-cache** (Cortex-M7, Cortex-A), two problems appear:

**Problem 1: transmit (CPU → DMA → peripheral).** The CPU fills the buffer, but the new data sits **dirty in the cache**. DMA reads **old data** from RAM.
**Fix: clean** (write back) the buffer's cache lines **before** starting the DMA.

**Problem 2: receive (peripheral → DMA → CPU).** DMA writes fresh data to RAM, but the CPU still has **old copies in its cache** and reads stale data.
**Fix: invalidate** the buffer's cache lines so the CPU re-reads RAM. Do it **before** starting the transfer (so no dirty line gets evicted on top of the DMA data) **and after** it completes (to drop lines that were speculatively refetched during the transfer).

```c
#include <stdint.h>
#include "stm32h7xx.h"                     /* any Cortex-M7 CMSIS device header */

#define CACHE_LINE 32u

/* Align to a cache line AND use a size that is a whole number of lines. Otherwise
   invalidating the buffer could also throw away a neighbouring variable's dirty data. */
static uint8_t tx_buf[512] __attribute__((aligned(CACHE_LINE)));
static uint8_t rx_buf[512] __attribute__((aligned(CACHE_LINE)));

void uart_dma_send(void)
{
    prepare_message(tx_buf, sizeof tx_buf);                           /* CPU writes go into the D-cache */
    SCB_CleanDCache_by_Addr((uint32_t *)tx_buf, sizeof tx_buf);       /* push dirty lines out to RAM    */
    start_dma_tx(tx_buf, sizeof tx_buf);                              /* DMA now reads correct data     */
}

void uart_dma_receive_start(void)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)rx_buf, sizeof rx_buf);  /* no dirty line may land on top  */
    start_dma_rx(rx_buf, sizeof rx_buf);
}

void uart_dma_rx_complete_isr(void)
{
    SCB_InvalidateDCache_by_Addr((uint32_t *)rx_buf, sizeof rx_buf);  /* drop stale/speculative lines   */
    process_message(rx_buf, sizeof rx_buf);                           /* CPU now reads fresh RAM data   */
}
```

**Alternative solutions:**

1. Configure the DMA buffer region as **non-cacheable** with the MPU (Chapter 14). This is simple and safe, but CPU access to that region becomes slower.
2. Put DMA buffers in **DTCM**, which is not cached (on STM32H7, check which DMA engines can reach DTCM).
3. Use a **write-through** policy for that region (it fixes TX but not RX).
4. On systems with **hardware cache coherency** (some Cortex-A with ACP/CCI, x86), DMA snoops the caches and none of this is needed.

**Vocabulary:**

| Operation | Meaning |
|---|---|
| **Clean** | Write dirty lines back to memory; keep them valid in the cache |
| **Invalidate** | Mark lines invalid (discard them) without writing back |
| **Clean + invalidate** | Write back, then discard. Often loosely called "flush" (an ambiguous word) |

### 12.13 Caches in microcontrollers

| MCU | What it has | Notes |
|---|---|---|
| Cortex-M0/M3/M4 | No CPU cache (the core has none) | Vendors add **flash accelerators**: STM32 ART (prefetch + small I-cache for flash wait states) |
| Cortex-M7 | L1 I-cache and D-cache (configurable size, often 16 KB + 16 KB), plus **TCM** | `SCB_EnableICache()`, `SCB_EnableDCache()`. DMA coherence needed (12.12) |
| Cortex-M33/M55/M85 | Optional caches (vendor-dependent) | |
| ESP32 family | **Cache for external flash and PSRAM** (instructions and read-only data executed or read in place) | Code that must run while flash is busy goes in **IRAM** (Chapter 16) |

**TCM (Tightly Coupled Memory):** ITCM/DTCM on Cortex-M7 is SRAM connected **directly** to the core with zero wait states. It is **not cached** and **always fast**, which makes it **deterministic**, ideal for ISRs, control loops and the stack. Caches are fast **on average**. TCM is fast **every time**.

---

## 13. Cache Replacement Coding Problems

These are among the most frequently asked coding questions. The interviewer wants **O(1)** `get` and `put`, clean pointer handling, and a clear explanation of the eviction rule.

| Policy | Evicts | Key data structures |
|---|---|---|
| **LRU** | The entry **unused for the longest time** | Hash map + **doubly linked list** ordered by recency |
| **LFU** | The entry **used the fewest times** (ties: the least recently used of those) | Hash map + **one list per frequency** + a `min_freq` variable |
| **Clock / second chance** | The first entry the "hand" finds **without** its reference bit set | Circular array + one bit per entry |

### 13.1 LRU cache: the idea

We need two things at once:

1. **Find a key in O(1)** → a **hash map** from key to node.
2. **Know the recency order, and move or remove any entry in O(1)** → a **doubly linked list**. The most recently used entry is at the **front** and the least recently used at the **back**.

```
 hash map                     doubly linked list (recency order)
 ┌─────┬──────┐
 │ key │ node │      HEAD ⇄ [k=3] ⇄ [k=1] ⇄ [k=7] ⇄ TAIL
 ├─────┼──────┤             ▲ MRU                ▲ LRU (evicted next)
 │  1  │  ●───┼──────────────────────┘
 │  3  │  ●───┼─────────────┘
 │  7  │  ●───┼───────────────────────────────────┘
 └─────┴──────┘
```

- **get(key):** look it up in the map. On a miss, return -1. On a hit, **unlink** the node and **move it to the front**, then return its value.
- **put(key, value):** if the key exists, update it and move it to the front. Otherwise, if the cache is full, **remove the node at the back** (from the list *and* the map), then insert the new node at the front.

**Why doubly linked?** To unlink a node in O(1) you need its **previous** node. A singly linked list would need an O(n) search.

**Why sentinel (dummy) head and tail nodes?** Insertions and removals then never meet a `NULL` neighbour, so the code has no special cases for an empty list or for the first and last node.

### 13.2 LRU cache in C

This version allocates **all nodes once** at creation and **recycles** the evicted node. There is no `malloc` during `get`/`put`, which makes it suitable for embedded use.

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int key, value;
    struct node *prev, *next;       /* recency list: head.next = most recently used */
    struct node *hnext;             /* chain inside one hash bucket                 */
} node_t;

typedef struct {
    int      capacity, size, nbuckets;
    node_t  *nodes;                 /* all nodes preallocated: no malloc after create */
    node_t **buckets;               /* hash table: key -> node                        */
    node_t   head, tail;            /* sentinels: no NULL checks in list code         */
} lru_t;

static unsigned bucket_of(const lru_t *c, int key)
{
    return (unsigned)key % (unsigned)c->nbuckets;
}

static void list_unlink(node_t *n)
{
    n->prev->next = n->next;
    n->next->prev = n->prev;
}

static void list_push_front(lru_t *c, node_t *n)
{
    n->prev = &c->head;
    n->next = c->head.next;
    c->head.next->prev = n;
    c->head.next = n;
}

static node_t *hash_find(const lru_t *c, int key)
{
    for (node_t *n = c->buckets[bucket_of(c, key)]; n != NULL; n = n->hnext)
        if (n->key == key)
            return n;
    return NULL;
}

static void hash_insert(lru_t *c, node_t *n)
{
    unsigned b = bucket_of(c, n->key);
    n->hnext = c->buckets[b];
    c->buckets[b] = n;
}

static void hash_remove(lru_t *c, node_t *n)
{
    node_t **pp = &c->buckets[bucket_of(c, n->key)];
    while (*pp != n)
        pp = &(*pp)->hnext;
    *pp = n->hnext;
}

lru_t *lru_create(int capacity)
{
    lru_t *c = calloc(1, sizeof *c);
    if (c == NULL)
        return NULL;
    c->capacity = capacity;
    c->nbuckets = 2 * capacity + 1;                /* load factor <= 0.5 keeps chains short */
    c->nodes    = calloc((size_t)capacity, sizeof *c->nodes);
    c->buckets  = calloc((size_t)c->nbuckets, sizeof *c->buckets);
    if (c->nodes == NULL || c->buckets == NULL) {
        free(c->nodes);
        free(c->buckets);
        free(c);
        return NULL;
    }
    c->head.next = &c->tail;
    c->tail.prev = &c->head;
    return c;
}

void lru_destroy(lru_t *c)
{
    if (c != NULL) {
        free(c->nodes);
        free(c->buckets);
        free(c);
    }
}

/* Returns 1 and stores the value on a hit, 0 on a miss. */
int lru_get(lru_t *c, int key, int *value)
{
    node_t *n = hash_find(c, key);
    if (n == NULL)
        return 0;
    list_unlink(n);                                /* move to front = most recently used */
    list_push_front(c, n);
    *value = n->value;
    return 1;
}

void lru_put(lru_t *c, int key, int value)
{
    if (c->capacity <= 0)
        return;

    node_t *n = hash_find(c, key);
    if (n != NULL) {                               /* existing key: update and mark MRU */
        n->value = value;
        list_unlink(n);
        list_push_front(c, n);
        return;
    }
    if (c->size < c->capacity) {
        n = &c->nodes[c->size++];                  /* still room: take an unused node   */
    } else {
        n = c->tail.prev;                          /* full: evict the LRU node, reuse it */
        list_unlink(n);
        hash_remove(c, n);
    }
    n->key = key;
    n->value = value;
    hash_insert(c, n);
    list_push_front(c, n);
}

static void show(lru_t *c, int key)
{
    int v;
    if (lru_get(c, key, &v))
        printf("get(%d) = %d\n", key, v);
    else
        printf("get(%d) = -1 (miss)\n", key);
}

int main(void)
{
    lru_t *c = lru_create(2);
    if (c == NULL)
        return 1;

    lru_put(c, 1, 1);
    lru_put(c, 2, 2);
    show(c, 1);          /* 1  -> key 1 becomes most recently used */
    lru_put(c, 3, 3);    /* full: evicts key 2 (least recently used) */
    show(c, 2);          /* -1 */
    lru_put(c, 4, 4);    /* evicts key 1 */
    show(c, 1);          /* -1 */
    show(c, 3);          /* 3  */
    show(c, 4);          /* 4  */

    lru_destroy(c);
    return 0;
}
```

### 13.3 LRU cache in C++

`std::list::splice` moves a node to the front in O(1) **without** invalidating iterators, which is exactly what we need.

```cpp
#include <iostream>
#include <list>
#include <unordered_map>
#include <utility>

class LRUCache {
public:
    explicit LRUCache(int capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = map_.find(key);
        if (it == map_.end()) return -1;
        order_.splice(order_.begin(), order_, it->second);   // move to front, O(1)
        return it->second->second;
    }

    void put(int key, int value) {
        if (cap_ <= 0) return;
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            order_.splice(order_.begin(), order_, it->second);
            return;
        }
        if (static_cast<int>(order_.size()) == cap_) {
            map_.erase(order_.back().first);                 // evict LRU from the map...
            order_.pop_back();                               // ...and from the list
        }
        order_.emplace_front(key, value);
        map_[key] = order_.begin();
    }

private:
    int cap_;
    std::list<std::pair<int, int>> order_;                   // front = MRU, back = LRU
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> map_;
};

int main() {
    LRUCache c(2);
    c.put(1, 1);
    c.put(2, 2);
    std::cout << c.get(1) << '\n';   // 1
    c.put(3, 3);                      // evicts 2
    std::cout << c.get(2) << '\n';   // -1
    c.put(4, 4);                      // evicts 1
    std::cout << c.get(1) << '\n';   // -1
    std::cout << c.get(3) << '\n';   // 3
    std::cout << c.get(4) << '\n';   // 4
}
```

### 13.4 LFU cache: the idea

**Evict the key with the lowest use count. On a tie, evict the least recently used among them.**

Keeping the entries sorted by frequency would cost O(log n). The O(1) trick:

- `nodes`: key → {value, freq, position in its frequency list}.
- `freq_lists`: freq → a list of keys with that frequency, **most recent at the front**.
- `min_freq`: the smallest frequency currently present.

```
 freq_lists:
   1 → [k9] ⇄ [k4]           ◄── min_freq = 1: the victim is the back of this list (k4)
   2 → [k1]
   5 → [k3] ⇄ [k7]
```

- **On every hit (`touch`):** remove the key from list `freq`, add it to the front of list `freq + 1`. If list `freq` became empty **and** it was `min_freq`, then `min_freq++`.
- **On inserting a new key:** evict if full (the **back** of `freq_lists[min_freq]`), insert with freq = 1, and set `min_freq = 1` (a new key always has the lowest frequency).

### 13.5 LFU cache in C++ (O(1))

```cpp
#include <iostream>
#include <list>
#include <unordered_map>

class LFUCache {
public:
    explicit LFUCache(int capacity) : cap_(capacity) {}

    int get(int key) {
        auto it = nodes_.find(key);
        if (it == nodes_.end()) return -1;
        touch(it->second);
        return it->second.value;
    }

    void put(int key, int value) {
        if (cap_ <= 0) return;
        auto it = nodes_.find(key);
        if (it != nodes_.end()) {                        // existing key: update + count a use
            it->second.value = value;
            touch(it->second);
            return;
        }
        if (static_cast<int>(nodes_.size()) == cap_) {   // evict
            auto &victims = freqLists_[minFreq_];         // least frequently used ...
            int victim = victims.back();                  // ... and least recent among them
            victims.pop_back();
            if (victims.empty()) freqLists_.erase(minFreq_);
            nodes_.erase(victim);
        }
        freqLists_[1].push_front(key);
        nodes_[key] = Node{value, 1, freqLists_[1].begin()};
        minFreq_ = 1;                                     // a new key always has the lowest freq
    }

private:
    struct Node {
        int value;
        int freq;
        std::list<int>::iterator pos;                    // where the key sits in freqLists_[freq]
    };

    void touch(Node &n) {
        int key = *n.pos;
        auto &oldList = freqLists_[n.freq];
        oldList.erase(n.pos);
        if (oldList.empty()) {
            freqLists_.erase(n.freq);
            if (minFreq_ == n.freq) minFreq_++;
        }
        n.freq++;
        freqLists_[n.freq].push_front(key);
        n.pos = freqLists_[n.freq].begin();
    }

    int cap_;
    int minFreq_ = 0;
    std::unordered_map<int, Node> nodes_;                // key  -> node
    std::unordered_map<int, std::list<int>> freqLists_;  // freq -> keys, front = most recent
};

int main() {
    LFUCache c(2);
    c.put(1, 1);
    c.put(2, 2);
    std::cout << c.get(1) << '\n';   // 1   (key1 freq=2)
    c.put(3, 3);                      // evicts key 2 (freq 1)
    std::cout << c.get(2) << '\n';   // -1
    std::cout << c.get(3) << '\n';   // 3   (key3 freq=2)
    c.put(4, 4);                      // tie at freq 2: evicts key 1 (less recent)
    std::cout << c.get(1) << '\n';   // -1
    std::cout << c.get(3) << '\n';   // 3
    std::cout << c.get(4) << '\n';   // 4
}
```

### 13.6 LFU cache in C for small embedded caches

On an MCU the cache often has just 4-32 entries. A plain array with a linear scan is then **simpler, smaller and just as fast** as hash maps. Interviewers accept it if you state the trade-off: O(n) per operation, but n is tiny and there are no allocations.

```c
#include <stdio.h>

#define LFU_CAP 2

typedef struct {
    int      key, value;
    unsigned freq;                /* number of uses                       */
    unsigned last_used;           /* logical time: breaks ties (LRU)      */
    int      valid;
} lfu_entry_t;

static lfu_entry_t cache[LFU_CAP];
static unsigned    tick;

int lfu_get(int key, int *value)
{
    for (int i = 0; i < LFU_CAP; i++) {
        if (cache[i].valid && cache[i].key == key) {
            cache[i].freq++;
            cache[i].last_used = ++tick;
            *value = cache[i].value;
            return 1;
        }
    }
    return 0;
}

void lfu_put(int key, int value)
{
    int victim = 0;

    for (int i = 0; i < LFU_CAP; i++) {
        if (cache[i].valid && cache[i].key == key) {       /* existing key: update */
            cache[i].value = value;
            cache[i].freq++;
            cache[i].last_used = ++tick;
            return;
        }
    }
    for (int i = 0; i < LFU_CAP; i++) {
        if (!cache[i].valid) {                              /* empty slot: no eviction */
            victim = i;
            break;
        }
        if (cache[i].freq < cache[victim].freq ||           /* least frequent ...      */
            (cache[i].freq == cache[victim].freq &&
             cache[i].last_used < cache[victim].last_used)) /* ... then least recent   */
            victim = i;
    }
    cache[victim] = (lfu_entry_t){ key, value, 1u, ++tick, 1 };
}

static void show(int key)
{
    int v;
    printf("get(%d) = %d\n", key, lfu_get(key, &v) ? v : -1);
}

int main(void)
{
    lfu_put(1, 1);
    lfu_put(2, 2);
    show(1);          /* 1  */
    lfu_put(3, 3);    /* evicts 2 */
    show(2);          /* -1 */
    show(3);          /* 3  */
    lfu_put(4, 4);    /* tie at freq 2: evicts 1 (less recent) */
    show(1);          /* -1 */
    show(3);          /* 3  */
    show(4);          /* 4  */
    return 0;
}
```

### 13.7 Second-chance (Clock) cache in C

Clock approximates LRU with **one bit per entry** and **no list manipulation on a hit**. That makes it cheaper than LRU, and it is easy to make lock-free or ISR-friendly. Here a newly inserted entry starts with `ref = 0`, so it must be used again to earn its second chance. (The page-replacement version in Chapter 11 sets the bit on load. Both conventions exist.)

```c
#include <stdbool.h>
#include <stdio.h>

#define CLOCK_CAP 3

typedef struct {
    int  key, value;
    bool valid;
    bool ref;                         /* "used since the hand last passed" */
} slot_t;

static slot_t slots[CLOCK_CAP];
static int    hand;                   /* next slot the hand will examine */

static int find_slot(int key)
{
    for (int i = 0; i < CLOCK_CAP; i++)
        if (slots[i].valid && slots[i].key == key)
            return i;
    return -1;
}

bool clock_get(int key, int *value)
{
    int i = find_slot(key);
    if (i < 0)
        return false;
    slots[i].ref = true;              /* hit: only set the bit */
    *value = slots[i].value;
    return true;
}

void clock_put(int key, int value)
{
    int i = find_slot(key);
    if (i >= 0) {
        slots[i].value = value;
        slots[i].ref = true;
        return;
    }
    while (slots[hand].valid && slots[hand].ref) {   /* give a second chance */
        slots[hand].ref = false;
        hand = (hand + 1) % CLOCK_CAP;
    }
    slots[hand] = (slot_t){ key, value, true, false };  /* empty slot or victim */
    hand = (hand + 1) % CLOCK_CAP;
}

static void show(int key)
{
    int v;
    if (clock_get(key, &v))
        printf("get(%d) = %d\n", key, v);
    else
        printf("get(%d) = miss\n", key);
}

int main(void)
{
    clock_put(1, 100);
    clock_put(2, 200);
    clock_put(3, 300);
    show(1);              /* hit: key 1 earns a second chance              */
    clock_put(4, 400);    /* hand skips key 1 (clears its bit), evicts 2   */
    show(2);              /* miss */
    show(1);              /* 100  */
    show(3);              /* 300  */
    show(4);              /* 400  */
    return 0;
}
```

### 13.8 Comparing the policies

| | LRU | LFU | Clock |
|---|---|---|---|
| Good for | Recency-driven workloads (most) | Stable "popular" items | Approximating LRU cheaply |
| Weakness | **Scan pollution**: one big sequential scan flushes the whole cache | **Stale popularity**: old hot items never leave; new items get evicted too soon | Only approximate; degenerates to FIFO when everything is referenced |
| Work on a hit | Move node to front (several pointer writes) | Move node between frequency lists | Set one bit |
| Memory per entry | 2 list pointers + hash | List pointers + frequency + hash | 1 bit (+ key/value) |
| Used in | Memcached, CPU caches (pseudo-LRU), Android `LruCache` | CDN caches, Redis (`allkeys-lfu`) | OS page replacement (Linux uses a two-list clock-like scheme) |

**Advanced policies worth naming:** **LRU-K** (uses the K-th most recent access), **2Q** (a probationary queue for new keys plus a main LRU, which resists scans), **ARC** (Adaptive Replacement Cache, balances recency and frequency automatically; used in ZFS), **W-TinyLFU** (a small LRU window plus a frequency sketch; used in Caffeine for Java).

**Follow-up questions to expect:**

- *"Make it thread-safe."* One mutex around `get`/`put` is the simple answer. Note that LRU's `get` **writes** (it moves the node), so even reads need the lock. Better options: lock striping (sharded caches), or Clock, whose hit path only sets a bit.
- *"Add a TTL (expiry time)."* Store an expiry timestamp per entry and treat expired entries as misses. Optionally run a periodic sweep.
- *"Cap the cache by memory, not entry count."* Track total bytes and evict in a loop until the new item fits.

---

## 14. MMU and MPU

### 14.1 The memory manager: the OS component

"Memory manager" is the part of an operating system responsible for memory. Its jobs:

| Responsibility | On Linux (with MMU) | On an RTOS (FreeRTOS, Zephyr) |
|---|---|---|
| Track which memory is used and free | Page frame allocator (buddy), `struct page` per frame | Heap implementation (`heap_4`, TLSF), memory pools |
| Allocate and free memory for programs | `brk`, `mmap`, page faults, `kmalloc`/`vmalloc` | `pvPortMalloc`/`vPortFree`, static allocation |
| Set up address translation | Builds page tables, loads CR3/TTBR/`satp` on context switch | Usually none: physical addresses only |
| Protection | Page permissions, user/kernel split | **MPU** regions per task (optional) |
| Move data between RAM and storage | Swapping, page cache, writeback | Not applicable |
| Reclaim memory | Page replacement, OOM killer | The application frees memory; leaks stay forever |

The hardware that helps it is the **MMU** (on application processors) or the **MPU/PMP** (on microcontrollers).

### 14.2 MMU (Memory Management Unit)

The **MMU** is hardware that sits between the CPU and the memory bus and **translates every virtual address into a physical address** using the page tables (Chapters 9-10).

**What it does:**

1. **Address translation** (virtual → physical), using the **TLB** and a **hardware page-table walker**.
2. **Protection:** checks R/W/X and user/supervisor permissions on every access. A violation raises a fault (data abort / page fault).
3. **Memory attributes per page:** cacheable or not, device memory or normal memory, shareable or not.
4. **Enables:** separate address spaces per process, demand paging, swapping, copy-on-write, shared libraries, `mmap`.

**Found in:** x86, ARM **Cortex-A** (and Cortex-R82), RISC-V with **Sv32/Sv39/Sv48**, Xtensa cores with an MMU option. You need one to run **full Linux**. (μClinux runs without an MMU, with no `fork()` and no memory protection.)

### 14.3 Why microcontrollers have no MMU

| Reason | Explanation |
|---|---|
| **Determinism** | A TLB miss triggers a page walk, adding a **variable** delay of several memory accesses. Hard real-time systems need predictable timing |
| **Cost, area and power** | The TLB, page-table walker and page tables cost silicon, energy and RAM. MCUs have only tens to hundreds of KB of RAM |
| **No need** | One firmware image, no disk to swap to, no untrusted programs to load, all addresses known at link time |
| **Simplicity** | Physical = virtual makes debugging, DMA and interrupt handling simpler |

What MCUs **do** need is **protection**: stop a buggy task from corrupting the kernel, catch stack overflows and NULL pointers. That is the job of the **MPU**.

### 14.4 MPU (Memory Protection Unit)

An **MPU** divides the address space into a small number of **regions** (typically 8 or 16). Each region has a **base address, a size, access permissions and memory attributes**. On every access, the MPU checks whether it is allowed.

**The MPU does NOT translate addresses.** Virtual = physical. It only **allows or blocks** accesses.

```
 CPU issues address 0x20004010, a write, from unprivileged code
        │
        ▼
 MPU: which enabled regions contain 0x20004010?
        │
   ┌────┴─────────────────────────────────────────┐
   │ no region matches   → background map (if     │
   │                       PRIVDEFENA and         │
   │                       privileged) else FAULT │
   │ region(s) match     → use the HIGHEST        │
   │                       numbered one (ARMv7-M) │
   │ permission OK?      → access proceeds        │
   │ permission denied?  → MemManage fault        │
   └──────────────────────────────────────────────┘
```

**Typical MPU uses:**

| Use | How |
|---|---|
| **Stack overflow detection** | A small no-access **guard region** at the bottom of each stack. The overflowing push faults immediately |
| **NULL pointer detection** | A no-access region at address `0x00000000`. Dereferencing `NULL` faults instead of reading the vector table |
| **Protect code from being overwritten** | Flash/code region read-only |
| **Stop code injection** | RAM regions marked **XN (execute never)** |
| **Isolate RTOS tasks** | Unprivileged tasks can access only their own stack and explicitly granted buffers (FreeRTOS-MPU, Zephyr userspace) |
| **Protect the kernel** | Kernel data is accessible only in privileged mode |
| **Cache policy** | Mark DMA buffers non-cacheable (Cortex-M7, Chapter 12.12) |
| **Device memory** | Mark the peripheral space as Device / strongly ordered, so accesses are not merged, reordered or speculated |

### 14.5 ARMv7-M MPU in detail (Cortex-M3, M4, M7)

**Region rules (memorise these):**

- **8 regions** (16 on some Cortex-M7 parts), numbered 0-7.
- **Size** is a **power of two** from **32 bytes to 4 GB**, encoded as `SIZE` where size = 2^(SIZE+1).
- **Base address must be aligned to the region size.** A 64 KB region must start at a multiple of 64 KB.
- **Subregions:** regions of **256 bytes or larger** are split into **8 equal subregions**, each of which can be disabled with the `SRD` bits. This lets you build sizes that are not powers of two.
- **Overlaps are allowed.** The **highest-numbered** region wins. Put guard regions in high-numbered slots.
- **Background region:** with `PRIVDEFENA = 1`, privileged code may access addresses **not covered by any region** using the default memory map. Unprivileged code faults there.

**Registers:**

| Register | Purpose |
|---|---|
| `MPU->TYPE` | How many regions exist (`DREGION` field) |
| `MPU->CTRL` | `ENABLE`, `HFNMIENA` (keep the MPU on in HardFault/NMI), `PRIVDEFENA` (background region) |
| `MPU->RNR` | Selects which region the next two registers configure |
| `MPU->RBAR` | Region base address |
| `MPU->RASR` | Attributes: `XN`, `AP`, `TEX`, `S`, `C`, `B`, `SRD`, `SIZE`, `ENABLE` |

**Access permissions (`AP` field):**

| AP | Privileged | Unprivileged |
|---|---|---|
| `000` | No access | No access |
| `001` | Read/write | No access |
| `010` | Read/write | Read-only |
| `011` | Read/write | Read/write |
| `101` | Read-only | No access |
| `110` | Read-only | Read-only |
| `111` | Read-only | Read-only |

**Memory type (`TEX`, `C`, `B`), the common encodings:**

| TEX | C | B | Memory type |
|---|---|---|---|
| 000 | 0 | 0 | Strongly-ordered |
| 000 | 0 | 1 | Device (shareable) |
| 000 | 1 | 0 | Normal, write-through, no write-allocate |
| 000 | 1 | 1 | Normal, write-back, no write-allocate |
| 001 | 0 | 0 | Normal, **non-cacheable** |
| 001 | 1 | 1 | Normal, write-back, read and write allocate |
| 010 | 0 | 0 | Device (non-shareable) |

### 14.6 Configuring the MPU in code

**Write it yourself: MPU setup for a typical STM32F4 (Cortex-M4)**

```c
#include <stdint.h>
#include "stm32f4xx.h"                     /* CMSIS: MPU, SCB, register bit masks */

extern uint32_t _sstack;                    /* lowest address of the main stack (linker) */

/* Build an RASR value. size_field encodes 2^(size_field + 1) bytes. */
#define RASR(xn, ap, tex, s, c, b, size_field)                                   \
    (((uint32_t)(xn)  << MPU_RASR_XN_Pos)  | ((uint32_t)(ap) << MPU_RASR_AP_Pos) | \
     ((uint32_t)(tex) << MPU_RASR_TEX_Pos) | ((uint32_t)(s)  << MPU_RASR_S_Pos)  | \
     ((uint32_t)(c)   << MPU_RASR_C_Pos)   | ((uint32_t)(b)  << MPU_RASR_B_Pos)  | \
     ((uint32_t)(size_field) << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk)

#define SIZE_32B    4u                      /* 2^5  */
#define SIZE_256B   7u                      /* 2^8  */
#define SIZE_128KB 16u                      /* 2^17 */
#define SIZE_512KB 18u                      /* 2^19 */
#define SIZE_512MB 28u                      /* 2^29 */

static void mpu_region(uint32_t n, uint32_t base, uint32_t rasr)
{
    MPU->RNR  = n;
    MPU->RBAR = base;                       /* must be aligned to the region size */
    MPU->RASR = rasr;
}

void mpu_init(void)
{
    __DMB();                                /* finish outstanding memory accesses   */
    MPU->CTRL = 0;                          /* disable while reconfiguring          */

    /* 0: Flash 512 KB @ 0x08000000: read-only for all, executable, normal memory     */
    mpu_region(0, 0x08000000u, RASR(0, 0x6, 0, 0, 1, 0, SIZE_512KB));

    /* 1: SRAM 128 KB @ 0x20000000: read/write for all, EXECUTE NEVER                 */
    mpu_region(1, 0x20000000u, RASR(1, 0x3, 0, 0, 1, 1, SIZE_128KB));

    /* 2: Peripherals 512 MB @ 0x40000000: device memory, execute never              */
    mpu_region(2, 0x40000000u, RASR(1, 0x3, 0, 1, 0, 1, SIZE_512MB));

    /* 3: NULL guard 256 B @ 0x00000000: no access (code runs from 0x08000000 and
          SystemInit() points VTOR there, so the alias at address 0 is not needed)   */
    mpu_region(3, 0x00000000u, RASR(1, 0x0, 0, 0, 0, 0, SIZE_256B));

    /* 4: Stack guard 32 B at the bottom of the stack: no access. Highest number wins,
          so it overrides the SRAM region it sits inside. Must be 32-byte aligned.    */
    mpu_region(4, (uint32_t)&_sstack, RASR(1, 0x0, 0, 0, 0, 0, SIZE_32B));

    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk     /* privileged code may use the default map */
              | MPU_CTRL_ENABLE_Msk;
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;/* MemManage fault instead of HardFault     */
    __DSB();                                /* make sure the MPU writes have completed  */
    __ISB();                                /* refetch instructions with new settings   */
}
```

**Why `DSB` + `ISB` at the end?** Writes to system registers might not take effect immediately. `DSB` waits until they complete, and `ISB` flushes the pipeline, so the **next** instruction already runs under the new MPU settings (Chapter 17).

**Caveat on stack guards:** when the main stack overflows into the guard, the hardware tries to push the exception frame for the MemManage fault onto the **same overflowed stack**. That push faults too (`MSTKERR`) and escalates to HardFault. Common solutions: make the guard bigger than one exception frame, or run fault handlers on a separate stack. On ARMv8-M, use `MSPLIM`/`PSPLIM` instead.

### 14.7 Handling a MemManage fault

```c
#include <stdint.h>
#include "stm32f4xx.h"

volatile uint32_t g_fault_addr;             /* inspect with a debugger, or log after reset */
volatile uint32_t g_fault_status;

void MemManage_Handler(void)
{
    uint32_t mmfsr = SCB->CFSR & 0xFFu;     /* MemManage Fault Status = CFSR[7:0] */

    g_fault_status = mmfsr;
    if (mmfsr & (1u << 7))                  /* MMARVALID: MMFAR holds the faulting address */
        g_fault_addr = SCB->MMFAR;

    /*  bit 0 IACCVIOL : instruction fetch from an XN or no-access region
        bit 1 DACCVIOL : data access violation (address in MMFAR)
        bit 3 MUNSTKERR: fault while unstacking on exception return
        bit 4 MSTKERR  : fault while stacking on exception entry (typical stack overflow)
        bit 5 MLSPERR  : fault during lazy FPU state preservation                    */

    SCB->CFSR = mmfsr;                      /* write 1 to clear (W1C) the flags we read */
    for (;;) { }                            /* or save a crash record and reset         */
}
```

To find **which instruction** faulted, read the **stacked PC** from the exception frame (word 6 of the frame on the active stack). Fault-handler libraries such as CmBacktrace do this automatically.

### 14.8 ARMv8-M MPU (Cortex-M23, M33, M55, M85)

| | ARMv7-M MPU | ARMv8-M MPU |
|---|---|---|
| Region definition | Base + **power-of-two size** | Base + **limit** (any size, **32-byte granularity**) |
| Alignment | Base aligned to the region size | 32-byte aligned |
| Subregions | Yes (8 per region) | No (not needed) |
| Overlapping regions | Allowed: the highest number wins | **Not allowed**: an access hitting two regions **faults** |
| Memory attributes | TEX/C/B/S bits per region | Index into the `MAIR0`/`MAIR1` attribute registers |
| Extras | — | Separate **Secure and Non-secure MPUs** with TrustZone, plus stack limit registers `MSPLIM`/`PSPLIM` |

```c
/* ARMv8-M: attributes are defined once in MAIR, regions refer to them by index. */
MPU->MAIR0 = (0xFFu << 0)                    /* Attr0: normal memory, write-back cacheable */
           | (0x44u << 8);                   /* Attr1: normal memory, non-cacheable        */

MPU->RNR  = 0;                               /* region 0: a DMA buffer area                 */
MPU->RBAR = (0x20010000u & ~0x1Fu)           /* base (32-byte aligned)                      */
          | (0u << 3)                        /* SH: non-shareable                           */
          | (1u << 1)                        /* AP = 01: read/write, any privilege          */
          | 1u;                              /* XN: execute never                           */
MPU->RLAR = (0x20017FFFu & ~0x1Fu)           /* limit (inclusive, last 32-byte block)       */
          | (1u << 1)                        /* AttrIndx = 1 -> non-cacheable               */
          | 1u;                              /* EN                                          */
```

### 14.9 Privilege levels: what makes protection work

Protection only works if application code **cannot simply reconfigure the MPU**.

- Cortex-M has **privileged** and **unprivileged** (thread) modes. The `CONTROL.nPRIV` bit drops thread mode to unprivileged.
- Unprivileged code **cannot** write MPU registers, change `CONTROL` or disable interrupts.
- To get back into privileged mode, it must execute **`SVC`** (supervisor call). The kernel handles the SVC and checks the request.
- **FreeRTOS-MPU** works this way: `xTaskCreateRestricted()` creates an unprivileged task with its own stack region plus up to `portNUM_CONFIGURABLE_REGIONS` extra regions. API calls go through wrappers that raise privilege. Accessing anything else raises a MemManage fault.

### 14.10 RISC-V PMP (Physical Memory Protection)

RISC-V microcontrollers, including the **ESP32-C3/C6/H2** (RISC-V cores), use **PMP**, RISC-V's equivalent of an MPU.

**Privilege modes:** **M-mode** (machine, the most privileged, where firmware runs), optionally **S-mode** (supervisor, OS kernel) and **U-mode** (user, applications).

**PMP basics:**

- Up to **16 or 64 entries**. Each has a `pmpaddrN` CSR (address **shifted right by 2**) and an 8-bit config byte packed into the `pmpcfgN` CSRs (on RV32, `pmpcfg0` holds entries 0-3).
- Config byte: **R** (bit 0), **W** (bit 1), **X** (bit 2), **A** (bits 3-4: address-matching mode), **L** (bit 7, lock).
- **Address-matching modes:**

| A | Mode | Region |
|---|---|---|
| 0 | OFF | Entry disabled |
| 1 | **TOR** (Top Of Range) | `pmpaddr[i-1] ≤ addr < pmpaddr[i]` (entry 0 uses 0 as the bottom) |
| 2 | **NA4** | Exactly 4 bytes |
| 3 | **NAPOT** | Naturally aligned power of two, 8 bytes or more; the size is encoded in the trailing 1 bits of `pmpaddr` |

**Rules:**

- The **lowest-numbered matching entry** decides (unlike ARMv7-M, where the highest wins).
- **S/U-mode** accesses that match **no** entry **fail** (if any PMP entries exist).
- **M-mode** is **not** restricted, **unless** the entry has the **L (lock)** bit. A locked entry also applies to M-mode and cannot be changed until reset. This is how a boot ROM protects itself.
- Violations raise **access-fault exceptions** (instruction, load or store access fault).

**Write it yourself: set up two PMP regions (RV32, M-mode code preparing U-mode)**

```c
#include <stdint.h>

#define PMP_R      0x01u
#define PMP_W      0x02u
#define PMP_X      0x04u
#define PMP_TOR    0x08u                 /* A = 1 */
#define PMP_NAPOT  0x18u                 /* A = 3 */
#define PMP_L      0x80u                 /* lock: also applies to M-mode */

/* NAPOT encoding: base aligned to size, size a power of two >= 8 bytes. */
static inline uint32_t pmp_napot(uint32_t base, uint32_t size)
{
    return (base >> 2) | ((size >> 3) - 1u);
}

/* Example addresses: application code at 0x80000000 (64 KB), app data at 0x80010000 (16 KB). */
void pmp_setup_for_user_mode(void)
{
    uint32_t code = pmp_napot(0x80000000u, 64u * 1024u);
    uint32_t data = pmp_napot(0x80010000u, 16u * 1024u);

    uint32_t cfg = ((PMP_R | PMP_X | PMP_NAPOT) << 0)    /* entry 0: code  R-X */
                 | ((PMP_R | PMP_W | PMP_NAPOT) << 8);   /* entry 1: data  RW- */

    __asm__ volatile ("csrw pmpaddr0, %0" :: "r"(code));
    __asm__ volatile ("csrw pmpaddr1, %0" :: "r"(data));
    __asm__ volatile ("csrw pmpcfg0,  %0" :: "r"(cfg));
    /* Then: set mstatus.MPP = 0 (U-mode), mepc = user entry point, and execute 'mret'.
       Any U-mode access outside these two regions now raises an access fault.         */
}
```

**NAPOT encoding check:** a 16 KB region at `0x80010000`: `(0x80010000 >> 2) | (16384 / 8 − 1)` = `0x20004000 | 0x7FF` = `0x200047FF`. The 11 trailing ones encode 2^(11+3) = 16 KB.

**Extensions:** **Smepmp (ePMP)** can also stop M-mode from executing or accessing user memory (like ARM's PXN/PAN), and the **IOPMP** proposal protects memory from DMA masters. The ESP32-C3 combines PMP with Espressif's own **PMS** (permission control) hardware. ESP-IDF configures both automatically when its memory-protection option is enabled (Chapter 16).

### 14.11 MMU vs MPU vs PMP

| Feature | MMU | MPU (ARM Cortex-M) | PMP (RISC-V) |
|---|---|---|---|
| Address translation | **Yes** (virtual → physical) | **No** | **No** |
| Protection | Per page (4 KB+) | Per region (8-16 regions) | Per entry (16-64 entries) |
| Granularity | Page size (4 KB, 2 MB, 1 GB) | 32 B min (v7-M: power of two; v8-M: 32 B steps) | 4 B (NA4/TOR), 8 B+ (NAPOT) |
| Number of mappings | Practically unlimited (tables in RAM) | Fixed (8/16 hardware registers) | Fixed (16/64 CSRs) |
| Configuration stored in | **Page tables in RAM** + TLB | **Hardware registers** | **CSRs** |
| Timing | Variable (TLB misses, page walks) | **Deterministic**: checked in parallel | **Deterministic** |
| Virtual memory, demand paging, swapping | Yes | No | No |
| Separate address spaces per process | Yes | No (one shared physical space) | No |
| Typical OS | Linux, Windows, Android, QNX | FreeRTOS(-MPU), Zephyr, ThreadX, bare metal | FreeRTOS, Zephyr, ESP-IDF, Keystone enclaves |
| Typical cores | Cortex-A, x86, RV64 with Sv39 | Cortex-M3/M4/M7/M23/M33, Cortex-R | ESP32-C3/C6, SiFive E-series |

> **Interview one-liner:** "An MMU **translates and protects**: it gives every process its own virtual address space using page tables. An MPU **only protects**: it checks each access against a few fixed regions without translating anything, which keeps it cheap and deterministic, ideal for microcontrollers."

---

## 15. Memory Management in FreeRTOS

### 15.1 Where FreeRTOS uses memory

Every kernel object needs RAM:

| Object | Memory needed |
|---|---|
| **Task** | A **TCB** (Task Control Block, ~100 bytes or more depending on configuration) + its own **stack** |
| **Queue** | A queue control structure + storage for `length × item_size` bytes |
| **Semaphore / mutex** | A queue structure with no item storage (it is implemented as a queue) |
| **Software timer** | A timer structure; all timers share the timer service task and its command queue |
| **Event group, stream/message buffer** | A control structure (+ a data buffer for stream/message buffers) |

This memory can come from the **FreeRTOS heap** (dynamic) or from **buffers you provide** (static).

### 15.2 Dynamic vs static allocation

| | Dynamic | Static |
|---|---|---|
| Config | `configSUPPORT_DYNAMIC_ALLOCATION 1` | `configSUPPORT_STATIC_ALLOCATION 1` |
| API | `xTaskCreate`, `xQueueCreate`, `xSemaphoreCreateMutex` ... | `xTaskCreateStatic`, `xQueueCreateStatic`, `xSemaphoreCreateMutexStatic` ... |
| Memory from | `pvPortMalloc()` (the FreeRTOS heap) | Variables **you** declare (usually globals) |
| Can fail at run time? | Yes: returns `NULL` / `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` | No: the creation can fail only on bad arguments |
| RAM usage visible at link time? | No | **Yes**: the linker/map file shows exactly what is used |
| Deleting objects | Memory is returned to the heap | You may reuse your buffers |
| Preferred for | Prototyping, objects created and deleted at run time | **Safety-critical** and certified systems, long-running products |

**Write it yourself: creating a task and a queue statically**

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SENSOR_STACK_WORDS  256             /* vanilla FreeRTOS: depth in WORDS, not bytes */
#define QUEUE_LENGTH        10

static StackType_t   sensor_stack[SENSOR_STACK_WORDS];
static StaticTask_t  sensor_tcb;            /* holds the TCB: no heap needed */

static uint8_t       queue_storage[QUEUE_LENGTH * sizeof(uint32_t)];
static StaticQueue_t queue_struct;

static void sensor_task(void *arg)
{
    QueueHandle_t q = (QueueHandle_t)arg;
    uint32_t sample = 0;
    for (;;) {
        sample++;
        xQueueSend(q, &sample, portMAX_DELAY);   /* the value is COPIED into the queue */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_start(void)
{
    QueueHandle_t q = xQueueCreateStatic(QUEUE_LENGTH, sizeof(uint32_t),
                                         queue_storage, &queue_struct);

    TaskHandle_t t = xTaskCreateStatic(sensor_task, "sensor", SENSOR_STACK_WORDS,
                                       (void *)q, tskIDLE_PRIORITY + 2,
                                       sensor_stack, &sensor_tcb);
    configASSERT(q != NULL && t != NULL);
    vTaskStartScheduler();
}

/* Required when configSUPPORT_STATIC_ALLOCATION == 1: memory for the idle task.
   (FreeRTOS V11 changed the last parameter's type to configSTACK_DEPTH_TYPE *.) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize)
{
    static StaticTask_t idle_tcb;
    static StackType_t  idle_stack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
/* If configUSE_TIMERS == 1, provide vApplicationGetTimerTaskMemory() the same way. */
```

### 15.3 The five heap implementations

FreeRTOS does not use the C library `malloc`. Instead it calls `pvPortMalloc()` and `vPortFree()`, and you **choose one implementation** by compiling exactly **one** of `heap_1.c` … `heap_5.c`. Except for heap_3, the heap is a static array `ucHeap[configTOTAL_HEAP_SIZE]`.

| | heap_1 | heap_2 | heap_3 | heap_4 | heap_5 |
|---|---|---|---|---|---|
| `vPortFree` supported? | **No** | Yes | Yes | Yes | Yes |
| Algorithm | Bump pointer: just advance | **Best fit** | Wraps the C library `malloc`/`free` | **First fit** | First fit (same as heap_4) |
| Coalesces free blocks? | — | **No** | Depends on the libc | **Yes** | **Yes** |
| Fragmentation | None (nothing is freed) | High if sizes vary | Depends on the libc | Low | Low |
| Deterministic? | **Yes** | No | No | No (but bounded in practice) | No |
| Memory regions | One array | One array | Linker-defined heap | One array (can be placed with `configAPPLICATION_ALLOCATED_HEAP`) | **Several non-contiguous regions** |
| Thread safety | Scheduler suspended during allocation | Scheduler suspended | **Wraps `malloc` with `vTaskSuspendAll()`** | Scheduler suspended | Scheduler suspended |
| Use when | Objects created once at start-up and never deleted | **Legacy**: superseded by heap_4 | You must share the libc heap | **Default choice** for most applications | RAM is split across banks (internal SRAM + external SDRAM, or SRAM1 + SRAM2) |

**How they work:**

- **heap_1:** keeps one index into `ucHeap`. Allocation = align the size, check that it fits, return `&ucHeap[index]`, advance the index. It is impossible to fragment and fully deterministic, but memory can never be returned. Many safety-critical systems use exactly this: "allocate everything at boot".
- **heap_2:** keeps a free list **sorted by size** and picks the **best fit**, but **never merges** neighbouring free blocks. It works only if you keep freeing and reallocating **the same sizes** (for example, creating and deleting identical tasks). It is kept only for backward compatibility.
- **heap_3:** calls the toolchain's `malloc`/`free` inside `vTaskSuspendAll()`/`xTaskResumeAll()`. `configTOTAL_HEAP_SIZE` has no effect; the linker script decides the heap size.
- **heap_4:** a free list **sorted by address**. Allocation uses **first fit** and splits blocks. `vPortFree` inserts the block back into the address-ordered list and **merges it with adjacent free blocks**. It provides `xPortGetMinimumEverFreeHeapSize()` and `vPortGetHeapStats()`.
- **heap_5:** the heap_4 algorithm spread over **multiple memory regions** that you describe at start-up.

**Using heap_5:**

```c
#include "FreeRTOS.h"

static uint8_t internal_heap[32 * 1024];                    /* region in on-chip SRAM */

static const HeapRegion_t heap_regions[] = {
    { internal_heap,              sizeof internal_heap },  /* regions in ADDRESS order */
    { (uint8_t *)0xC0000000u,     2u * 1024u * 1024u    },  /* external SDRAM (2 MB)    */
    { NULL,                       0                     }   /* terminator               */
};

int main(void)
{
    hardware_init();
    sdram_init();                           /* external RAM must work before it is used */
    vPortDefineHeapRegions(heap_regions);   /* BEFORE any object is created            */

    /* ... create tasks and queues, then vTaskStartScheduler() ... */
    for (;;) { }
}
```

### 15.4 Monitoring heap usage

```c
size_t free_now   = xPortGetFreeHeapSize();               /* free bytes right now            */
size_t free_worst = xPortGetMinimumEverFreeHeapSize();    /* low-water mark since boot (4/5) */

HeapStats_t stats;                                        /* heap_4 / heap_5 only            */
vPortGetHeapStats(&stats);
/* stats.xSizeOfLargestFreeBlockInBytes, xNumberOfFreeBlocks, xNumberOfSuccessfulAllocations ... */
```

**Detecting fragmentation:** compare `xAvailableHeapSpaceInBytes` (total free) with `xSizeOfLargestFreeBlockInBytes`. If 20 KB is free but the largest block is 1 KB, the heap is badly fragmented, and a 2 KB request will fail.

### 15.5 Hooks: getting told when things go wrong

```c
/* configUSE_MALLOC_FAILED_HOOK = 1: called when pvPortMalloc() returns NULL. */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    log_fatal("heap exhausted: free=%u", (unsigned)xPortGetFreeHeapSize());
    for (;;) { }                            /* or trigger a controlled reset */
}

/* configCHECK_FOR_STACK_OVERFLOW = 1 or 2: called when a task's stack overflowed. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    /* The stack is already corrupted: do as little as possible, then halt or reset. */
    save_crash_record("stack overflow", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}
```

**Stack-overflow checking methods:**

| `configCHECK_FOR_STACK_OVERFLOW` | How it checks | Catches |
|---|---|---|
| **1** | At each context switch, checks that the saved **stack pointer** is still inside the task's stack | Overflow that is still in progress when the task is switched out. Fast, but misses peaks that came and went |
| **2** | Method 1 **plus** checks that the **last 16 bytes** of the stack still hold the fill pattern (`0xA5`) written at task creation | Also peaks that have already unwound. Slightly slower |
| **3** | Available on some ports: also checks the **ISR stack** | Interrupt stack overflows |

These are **not guaranteed** to catch every overflow: an overflow can jump over the checked bytes, or corrupt memory before the next context switch. **MPU guard regions** or **stack-limit registers** (ARMv8-M) catch overflows the moment they happen.

### 15.6 Sizing task stacks

```c
/* How close has this task's stack ever come to overflowing? */
UBaseType_t headroom = uxTaskGetStackHighWaterMark(NULL);   /* NULL = calling task */
/* Vanilla FreeRTOS: result in WORDS. ESP-IDF: in BYTES. 0 = it has already overflowed. */
```

**Procedure:**

1. Start generous (e.g. 1-2 KB).
2. Run every feature and worst-case path (error handling, logging with `printf`, deep call chains).
3. Read the high-water mark for every task (`vTaskList()` or `uxTaskGetSystemState()`).
4. Set the stack to the **used amount + 20-50% margin**.

> **The words vs bytes trap.** In vanilla FreeRTOS, `xTaskCreate(..., usStackDepth, ...)` is in **words**. On a 32-bit MCU, `256` means **1024 bytes**. In **ESP-IDF**, the same parameter is in **bytes**, so `256` means only 256 bytes. Copying code between the two silently makes stacks 4× too small (or too big).

### 15.7 Thread safety and ISRs

- **`pvPortMalloc` is thread-safe.** It suspends the scheduler while working on the heap.
- **It is NOT ISR-safe.** Never call `pvPortMalloc`, `vPortFree`, `malloc` or `free` from an interrupt. They are not reentrant, they can take an unbounded time, and suspending the scheduler does not block interrupts. If an ISR needs a buffer, take it from a **pre-allocated pool** protected by a critical section (Chapter 5.3), or send the work to a task.
- **The C library's `malloc`** (newlib), used directly or by `printf`, `strdup` and so on, is **not thread-safe** unless you provide the lock hooks `__malloc_lock()` and `__malloc_unlock()`. With newlib's reentrancy support (`configUSE_NEWLIB_REENTRANT`), each task also gets its own `struct _reent`. Many vendor ports provide this. Check that yours does.

### 15.8 Passing data between tasks: by copy or by reference

FreeRTOS queues **copy** the item into the queue storage and copy it out again.

| | By copy (`xQueueSend(q, &data, ...)`) | By reference (send a **pointer**) |
|---|---|---|
| What is copied | The whole item (`item_size` bytes) | Only a 4-byte pointer |
| Ownership | Sender and receiver each have their own copy: simple and safe | The **buffer must stay valid** until the receiver is finished. Clear ownership rules are needed |
| Good for | Small items (events, a few bytes) | Large buffers (audio frames, packets, images) |
| Typical pattern | — | A **pool** of buffers: the producer takes a buffer from the pool, fills it and sends the pointer; the consumer processes it and **returns it to the pool** |

**Never send a pointer to a local (stack) variable** through a queue, unless the sender is guaranteed to wait until the receiver has finished with it. The sender's stack frame may be gone by the time the receiver reads it.

### 15.9 Common FreeRTOS memory bugs

| Symptom | Likely cause |
|---|---|
| Random crashes, corrupted variables | A task stack overflow (enable method 2, check high-water marks) |
| `xTaskCreate` returns `pdFAIL` | `configTOTAL_HEAP_SIZE` too small, or the heap is fragmented |
| Crash on `vPortFree` | Double free, freeing a pointer from `malloc` (another heap), or heap corruption by a buffer overflow |
| HardFault inside an ISR | Calling a non-`FromISR` API, or `malloc`/`printf` inside the ISR |
| Heap shrinks over days | A memory leak (for example, a deleted task whose resources were never freed) |
| Memory of a deleted task not reclaimed | When a task **deletes itself**, the **idle task** frees its stack and TCB. If the idle task never runs (higher-priority tasks never block), the memory is never released |

---

## 16. ESP32 Memory Architecture

### 16.1 The ESP32 family at a glance

| Chip | CPU | Internal SRAM | External memory | Notes |
|---|---|---|---|---|
| **ESP32** | Xtensa LX6, dual-core | 520 KB | Flash up to 16 MB, PSRAM up to 4 MB mapped (8 MB with bank switching) | The original |
| **ESP32-S2** | Xtensa LX7, single-core | 320 KB | Flash + PSRAM | USB OTG |
| **ESP32-S3** | Xtensa LX7, dual-core | 512 KB | Flash + PSRAM (quad/octal) | Vector instructions for AI |
| **ESP32-C3** | **RISC-V RV32IMC**, single-core | 400 KB (16 KB usable as cache) | Flash only (no PSRAM) | Low cost, PMP |
| **ESP32-C6** | **RISC-V**, single HP core + LP core | 512 KB HP + 16 KB LP | Flash | Wi-Fi 6, Thread/Zigbee |

All of them: execute code from **external SPI flash through a cache** (XIP), have a **ROM** holding the first-stage bootloader and library functions, and have small **RTC memory** that keeps its contents in deep sleep.

### 16.2 The original ESP32's memory map

The same physical memory can appear at **two addresses**: one on the **instruction bus** (for fetching code) and one on the **data bus** (for loads and stores).

| Region | Bus | Address range | Size | Use |
|---|---|---|---|---|
| Internal ROM 0 | Instruction | `0x4000_0000`-`0x4005_FFFF` | 384 KB | Boot ROM, ROM functions |
| Internal ROM 1 | Data | `0x3FF9_0000`-`0x3FF9_FFFF` | 64 KB | ROM data |
| **SRAM 0** | Instruction | `0x4007_0000`-`0x4009_FFFF` | 192 KB | **IRAM**. The first 64 KB is the flash cache for the two CPUs, so ESP-IDF's IRAM starts at `0x4008_0000` |
| **SRAM 1** | Data **and** instruction (aliased) | `0x3FFE_0000`-`0x3FFF_FFFF` (data) / `0x400A_0000`-`0x400B_FFFF` (instruction) | 128 KB | DRAM, or IRAM |
| **SRAM 2** | Data | `0x3FFA_E000`-`0x3FFD_FFFF` | 200 KB | **DRAM** (`.data`, `.bss`, heap) |
| RTC FAST | Data / instruction | `0x3FF8_0000` / `0x400C_0000` | 8 KB | Deep-sleep wake stub code, RTC data (PRO CPU) |
| RTC SLOW | Data / instruction | `0x5000_0000` | 8 KB | Kept in deep sleep (ULP coprocessor) |
| External flash (code) | Instruction, cached | `0x400C_2000`-`0x40BF_FFFF` | up to ~11 MB | **IROM**: `.text` executed in place |
| External flash (data) | Data, cached | `0x3F40_0000`-`0x3F7F_FFFF` | 4 MB | **DROM**: `.rodata`, `const` data |
| External PSRAM | Data, cached | `0x3F80_0000`-`0x3FBF_FFFF` | 4 MB | Large buffers |
| Peripherals | Data | `0x3FF0_0000`-`0x3FF7_FFFF` | 512 KB | Memory-mapped registers |

**Key takeaways:**

- `0x3F...` and `0x3FF...` addresses are **data**. `0x40...` addresses are **instructions**. A crash address tells you which bus was involved.
- On the classic ESP32, **statically allocated DRAM (`.data` + `.bss`) is limited to 160 KB**, even though about 320 KB of DRAM exists. The rest is available only as **heap**.
- On the classic ESP32, **IRAM supports only 32-bit aligned word access**. A byte or 16-bit access to IRAM raises a `LoadStoreError` exception.

### 16.3 The ESP32-C3 (RISC-V) memory organisation

- **400 KB SRAM**. 16 KB can serve as the instruction cache; the remaining **384 KB is one physical block** reachable from both the instruction bus and the data bus. ESP-IDF splits it into an **IRAM part** and a **DRAM part** at link time. There is no fixed "IRAM chip" and "DRAM chip" as on the ESP32: the split is software-defined.
- **384 KB ROM** (bootloader, libc and Wi-Fi helper functions).
- **8 KB RTC FAST memory** (kept in deep sleep).
- **External flash** through the cache, mapped by an **MMU with 64 KB pages**. Code executes from one address window and read-only data is read through another.
- **No PSRAM** support.
- **Memory protection:** the RISC-V **PMP** plus Espressif's **PMS (Permission Control)** hardware. With ESP-IDF's **memory protection** option (on by default), the IRAM part is **not writable** and the DRAM part is **not executable**. A stray write into code, or a jump into data, then raises an immediate fault instead of silently corrupting the program.

### 16.4 Flash, cache and the IRAM rule

ESP32 code normally runs **directly from external flash** through the **cache**. Anything not in the cache is fetched from flash over SPI.

**The catch:** while the SPI flash is being **erased or written** (NVS writes, OTA updates, SPIFFS/LittleFS), the flash cannot be read, so **the cache is disabled**. If **any** code runs from flash, or **any** `const` data is read from flash, during that window, the chip crashes with:

```
Guru Meditation Error: Core 0 panic'ed (Cache disabled but cached memory region accessed)
```

**Rules that follow:**

1. **ISRs that must run during flash operations** (registered with `ESP_INTR_FLAG_IRAM`) must be in IRAM: mark them `IRAM_ATTR`, and **every function they call** too.
2. **Constant data** used by those ISRs must be in DRAM: mark it `DRAM_ATTR`. A plain `const` array goes to flash (`.rodata`).
3. **String literals** in such ISRs must also be in DRAM. For logging, use `ESP_DRAM_LOGE()`/`ESP_EARLY_LOGE()` with `DRAM_STR()`.
4. IRAM is small (about 128-200 KB, shared with Wi-Fi and FreeRTOS code), so put only time-critical code there.

### 16.5 ESP-IDF placement attributes

| Attribute | Places | Use |
|---|---|---|
| `IRAM_ATTR` | Function in **IRAM** | ISRs, code that runs while the flash cache is disabled, very hot code |
| `DRAM_ATTR` | Data in **DRAM** (even if `const`) | Constant tables used by IRAM ISRs |
| `RTC_DATA_ATTR` | Data in **RTC memory**, initialised at power-on | Values that must **survive deep sleep** (boot counters, state) |
| `RTC_NOINIT_ATTR` | RTC memory, **never initialised** | Values that survive deep sleep **and** software resets |
| `RTC_IRAM_ATTR` | Function in RTC FAST memory | Deep-sleep wake stubs |
| `__NOINIT_ATTR` | Internal RAM, not initialised at boot | Data that survives a software reset (not power loss) |
| `EXT_RAM_BSS_ATTR` | Zero-initialised data in **PSRAM** | Big buffers that do not need internal RAM (needs the matching Kconfig option) |
| `DMA_ATTR` | DMA-capable internal RAM, word aligned | Static DMA buffers |
| `WORD_ALIGNED_ATTR` | 4-byte alignment | Buffers accessed as 32-bit words |

```c
#include <stdint.h>
#include "esp_attr.h"

RTC_DATA_ATTR static uint32_t boot_count;                   /* survives deep sleep             */
DRAM_ATTR static const uint8_t crc_table[256] = { 0 };      /* const, but in RAM for the ISR   */
DMA_ATTR static uint8_t spi_rx_buf[256];                    /* internal, DMA-capable           */

static void IRAM_ATTR gpio_isr_handler(void *arg)           /* safe while the flash cache is off */
{
    uint32_t pin = (uint32_t)arg;
    (void)crc_table[pin & 0xFFu];                           /* reads DRAM, not flash           */
}
```

### 16.6 The ESP-IDF heap: capabilities

ESP-IDF does **not** have one heap. It has **several heaps** (internal DRAM, IRAM usable as heap, RTC memory, PSRAM), and each has **capabilities**. You ask for memory **by capability**, and the allocator picks a heap that has all of them. The underlying allocator (`multi_heap`) uses **TLSF** since ESP-IDF v5.0.

| Capability | Meaning |
|---|---|
| `MALLOC_CAP_DEFAULT` | What `malloc()` returns: byte-addressable, normal data memory |
| `MALLOC_CAP_INTERNAL` | Internal SRAM only (not PSRAM) |
| `MALLOC_CAP_SPIRAM` | External PSRAM |
| `MALLOC_CAP_DMA` | Reachable by DMA (internal SRAM on ESP32) |
| `MALLOC_CAP_8BIT` | Byte-accessible |
| `MALLOC_CAP_32BIT` | 32-bit-accessible only. On ESP32 this includes IRAM, which needs **word** access |
| `MALLOC_CAP_EXEC` | Executable memory (IRAM) |
| `MALLOC_CAP_RTCRAM` | RTC memory |

```c
#include <stdint.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "mem";

void memory_demo(void)
{
    uint8_t *dma_buf = heap_caps_malloc(4096, MALLOC_CAP_DMA);            /* internal, DMA-capable */
    uint8_t *frame   = heap_caps_malloc(300 * 1024, MALLOC_CAP_SPIRAM);   /* big buffer in PSRAM   */
    uint32_t *words  = heap_caps_malloc(1024, MALLOC_CAP_32BIT);          /* may land in IRAM      */

    ESP_LOGI(TAG, "internal free: %u, largest block: %u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    ESP_LOGI(TAG, "PSRAM free: %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "minimum free heap since boot: %u", (unsigned)esp_get_minimum_free_heap_size());

    heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);                       /* per-heap details      */

    heap_caps_free(words);                                                /* plain free() also works */
    heap_caps_free(frame);
    heap_caps_free(dma_buf);
}
```

**Common points:**

- **DMA buffers on the ESP32 must be in internal RAM.** Most peripheral DMA engines cannot reach flash or PSRAM. Passing a PSRAM or flash pointer to an SPI DMA transfer fails, or ESP-IDF silently copies it into a bounce buffer.
- With `CONFIG_SPIRAM_USE_MALLOC`, a plain `malloc()` of a large block can come from **PSRAM**. That is slower and not DMA-capable. Use explicit capabilities for anything that matters.
- `heap_caps_get_largest_free_block()` vs `heap_caps_get_free_size()` shows **fragmentation**, exactly as with FreeRTOS's heap stats (Chapter 15.4).
- **Task stacks** come from internal RAM by default. `xTaskCreate()` stack sizes in ESP-IDF are in **bytes**.

### 16.7 Debugging memory on ESP32

| Tool | How | Finds |
|---|---|---|
| **Heap poisoning** | Kconfig: *Light* (canary words around every block) or *Comprehensive* (also fills new memory with `0xCE` and freed memory with `0xFE`) | Buffer overflows/underflows, use-after-free, use of uninitialised memory |
| **Heap integrity check** | `heap_caps_check_integrity_all(true)` | Corruption, checked on demand (call it periodically while hunting a bug) |
| **Heap tracing** | `heap_trace_init_standalone()`, `heap_trace_start(HEAP_TRACE_LEAKS)`, `heap_trace_stop()`, `heap_trace_dump()` | **Leaks**: lists blocks allocated and not freed, with call stacks |
| **Stack watchpoint** | `CONFIG_FREERTOS_WATCHPOINT_END_OF_STACK` | Puts a **hardware watchpoint** on the end of each task's stack: instant stack-overflow detection |
| **High-water mark** | `uxTaskGetStackHighWaterMark()` (bytes) | Stack sizing |
| **Size reports** | `idf.py size`, `idf.py size-components`, `idf.py size-files` | Static RAM/flash usage per component and per file |
| **Core dump** | Kconfig: save a core dump to flash or UART | Post-mortem analysis with `idf.py coredump-info` |

```c
#include "esp_heap_trace.h"

#define NUM_RECORDS 100
static heap_trace_record_t trace_records[NUM_RECORDS];   /* must be in internal RAM */

void find_leak(void)
{
    ESP_ERROR_CHECK(heap_trace_init_standalone(trace_records, NUM_RECORDS));
    ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));
    suspected_leaky_function();
    ESP_ERROR_CHECK(heap_trace_stop());
    heap_trace_dump();                                    /* prints every unfreed allocation */
}
```

### 16.8 Decoding ESP32 crashes (Guru Meditation errors)

| Error | Typical cause |
|---|---|
| `LoadProhibited` / `StoreProhibited` (Xtensa) | Read or write through an invalid pointer. `EXCVADDR` near `0x00000000` means a **NULL pointer** (plus a small struct offset) |
| `Load access fault` / `Store access fault` (RISC-V) | Same as above, on C3/C6, or a PMP/PMS protection violation |
| `InstrFetchProhibited` / `Instruction access fault` | Jump to an invalid address: corrupted function pointer, smashed return address |
| `LoadStoreError` (ESP32) | Byte or 16-bit access to IRAM, or another misaligned/wrong-width access |
| `LoadStoreAlignment` | Misaligned 16/32-bit access |
| `IllegalInstruction` | Executing data, or a corrupted code pointer |
| `Cache disabled but cached memory region accessed` | Flash code or `const` data used while flash was being written (Chapter 16.4) |
| `Stack canary watchpoint triggered (task)` | **Stack overflow** in that task |
| `CORRUPT HEAP: Bad head at 0x...` | Heap overflow or underflow, or a double free (enable heap poisoning) |
| `***ERROR*** A stack overflow in task X has been detected` | FreeRTOS stack-overflow check (method 2 equivalent) |

Use `idf.py monitor`. It decodes the backtrace addresses into **file:line** automatically, using the ELF file.

---

## 17. Memory Barriers and Ordering

### 17.1 The problem: memory operations get reordered

You write:

```c
data  = 42;      /* 1. write the payload  */
ready = 1;       /* 2. then raise a flag  */
```

and another core, an ISR or a DMA engine waits for `ready == 1` and then reads `data`. You expect it to see `42`. **It might not**, because the order can change at three levels:

1. **The compiler** may reorder independent statements, keep values in registers or merge writes, because to a single thread the result looks the same.
2. **The CPU** may execute or complete memory operations out of order. Store buffers, write buffers and out-of-order execution make writes visible to **other** observers in a different order. ARM and RISC-V are **weakly ordered**. x86 is **strongly ordered** (TSO: only a store followed by a load can appear reordered).
3. **The memory system:** caches, buses and interconnects can deliver writes to different devices in different orders.

A **memory barrier (fence)** is an instruction that **forbids reordering across it**.

> **Single-core MCU rule of thumb.** A single core always sees **its own** memory accesses in program order. Between a task and an ISR on the **same core**, only **compiler** reordering matters, and `volatile` or atomics handle it. **Hardware** barriers become necessary with **multiple cores** (ESP32 is dual-core!), **DMA**, **peripheral and system-control registers**, and **self-modifying code** (flash programming, loading code into RAM).

### 17.2 Compiler barrier

Stops the **compiler** from moving memory accesses across this point. It emits **no instruction**.

```c
#define COMPILER_BARRIER()  __asm__ volatile ("" ::: "memory")
/* "memory" clobber: the compiler must assume any memory may have changed,
   so it re-reads values after this point and finishes all writes before it. */
```

Calls into the RTOS (taking a mutex, giving a semaphore) act as compiler barriers too, because the compiler cannot see what an external function does.

### 17.3 ARM barriers: DMB, DSB, ISB

| Instruction | Name | What it guarantees | Typical use |
|---|---|---|---|
| `DMB` | Data Memory Barrier | All memory accesses **before** it are **observed** before any memory access after it. It does not wait for them to finish | Ordering between cores, or between normal memory and a DMA "start" register |
| `DSB` | Data Synchronization Barrier | **Waits** until all memory accesses before it have **completed**. No instruction after it executes until then | Before `WFI` (sleep), after writing system registers, after clearing an interrupt flag |
| `ISB` | Instruction Synchronization Barrier | **Flushes the pipeline**, so the following instructions are fetched again with the new context | After changing `CONTROL`, the MPU, `VTOR` or after writing new code |

In CMSIS they are `__DMB()`, `__DSB()` and `__ISB()`.

**Where Cortex-M code needs them:**

```c
/* 1. After reconfiguring the MPU, VTOR, or the CONTROL register: */
SCB->VTOR = (uint32_t)&ram_vector_table;
__DSB();                 /* the write has completed                         */
__ISB();                 /* following instructions see the new vector table */

/* 2. Clearing an interrupt flag at the END of an ISR. Without DSB, the write can still sit
      in a write buffer when the ISR returns, so the NVIC sees the line still active and
      re-enters the ISR a second time (a spurious "double interrupt"). */
void TIM2_IRQHandler(void)
{
    handle_timer();
    TIM2->SR = ~TIM_SR_UIF;  /* rc_w0 bit: write 0 to clear UIF, 1 elsewhere (no effect) */
    __DSB();
}

/* 3. Before sleeping, make sure every pending write has reached its target: */
__DSB();
__WFI();

/* 4. Cortex-M7: descriptors in normal memory, then "go" in a device register: */
dma_desc->len   = n;
dma_desc->owner = DESC_OWNED_BY_DMA;
__DMB();                 /* descriptor writes are observed before the doorbell write */
DMA_DEV->START = 1u;
```

### 17.4 RISC-V and x86 fences

**RISC-V** (the RVWMO weak memory model):

| Instruction | Meaning |
|---|---|
| `fence rw, rw` | Earlier reads and writes are ordered before later reads and writes (a full fence) |
| `fence w, w` | Orders earlier writes before later writes (cheaper; publishing data) |
| `fence r, rw` | Acquire-like ordering |
| `fence iorw, iorw` | Also orders **device I/O** accesses |
| `fence.i` | Synchronises the **instruction** fetch with earlier data writes. Needed after writing code into memory |
| `sfence.vma` | Flushes TLB entries after changing page tables |

**x86** is strongly ordered, so most patterns need no fence at all. `MFENCE` is a full fence. Any `LOCK`-prefixed instruction (`lock xadd`, `xchg`) is also a full barrier. `SFENCE`/`LFENCE` are used with special streaming instructions.

### 17.5 C11 atomics and memory orders

C11 `<stdatomic.h>` (and C++11 `<atomic>`) provide portable atomic operations **with built-in ordering**. The compiler then emits the right barrier for each CPU.

| Memory order | Guarantee | Cost |
|---|---|---|
| `memory_order_relaxed` | Only atomicity: no ordering with other accesses | Cheapest |
| `memory_order_acquire` (loads) | No later access moves **before** this load | Cheap |
| `memory_order_release` (stores) | No earlier access moves **after** this store | Cheap |
| `memory_order_acq_rel` | Both (read-modify-write operations) | Medium |
| `memory_order_seq_cst` (default) | One global order that all threads agree on | Most expensive |

**The acquire/release pattern** is the correct version of the "data + ready flag" example:

```
 Producer                              Consumer
 data = 42;                            while (!atomic_load_explicit(&ready, acquire)) ;
 atomic_store_explicit(&ready, 1,      use(data);   // guaranteed to see 42
                       release);
```

A **release** store "publishes" everything written before it. An **acquire** load that sees the value also sees everything that was published with it.

**Write it yourself: lock-free single-producer/single-consumer ring buffer**

This pattern is used constantly in embedded code: UART RX (ISR producer → task consumer), audio streams, logging. It needs **no lock** because each index has **exactly one writer**.

```c
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define RB_SIZE 256u                            /* must be a power of two */

typedef struct {
    uint32_t    buf[RB_SIZE];
    atomic_uint head;                           /* written ONLY by the producer */
    atomic_uint tail;                           /* written ONLY by the consumer */
} ringbuf_t;

/* Free-running indices: they simply wrap around at UINT_MAX. head - tail is
   always the number of items, because RB_SIZE divides 2^32. */

static bool rb_push(ringbuf_t *rb, uint32_t v)
{
    unsigned head = atomic_load_explicit(&rb->head, memory_order_relaxed);   /* our own index */
    unsigned tail = atomic_load_explicit(&rb->tail, memory_order_acquire);   /* slot freed?   */
    if (head - tail == RB_SIZE)
        return false;                                                       /* full          */
    rb->buf[head & (RB_SIZE - 1u)] = v;                                     /* 1. write data */
    atomic_store_explicit(&rb->head, head + 1u, memory_order_release);      /* 2. publish it */
    return true;
}

static bool rb_pop(ringbuf_t *rb, uint32_t *v)
{
    unsigned tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    unsigned head = atomic_load_explicit(&rb->head, memory_order_acquire);   /* see the data  */
    if (head == tail)
        return false;                                                       /* empty         */
    *v = rb->buf[tail & (RB_SIZE - 1u)];
    atomic_store_explicit(&rb->tail, tail + 1u, memory_order_release);      /* slot reusable */
    return true;
}

#define COUNT 1000000u
static ringbuf_t rb;

static void *producer(void *arg)
{
    (void)arg;
    for (uint32_t i = 1; i <= COUNT; i++)
        while (!rb_push(&rb, i)) { }                                         /* spin if full  */
    return NULL;
}

int main(void)
{
    pthread_t t;
    pthread_create(&t, NULL, producer, NULL);

    uint64_t sum = 0;
    uint32_t expected = 1, v;
    bool in_order = true;
    for (uint32_t n = 0; n < COUNT; ) {
        if (rb_pop(&rb, &v)) {
            if (v != expected++)
                in_order = false;
            sum += v;
            n++;
        }
    }
    pthread_join(t, NULL);
    printf("received %u values, in order: %s, sum correct: %s\n", COUNT,
           in_order ? "yes" : "NO",
           sum == (uint64_t)COUNT * (COUNT + 1u) / 2u ? "yes" : "NO");
    return 0;
}
```

**Interview points:**

- **Why power-of-two size?** `index & (SIZE − 1)` replaces the slower `index % SIZE`, and free-running unsigned indices wrap around correctly.
- **Why no lock?** Each index has exactly **one writer**. With **two** producers or **two** consumers this breaks: you would need a lock or compare-and-swap (CAS).
- **Full vs empty without wasting a slot:** free-running indices make "full" (`head − tail == SIZE`) and "empty" (`head == tail`) distinguishable. The classic alternative keeps the indices wrapped in `0..SIZE−1` and sacrifices one slot.
- **On a single-core MCU** with an ISR producer, the acquire/release atomics compile to plain loads and stores plus compiler barriers, which is still correct and costs nothing.

### 17.6 `volatile` vs atomics vs barriers

| | `volatile` | Atomic (`_Atomic`, `std::atomic`) | Barrier / fence |
|---|---|---|---|
| Prevents the compiler from caching or removing accesses | Yes | Yes | Only across the barrier point |
| Read-modify-write is indivisible (`x++`) | **No** | **Yes** | No |
| Orders **other** memory accesses | **No** (only other volatile accesses, and only in the compiler) | Yes, according to the chosen memory order | Yes: that is its only job |
| Visibility to other cores | **Not guaranteed** | Guaranteed | Guaranteed (with matching barriers on both sides) |
| Right tool for | **Hardware registers**, ISR flags on single-core MCUs | **Data shared between threads/cores** | Low-level code: drivers, lock-free structures, system registers |

**Linux kernel equivalents:** `READ_ONCE()`/`WRITE_ONCE()` (stop the compiler from tearing or merging), `smp_mb()`/`smp_rmb()`/`smp_wmb()` (barriers between CPUs), `mb()`/`rmb()`/`wmb()` (also order device I/O), and `smp_load_acquire()`/`smp_store_release()`.

---

## 18. Flash and Non Volatile Memory

### 18.1 How flash memory behaves

Flash is not "slow RAM". It follows its own rules:

| Rule | Detail |
|---|---|
| **Erased state is all 1s** | An erased byte reads `0xFF` |
| **Programming can only turn 1 → 0** | To turn a 0 back into 1 you must **erase** |
| **Erase is by sector/page/block** | You cannot erase one byte. Typical sizes: 4 KB (SPI NOR), 2 KB pages (STM32F1/L4), 16-128 KB sectors (STM32F4) |
| **Programming has a minimum width** | Byte, half-word, word or **double-word** (STM32L4/G4/H7 program 64 or 256 bits at a time with ECC, and **cannot re-program** a location that is not erased) |
| **Erase is slow** | Milliseconds to seconds per sector |
| **Limited endurance** | NOR: ~10,000-100,000 erase cycles per sector. SLC NAND ~100k, MLC ~3-10k, TLC ~1k. Internal MCU flash: often 10k |
| **Data retention** | ~10-20 years, shorter at high temperature or after many cycles |
| **Read-while-write** | A flash **bank** cannot be read while it is being erased or programmed. Code running from that bank **stalls**, or (ESP32) the cache is disabled |

### 18.2 Execute in place (XIP) and wait states

- **NOR flash** is random-access, so the CPU can **execute code directly from it** (XIP). NAND cannot: code is copied to RAM first (as in Linux systems).
- Internal flash is slower than the CPU, so it needs **wait states**. For example, STM32F4 at 168 MHz needs 5 wait states. Prefetch buffers and flash caches (STM32 **ART accelerator**) hide most of this.
- **External QSPI/OSPI flash** can be **memory-mapped** (STM32 QSPI memory-mapped mode, ESP32 flash cache), so it looks like normal read-only memory.
- **Running code from RAM** (`.RamFunc` on STM32, `IRAM_ATTR` on ESP32) is faster and deterministic, and **required** for code that runs while the flash is being erased or programmed (the flash driver itself, the ISRs that must keep running).

### 18.3 Programming internal flash (STM32 HAL example)

```c
#include "stm32f4xx_hal.h"

/* Store a 32-bit value in the last sector (sector 11 on a 1 MB STM32F4). */
HAL_StatusTypeDef flash_store_word(uint32_t address, uint32_t value)
{
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = FLASH_SECTOR_11,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,     /* 2.7-3.6 V: allows 32-bit programming */
    };
    uint32_t bad_sector = 0;

    HAL_FLASH_Unlock();                             /* flash is write-protected by default */
    HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &bad_sector);   /* whole sector -> 0xFF */
    if (st == HAL_OK)
        st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, value);
    HAL_FLASH_Lock();
    return st;
}
```

The whole 128 KB sector is erased to store 4 bytes. That is why real systems **append** records instead of rewriting them in place (see 18.5).

### 18.4 Wear leveling

If you rewrite the same sector every second with 10,000-cycle endurance, it wears out in **under 3 hours**. **Wear leveling** spreads writes over many sectors.

| Kind | How |
|---|---|
| **Dynamic wear leveling** | New writes go to the **least-worn free** block |
| **Static wear leveling** | Also **moves rarely changing data** occasionally, so its little-worn blocks join the rotation |

Where it lives:

- **Managed flash** (SD cards, eMMC, SSDs, USB sticks) has a controller with an **FTL (Flash Translation Layer)** that maps logical blocks to physical blocks and wear-levels internally.
- **Raw flash** (MCU internal flash, SPI NOR, raw NAND) needs a **flash-aware file system or key-value store**: **LittleFS** (power-loss resilient, wear leveling), **SPIFFS** (older), **ESP-IDF NVS** (key-value, log-structured), **UBIFS/JFFS2** (Linux on raw NAND/NOR).
- **FAT on raw flash** is a bad idea: it rewrites the FAT table in the same place constantly.

**Life estimate:** sectors × endurance ÷ writes per day. Example: 2 sectors × 10,000 cycles, one sector erase per day → about 20,000 days (55 years). Do the same calculation for logging every second and you will see why buffering writes in RAM matters.

### 18.5 EEPROM emulation on flash

Many MCUs have no EEPROM, so it is emulated with **two flash pages** and **log-structured records**:

```
 Page A (ACTIVE)                    Page B (ERASED)
 ┌─────────────────────────┐        ┌─────────────────────────┐
 │ header: ACTIVE          │        │ 0xFF 0xFF 0xFF ...      │
 │ {id=1, val=10}          │        │                         │
 │ {id=2, val=7}           │        │                         │
 │ {id=1, val=11}  ◄ newer │        │                         │
 │ {id=1, val=12}  ◄ newest│        │                         │
 │ 0xFF... free space      │        │                         │
 └─────────────────────────┘        └─────────────────────────┘
```

1. **Write:** **append** a record `{id, value, CRC}` into the next free (`0xFF`) slot. No erase is needed.
2. **Read:** scan for the **last** record with that id.
3. **Page full:** copy only the **latest** value of each id to page B, mark B ACTIVE, then erase A. The roles swap.
4. **Power-loss safety:** page status headers (ERASED → RECEIVING → ACTIVE) and a CRC per record. At boot, a half-finished transfer is detected and completed.

Each write costs a few bytes instead of a whole sector erase, so the page is erased far less often. ST's EEPROM-emulation application notes and ESP-IDF's NVS work on this principle.

### 18.6 Power-loss-safe updates

| Technique | Idea |
|---|---|
| **CRC / checksum per record** | Detect half-written data |
| **Sequence numbers** | Choose the newest valid copy at boot |
| **Double buffering (A/B copies)** | Write the new copy to B, verify it, then mark it valid. A stays valid until B is complete |
| **Write-ahead log / journal** | Record the intent first, apply it second, and replay or roll back after a crash |
| **A/B firmware partitions (OTA)** | New firmware goes to the inactive slot. The bootloader switches only after it is verified. The image can be rolled back if the new firmware fails to confirm itself |

### 18.7 Flash layout: example ESP32 partition table

```
 Offset      Size     Name       Purpose
 0x1000      ~28 KB   bootloader (second stage; at 0x0 on ESP32-C3/S3)
 0x8000      4 KB     partition table
 0x9000      24 KB    nvs        key-value store (Wi-Fi credentials, settings)
 0xF000      4 KB     phy_init   RF calibration data
 0x10000     1 MB     factory    application image (code runs from here via cache)
 (OTA layouts add: otadata 8 KB, ota_0, ota_1 instead of a single factory app)
```

The **boot flow** in memory terms: the boot ROM (on-chip, cannot be changed) loads the **second-stage bootloader** from flash into **RAM**. The bootloader reads the **partition table**, chooses the app (factory or ota_x), verifies it (and with **secure boot**, checks its signature), sets up the **flash MMU/cache mappings** for the app's code and read-only data, copies its IRAM/DRAM sections into internal RAM, and jumps to its entry point.

### 18.8 Overlays and bank switching (historical)

- **Overlays:** before virtual memory existed, programs larger than RAM were split into pieces that **took turns** in the same memory region, loaded by the program itself on demand.
- **Bank switching:** 8-bit MCUs (8051, PIC) with 16-bit addresses could only see 64 KB, so a bank register selected which part of a larger memory appeared in a window. The ESP32's PSRAM "himem" API uses the same trick to reach 8 MB through a 4 MB window.

---

## 19. Memory Bugs and Debugging Tools

### 19.1 The catalogue of memory bugs

| # | Bug | Example | What happens | Fix |
|---|---|---|---|---|
| 1 | **Memory leak** | `p = malloc(n); return;` (never freed) | Heap slowly runs out; device fails after hours or days | Every allocation has one clear owner who frees it; free on every error path |
| 2 | **Use-after-free** (dangling pointer) | `free(p); p->x = 1;` | Corrupts whatever now owns that memory; often crashes much later | Set `p = NULL` after `free`; clear ownership rules |
| 3 | **Double free** | `free(p); free(p);` | Corrupts the allocator's free list; a crash in a later `malloc` | `p = NULL` after `free` (`free(NULL)` is a no-op) |
| 4 | **Buffer overflow** | `char b[8]; strcpy(b, "123456789");` | Overwrites neighbouring data, the return address (stack) or heap headers | Bounds checks, `snprintf`, `strlcpy`, sizes from `sizeof` |
| 5 | **Off-by-one** | `for (i = 0; i <= N; i++) a[i] = 0;` or `malloc(strlen(s))` (no room for `'\0'`) | Writes one element past the end | `i < N`; `malloc(strlen(s) + 1)` |
| 6 | **Buffer underflow** | `p[-1] = 0;` or a loop counting down past 0 | Corrupts the heap block header in front | Check lower bounds; beware unsigned loops `for (size_t i = n - 1; i >= 0; i--)` (never ends) |
| 7 | **Uninitialised read** | `int x; if (x) ...` or reading `malloc`'d memory | Random behaviour that changes with build options | Initialise everything; `calloc` when zeros are needed |
| 8 | **NULL dereference** | `p = malloc(n); p[0] = 1;` without a check | Segfault (PC) or a read/write at address 0 (MCU: vector table!) | Check every allocation; MPU NULL guard |
| 9 | **Wild pointer** | `int *p; *p = 5;` | Writes to a random address | Initialise pointers (to `NULL` or a valid object) |
| 10 | **Freeing non-heap memory** | `int x; free(&x);` or `free(p + 4)` | Allocator corruption, crash | Free only what `malloc` returned |
| 11 | **Mismatched alloc/free** | `new[]` → `delete`, `new` → `free`, `malloc` from heap A → free in heap B | Undefined behaviour, leaked destructors | Pair them correctly: `new`/`delete`, `new[]`/`delete[]`, `malloc`/`free` |
| 12 | **Size overflow** | `malloc(count * size)` with huge `count` | Tiny allocation, then a huge overflow | `calloc`, or check `count > SIZE_MAX / size` |
| 13 | **`sizeof` pointer mistake** | `p = malloc(sizeof p);` / `memset(p, 0, sizeof p)` | Allocates or clears 4-8 bytes instead of the object | `malloc(sizeof *p)` and `sizeof` of the real object |
| 14 | **Returning a pointer to a local** | `return &local;` | Dangling pointer into a dead stack frame | static, heap, or caller-provided storage (Chapter 3.9) |
| 15 | **Stale pointer after `realloc`** | `q = p; p = realloc(p, n); *q = 1;` | `q` points to the old, freed block | Re-derive pointers after `realloc` |
| 16 | **Stack overflow** | Deep recursion, big local arrays | Silent corruption on an MCU | Size stacks, watermarks, MPU guards (Chapter 3.6) |
| 17 | **Misaligned or aliased access** | `*(uint32_t *)(buf + 1)` | HardFault on Cortex-M0, wrong code from the optimiser | `memcpy` (Chapters 6.6 and 7.2) |
| 18 | **Race on shared memory** | ISR and task both `count++` | Lost updates | Critical sections, atomics (Chapter 8.4) |

**Why memory bugs are so hard to find:** the **symptom** (a crash, wrong data) usually appears **far away** in time and code from the **cause** (the bad write). The corrupted memory belongs to someone else, who only notices later.

### 19.2 The security side

Memory bugs are the root of most serious security vulnerabilities in C and C++ code (Microsoft and Google have each reported about 70% of their severe bugs as memory-safety issues).

- **Stack smashing:** overflow a local buffer to overwrite the **return address**, so the function "returns" into attacker-controlled code. Defences: **stack canaries** (`-fstack-protector-strong`), **NX** stacks, **ASLR**, **CFI** (control-flow integrity), and hardware features such as ARM **PAC** (pointer authentication) and Intel **CET** shadow stacks.
- **Heap exploits:** corrupt allocator metadata or a function pointer stored in a heap object (use-after-free is the favourite). Defences: hardened allocators, safe unlinking checks, **MTE** (ARM Memory Tagging Extension).
- **Format-string bugs:** `printf(user_input)` lets the input read and write memory through `%x` and `%n`. Always use `printf("%s", s)`.

### 19.3 Tools

| Tool | Type | Finds | Overhead / notes |
|---|---|---|---|
| **AddressSanitizer** (`-fsanitize=address`) | Compiler instrumentation | Heap/stack/global overflow, use-after-free, double free, leaks (LeakSanitizer) | ~2× slower, ~3× memory. **Use it in every test build** |
| **UBSan** (`-fsanitize=undefined`) | Compiler instrumentation | Misaligned access, signed overflow, invalid shifts, NULL use | Low |
| **MemorySanitizer** (Clang, `-fsanitize=memory`) | Compiler instrumentation | Use of **uninitialised** memory | Needs the whole program instrumented |
| **Valgrind memcheck** | Runs the program on a simulated CPU | Leaks, invalid reads/writes, uninitialised use, bad frees | ~20-50× slower; no recompilation needed |
| **Valgrind massif** | Heap profiler | Who uses how much heap over time | |
| **GCC flags** | Compile time and run time | `-Wall -Wextra`, `-fanalyzer`, `-Wstack-usage=N`, `-fstack-usage`, `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2` | Cheap: enable them always |
| **Static analysers** | Source analysis | cppcheck, clang-tidy, Clang Static Analyzer, Coverity, PC-lint, Polyspace, MISRA checkers | Find bugs without running the code |
| **GDB watchpoints** | Debugger | `watch var` stops at the exact instruction that changes `var` | Hardware watchpoints are free on most CPUs |
| **Cortex-M DWT** | On-chip debug hardware | Data watchpoints: break or trace when an address is written | Up to 4 comparators |
| **RTOS tools** | Runtime | Stack high-water marks, heap stats, malloc-failed and stack-overflow hooks, SEGGER SystemView, Percepio Tracealyzer | |

**Example: AddressSanitizer report**

```c
/* bug.c */
#include <stdlib.h>
int main(void)
{
    int *a = malloc(5 * sizeof(int));
    a[5] = 1;                 /* one element past the end */
    free(a);
    return 0;
}
```

```
$ gcc -g -fsanitize=address bug.c && ./a.out
==4242==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x503000000024
WRITE of size 4 at 0x503000000024 thread T0
    #0 0x55d4c3a0b1d9 in main bug.c:6
0x503000000024 is located 0 bytes after 20-byte region [0x503000000010,0x503000000024)
allocated by thread T0 here:
    #0 0x7f1e2c8fd9c7 in malloc
    #1 0x55d4c3a0b19e in main bug.c:5
```

It reports the **exact line** of the bad write **and** where the block was allocated.

**Example: Valgrind leak report**

```
$ valgrind --leak-check=full ./program
==1234== 64 bytes in 1 blocks are definitely lost in loss record 1 of 1
==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:309)
==1234==    by 0x109156: create_buffer (buffer.c:12)
==1234==    by 0x1091A2: main (main.c:8)
```

**Embedded tip:** most firmware logic (parsers, protocol state machines, ring buffers, allocators) can be compiled **for the PC** and unit-tested there **with ASan and UBSan**. That catches most memory bugs before they ever reach the target.

### 19.4 Write it yourself: a simple leak detector

**Question:** "Without Valgrind, how would you find memory leaks in your firmware?" Wrap `malloc` and `free`, record every live allocation with its file and line, and print whatever is still recorded at the end.

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct alloc_rec {
    void             *ptr;
    size_t            size;
    const char       *file;
    int               line;
    struct alloc_rec *next;
} alloc_rec_t;

static alloc_rec_t *live;                      /* list of allocations not yet freed */

void *dbg_malloc(size_t size, const char *file, int line)
{
    void *p = malloc(size);
    alloc_rec_t *r = malloc(sizeof *r);
    if (p == NULL || r == NULL) {
        free(p);
        free(r);
        return NULL;
    }
    *r = (alloc_rec_t){ p, size, file, line, live };
    live = r;                                  /* push onto the live list */
    return p;
}

void dbg_free(void *p, const char *file, int line)
{
    if (p == NULL)
        return;
    for (alloc_rec_t **pp = &live; *pp != NULL; pp = &(*pp)->next) {
        if ((*pp)->ptr == p) {                 /* found: unlink the record and free both */
            alloc_rec_t *dead = *pp;
            *pp = dead->next;
            free(dead);
            free(p);
            return;
        }
    }
    fprintf(stderr, "%s:%d: free(%p) of unknown pointer (double free?)\n", file, line, p);
}

void dbg_report_leaks(void)
{
    size_t total = 0;
    for (alloc_rec_t *r = live; r != NULL; r = r->next) {
        fprintf(stderr, "LEAK: %zu bytes allocated at %s:%d\n", r->size, r->file, r->line);
        total += r->size;
    }
    fprintf(stderr, "total leaked: %zu bytes\n", total);
}

/* From here on, every malloc/free in this file is tracked. In a real project these two
   macros go in a header that application files include (but not the tracker's own .c). */
#define malloc(n) dbg_malloc((n), __FILE__, __LINE__)
#define free(p)   dbg_free((p), __FILE__, __LINE__)

int main(void)
{
    char *a = malloc(32);
    char *b = malloc(64);
    free(a);
    free(a);                                    /* double free: detected, not executed */
    (void)b;                                    /* b is never freed: reported as a leak */
    dbg_report_leaks();
    return 0;
}
```

**Extensions interviewers like:** add **guard bytes** (canaries) before and after each block and check them in `dbg_free` to detect overflows; fill freed memory with a pattern (`0xDD`) to expose use-after-free; track the **peak** usage; make it thread-safe with a mutex.

### 19.5 Defensive habits

1. **One owner per allocation.** Document who frees what ("caller must free the result").
2. **Allocate and free in the same module**, ideally at the same level of the code (`create`/`destroy` pairs).
3. **Single exit for clean-up** in C (the `goto cleanup;` pattern, used throughout the Linux kernel):

```c
int process_file(const char *path)
{
    int ret = -1;
    char *buf = NULL;
    FILE *f = fopen(path, "rb");
    if (f == NULL)
        goto out;

    buf = malloc(4096);
    if (buf == NULL)
        goto out;

    if (fread(buf, 1, 4096, f) == 0)
        goto out;

    ret = 0;                                  /* success */
out:
    free(buf);                                /* free(NULL) is fine        */
    if (f != NULL)
        fclose(f);
    return ret;                               /* no leak on any path       */
}
```

4. `p = NULL;` after `free(p)`.
5. **`sizeof *p`** instead of `sizeof(type)`: it stays correct if the type of `p` changes.
6. **Prefer static allocation and pools** in embedded systems (Chapters 4.12 and 5.3).
7. **Run sanitizers** in CI, and keep `-Wall -Wextra -Werror` clean.

### 19.6 C++: RAII and smart pointers

**RAII (Resource Acquisition Is Initialization):** tie a resource's lifetime to an object. The constructor acquires it and the destructor releases it, so it is released automatically on **every** path, including early returns and exceptions.

| Smart pointer | Ownership | Cost | Use |
|---|---|---|---|
| `std::unique_ptr<T>` | **Exclusive**: one owner, can be **moved** but not copied | Zero overhead compared with a raw pointer | The default choice |
| `std::shared_ptr<T>` | **Shared**: a **reference count** in a control block; the object is deleted when the count reaches 0 | Atomic increments/decrements, an extra allocation (unless `make_shared`) | Genuinely shared ownership |
| `std::weak_ptr<T>` | **Non-owning** observer of a `shared_ptr` object; `lock()` to use it | — | Breaking **reference cycles**, caches |

```cpp
#include <cstdio>
#include <memory>

struct Node {
    explicit Node(int v) : value(v) { std::printf("Node %d created\n", value); }
    ~Node() { std::printf("Node %d destroyed\n", value); }
    int value;
    std::shared_ptr<Node> next;   // owning link
    std::weak_ptr<Node>   prev;   // non-owning back link: no cycle, no leak
};

struct FileCloser {
    void operator()(std::FILE *f) const { if (f) std::fclose(f); }
};

int main()
{
    {
        auto a = std::make_unique<Node>(1);   // sole owner
    }                                         // "Node 1 destroyed" here, automatically

    auto first  = std::make_shared<Node>(2);
    auto second = std::make_shared<Node>(3);
    first->next  = second;                    // first owns second
    second->prev = first;                     // weak: does not keep first alive
    std::printf("use_count: first=%ld second=%ld\n", first.use_count(), second.use_count());

    std::unique_ptr<std::FILE, FileCloser> f(std::fopen("/dev/null", "w"));   // custom deleter
    if (f) std::fputs("hello", f.get());
    return 0;                                 // file closed, nodes 2 and 3 destroyed
}
```

**The classic `shared_ptr` leak:** if `prev` were a `shared_ptr` too, the two nodes would point at each other, both counts would stay at 1 forever, and neither would ever be destroyed. **Reference counting cannot free cycles.** That is exactly what `weak_ptr` is for.

### 19.7 Garbage collection (for completeness)

Languages such as Java, Go, Python and JavaScript reclaim memory **automatically**:

| Technique | How | Pros / cons |
|---|---|---|
| **Reference counting** | Free an object when its count reaches 0 (CPython, Swift ARC, `shared_ptr`) | Immediate and predictable, but **cannot free cycles** (needs a backup cycle collector) |
| **Mark and sweep** | Starting from the **roots** (globals, stacks, registers), **mark** every reachable object, then **sweep** away the rest | Handles cycles, but pauses the program; the heap stays fragmented |
| **Copying / compacting** | Copy live objects into a fresh area, which compacts them | No fragmentation and fast allocation, but needs 2× memory |
| **Generational** | Most objects die young: collect the young generation often and the old one rarely | Standard in the JVM and .NET |

**Why C firmware does not use GC:** collection **pauses** are unpredictable (bad for real time), it costs extra memory and CPU, and C pointers cannot be tracked precisely. **MicroPython** on the ESP32 does use a **mark-and-sweep** collector. That is why calling `gc.collect()` at quiet moments helps avoid pauses at bad ones.

---

## 20. Linux Memory Management for Embedded Linux

For roles involving Raspberry Pi, i.MX, STM32MP1, Yocto/Buildroot, BSPs or device drivers.

### 20.1 The big picture

```
 User space:   malloc/free (glibc)  ──► brk / mmap system calls
 ────────────────────────────────────────────────────────────────────
 Kernel:       VMAs (per-process regions) + page tables + page-fault handler
                          │
               kmalloc ──► SLUB (object caches: kmalloc-64, task_struct, ...)
               vmalloc ──► maps scattered pages into a contiguous virtual range
                          │
               Buddy allocator (pages, orders 0..10) ◄── CMA (big contiguous DMA buffers)
                          │
               Zones: ZONE_DMA / ZONE_DMA32 / ZONE_NORMAL / ZONE_HIGHMEM (32-bit only)
                          │
               Physical RAM (every 4 KB frame has a 'struct page')
```

- **Zones** exist because some hardware can address only part of RAM: old ISA DMA only the first 16 MB (`ZONE_DMA`), 32-bit DMA devices only the first 4 GB (`ZONE_DMA32`). On 32-bit systems, RAM above about 896 MB is not permanently mapped into the kernel (`ZONE_HIGHMEM`).
- The **buddy allocator** hands out 2^order contiguous pages (Chapter 5.4). `cat /proc/buddyinfo` shows the free blocks of each order, which is a direct view of **fragmentation**.
- **SLUB** carves pages into same-sized objects (Chapter 5.5). `cat /proc/slabinfo` or `slabtop` shows the caches.

### 20.2 Kernel allocation APIs

| API | Gives you | Contiguous physically? | Can sleep? | Use for |
|---|---|---|---|---|
| `kmalloc(size, gfp)` / `kzalloc` / `kcalloc` | Small objects from slab caches | **Yes** | Only with `GFP_KERNEL` | Most driver structures and buffers (up to a few MB, typically 4 MB) |
| `vmalloc(size)` / `vzalloc` | Large buffers | **No** (only virtually) | **Yes** | Big buffers used only by the CPU (firmware images, large tables) |
| `kvmalloc(size, gfp)` | Tries `kmalloc`, falls back to `vmalloc` | Maybe | Yes | Large allocations that do not need physical contiguity |
| `alloc_pages(gfp, order)` / `__get_free_pages` | 2^order whole pages | Yes | Depends on gfp | Page-level needs |
| `kmem_cache_create` / `kmem_cache_alloc` | Your own slab cache of fixed-size objects | Yes | Depends on gfp | Many objects of one type (network buffers, descriptors) |
| `devm_kzalloc(dev, size, gfp)` | Like `kzalloc`, **freed automatically** when the driver detaches | Yes | Depends on gfp | Driver `probe()` functions: no clean-up code needed |
| `dma_alloc_coherent(dev, size, &handle, gfp)` | A DMA buffer plus the **bus address** the device uses | Yes (from the device's view) | With `GFP_KERNEL` | Buffers shared with a device for a long time (descriptor rings) |
| `ioremap(phys, size)` | Kernel virtual address for **device registers** | — | — | Memory-mapped I/O (use `readl`/`writel`) |

**`kmalloc` vs `vmalloc` (a classic question):**

| | `kmalloc` | `vmalloc` |
|---|---|---|
| Physical memory | **Contiguous** | **Scattered pages** |
| Virtual memory | Contiguous (direct mapping) | Contiguous (special vmalloc area) |
| Speed | Fast | Slower: builds page tables; TLB pressure |
| Maximum size | Limited (typically 4 MB) | Large (limited by the vmalloc area) |
| Usable for DMA | **Yes** (with the DMA API) | **No** (a device needs physically contiguous memory, unless there is an IOMMU) |
| From interrupt context | Yes, with `GFP_ATOMIC` | **No** (may sleep) |

### 20.3 GFP flags: where and how you may allocate

| Flag | Meaning | Use in |
|---|---|---|
| `GFP_KERNEL` | Normal allocation. **May sleep** (to reclaim or write back memory) | Process context: `probe`, `open`, `ioctl`, kernel threads |
| `GFP_ATOMIC` | **Never sleeps**. May dip into emergency reserves. Fails more easily | **Interrupt handlers, softirqs, tasklets, code holding a spinlock** |
| `GFP_NOWAIT` | Never sleeps, does not use the reserves | Opportunistic allocations |
| `GFP_NOIO` / `GFP_NOFS` | May sleep, but must not start I/O / filesystem calls | Block and filesystem code (prevents deadlock by recursion) |
| `GFP_DMA` / `GFP_DMA32` | Allocate from the DMA zones | Devices with addressing limits (prefer the DMA API) |
| `__GFP_ZERO` | Return zeroed memory | `kzalloc` = `kmalloc` + `__GFP_ZERO` |

> **Interview tip.** "Can you call `kmalloc(GFP_KERNEL)` in an interrupt handler?" **No.** It may sleep, and sleeping in atomic context is a bug ("BUG: scheduling while atomic"). Use `GFP_ATOMIC`, or better, allocate in advance.

### 20.4 A typical driver probe

```c
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define BUF_SIZE    4096
#define REG_CTRL    0x00
#define REG_DMA_ADR 0x04

struct mydev {
    void __iomem *regs;        /* __iomem: device memory, use readl()/writel() only */
    void         *dma_cpu;     /* CPU (virtual) address of the DMA buffer            */
    dma_addr_t    dma_bus;     /* address the DEVICE uses for the same buffer        */
};

static int mydev_probe(struct platform_device *pdev)
{
    struct mydev *md;

    md = devm_kzalloc(&pdev->dev, sizeof(*md), GFP_KERNEL);      /* auto-freed on remove   */
    if (!md)
        return -ENOMEM;

    md->regs = devm_platform_ioremap_resource(pdev, 0);          /* registers from the DT  */
    if (IS_ERR(md->regs))
        return PTR_ERR(md->regs);

    md->dma_cpu = dmam_alloc_coherent(&pdev->dev, BUF_SIZE, &md->dma_bus, GFP_KERNEL);
    if (!md->dma_cpu)
        return -ENOMEM;

    writel(lower_32_bits(md->dma_bus), md->regs + REG_DMA_ADR);  /* device gets BUS address */
    writel(0x1, md->regs + REG_CTRL);                            /* start the device        */

    platform_set_drvdata(pdev, md);
    return 0;
}
```

**Why `readl`/`writel` and not plain pointer access?** They guarantee the correct access width, prevent the compiler from merging or reordering the accesses, include the barriers the architecture needs for device memory, and handle endianness. They are the kernel's version of `volatile` register access (Chapter 8).

### 20.5 DMA in Linux

| | Coherent (consistent) DMA | Streaming DMA |
|---|---|---|
| API | `dma_alloc_coherent()` | `dma_map_single(dev, ptr, size, dir)` / `dma_unmap_single()` |
| Cache handling | Memory is uncached, or coherent by hardware: no maintenance needed | The kernel cleans/invalidates caches at map/unmap time |
| Typical use | Long-lived descriptor rings and control structures | One-off transfers of existing buffers (network packets, block I/O) |
| Direction | Both | `DMA_TO_DEVICE`, `DMA_FROM_DEVICE`, `DMA_BIDIRECTIONAL` |

- While a streaming buffer is mapped, the **device owns it**. To let the CPU look at it in between, call `dma_sync_single_for_cpu()` and then hand it back with `dma_sync_single_for_device()`. This is exactly the clean/invalidate dance from Chapter 12.12, done for you.
- Always check `dma_mapping_error()` after mapping.
- An **IOMMU** (SMMU on ARM) is an MMU **for devices**: it translates device addresses and blocks devices from accessing memory they were not given.
- If a device cannot reach a buffer's physical address, **swiotlb bounce buffers** copy the data through low memory, which is slow but works.

### 20.6 The user/kernel boundary: `copy_from_user`

A kernel driver **must never dereference a user-space pointer directly**:

1. The pointer may be **invalid or malicious** (for example, pointing into kernel memory).
2. The page may be **swapped out**, so the access faults and must be handled.
3. Hardware features (**SMAP** on x86, **PAN** on ARM) **forbid** the kernel from touching user memory except through special accessors.

```c
static ssize_t mydev_write(struct file *f, const char __user *ubuf, size_t len, loff_t *off)
{
    char kbuf[64];
    size_t n = min(len, sizeof(kbuf) - 1);

    if (copy_from_user(kbuf, ubuf, n))     /* returns the number of bytes NOT copied */
        return -EFAULT;
    kbuf[n] = '\0';
    /* ... parse kbuf ... */
    return n;
}
```

The family: `copy_to_user()`, `copy_from_user()`, and `get_user()`/`put_user()` for single values. The `__user` annotation lets the `sparse` checker flag direct dereferences.

### 20.7 Kernel stacks are tiny

Each thread's kernel stack is only **8 KB** (32-bit ARM) or **16 KB** (x86-64, ARM64). Therefore: **no large local arrays** and **no recursion** in kernel code. Allocate big buffers with `kmalloc`. Modern kernels place stacks in vmalloc space with **guard pages** (`CONFIG_VMAP_STACK`), so an overflow faults instead of silently corrupting memory.

### 20.8 Reading memory statistics

**Per process:**

| Metric | Meaning |
|---|---|
| **VSZ / VIRT** | Total **virtual** memory mapped (includes untouched heap, libraries, reservations). Often huge and mostly meaningless |
| **RSS / RES** | Physical pages **currently in RAM** for this process, **including shared libraries** (counted fully in every process using them) |
| **PSS** | Proportional set size: shared pages **divided** among the processes sharing them. The fairest "real" usage figure (`/proc/<pid>/smaps_rollup`) |
| **USS** | Pages **only** this process uses. What you would get back by killing it |

**System-wide (`/proc/meminfo`, `free -h`):**

- `MemFree`: completely unused RAM.
- `Cached` / `Buffers`: the **page cache** (file data kept in RAM). It is **reclaimable**: Linux uses "free" RAM as cache because unused RAM is wasted RAM.
- **`MemAvailable`**: the estimate of memory available for new programs **without swapping**. **This** is the number to watch, not `MemFree`.
- `Slab`: kernel object caches (`SReclaimable` is the part that can be freed).

### 20.9 Overcommit and the OOM killer

- `malloc(1 GB)` usually **succeeds** even on a 512 MB board, because the kernel only reserves virtual space. Physical pages are assigned on **first touch** (demand paging).
- `vm.overcommit_memory`: **0** = heuristic (the default), **1** = always allow, **2** = strict (commit limit = swap + RAM × `overcommit_ratio`%). Embedded systems without swap often choose **2**, so that `malloc` fails cleanly instead of the OOM killer firing later.
- When RAM truly runs out, the **OOM killer** chooses a victim by `oom_score` (mostly memory usage) and kills it. Protect critical daemons with `echo -1000 > /proc/<pid>/oom_score_adj`. Mark expendable processes with a high value.

### 20.10 Real-time Linux and memory

Page faults are disastrous for latency, because a major fault can take milliseconds. Real-time applications (with `PREEMPT_RT`) therefore:

```c
#include <sys/mman.h>

mlockall(MCL_CURRENT | MCL_FUTURE);   /* lock every current and future page in RAM: no paging */
/* then "pre-fault": touch the whole stack and all buffers once at start-up, so every page is
   already resident before the real-time loop starts; avoid malloc inside the RT loop */
```

### 20.11 Embedded Linux memory tips

- **No swap on flash** (wear). Use **zram** (compressed RAM swap) if you need more headroom.
- **CMA (Contiguous Memory Allocator):** reserves a region at boot that the kernel lends out for movable pages, then reclaims it when a driver (camera, display, video codec) needs a **large physically contiguous** buffer. Configured with `cma=64M` on the kernel command line or in the device tree.
- **`reserved-memory`** device-tree nodes (with `no-map`) keep RAM away from Linux entirely, for example memory shared with a Cortex-M co-processor (**remoteproc/RPMsg** on STM32MP1 and i.MX).
- **`/dev/mem`** and **UIO** map device registers into user space for quick prototyping (needs root, gives no protection: use a real driver for production).

---

## 21. Formula Sheet and Numericals

### 21.1 Formula sheet

**Paging**

| Quantity | Formula |
|---|---|
| Offset bits | log₂(page size) |
| Page-number bits | virtual address bits − offset bits |
| Number of pages | 2^(page-number bits) = virtual space / page size |
| Number of frames | physical memory / frame size |
| Frame-number bits | log₂(number of frames) |
| Page table size (flat) | number of pages × PTE size |
| Entries per page-table page | page size / PTE size |
| Bits resolved per level | log₂(entries per page-table page) |
| Number of levels | ⌈(virtual bits − offset bits) / bits per level⌉ |
| Inverted page table entries | number of physical frames |
| Average internal fragmentation | page size / 2 per process (or per segment) |
| EAT with TLB (n-level table) | h × (t_TLB + t_mem) + (1 − h) × (t_TLB + (n + 1) × t_mem) |
| EAT with page faults | (1 − p) × t_mem + p × t_fault_service |

**Cache**

| Quantity | Formula |
|---|---|
| Number of lines | cache size / line size |
| Number of sets | lines / associativity |
| Offset bits | log₂(line size) |
| Index bits | log₂(number of sets) |
| Tag bits | address bits − index bits − offset bits |
| Tag storage | lines × (tag + valid + dirty + replacement bits) |
| AMAT | hit time + miss rate × miss penalty |
| Two-level AMAT | HT₁ + MR₁ × (HT₂ + MR₂ × MP₂) |

**Allocation**

| Quantity | Formula |
|---|---|
| Proportional frame allocation | aᵢ = (sᵢ / S) × m |
| Buddy block size | 2^⌈log₂(request)⌉ |
| Buddy internal fragmentation | block size − request |
| Buddy address | offset XOR block size |
| Knuth's 50% rule | N allocated blocks → about 0.5 N blocks lost (one third of memory) |

### 21.2 Solved numericals

**Q1.** A system has a 32-bit virtual address, 4 KB pages and 4-byte PTEs. How big is a flat page table?
**A.** Offset = 12 bits, page number = 20 bits → 2²⁰ entries × 4 B = **4 MB** per process.

**Q2.** Physical memory is 1 GB and pages are 4 KB. How many frames, and how many bits for the frame number?
**A.** 2³⁰ / 2¹² = 2¹⁸ = **262,144 frames** → **18 bits**. With valid, dirty, referenced and protection bits, the PTE rounds up to **4 bytes**.

**Q3.** A logical address space has 64 pages of 1024 bytes, mapped onto 32 frames. How many bits are in the logical and the physical address?
**A.** Logical: 64 = 2⁶ pages, 1024 = 2¹⁰ offset → **16 bits**. Physical: 32 = 2⁵ frames → 5 + 10 = **15 bits**.

**Q4.** Page size 1 KB, page table: page 0 → frame 5, 1 → 2, 2 → 7, 3 → 1. Translate logical address **2100**.
**A.** Page = ⌊2100 / 1024⌋ = 2, offset = 2100 − 2048 = 52. Page 2 → frame 7 → physical = 7 × 1024 + 52 = **7220**.

**Q5.** A 32-bit address with 4 KB pages and 4-byte PTEs uses two-level paging. How are the bits split?
**A.** One page holds 4096 / 4 = 1024 = 2¹⁰ PTEs → **10 bits (outer) | 10 bits (inner) | 12 bits (offset)**.

**Q6.** How many levels for a 48-bit virtual address, 4 KB pages, 8-byte PTEs?
**A.** 4096 / 8 = 512 = 2⁹ entries per table → 9 bits per level. (48 − 12) / 9 = **4 levels**.

**Q7.** TLB lookup 10 ns, memory access 100 ns, TLB hit ratio 90%, **two-level** page table. EAT?
**A.** Hit: 10 + 100 = 110 ns. Miss: 10 + 2 × 100 (walk) + 100 (data) = 310 ns. EAT = 0.9 × 110 + 0.1 × 310 = 99 + 31 = **130 ns**.

**Q8.** TLB 20 ns, memory 100 ns, single-level table. What hit ratio keeps EAT ≤ 125 ns?
**A.** EAT = 120h + 220(1 − h) = 220 − 100h ≤ 125 → **h ≥ 0.95**.

**Q9.** Memory access 100 ns, page-fault service 10 ms, fault rate p = 10⁻⁶. EAT?
**A.** EAT = (1 − 10⁻⁶) × 100 + 10⁻⁶ × 10⁷ ≈ 100 + 10 = **≈ 110 ns**: one fault in a million accesses costs 10% performance.

**Q10.** 16 KB direct-mapped cache, 32-byte lines, 32-bit addresses. Tag, index and offset?
**A.** Lines = 16384 / 32 = 512. Offset = 5, index = 9, tag = 32 − 9 − 5 = **18 bits**.

**Q11.** 64 KB, 2-way set-associative cache, 64-byte lines, 32-bit addresses.
**A.** Lines = 1024, sets = 512. Offset = 6, index = 9, tag = 32 − 9 − 6 = **17 bits**.

**Q12.** L1 hit 2 cycles, L1 miss rate 10%, L2 hit 15 cycles, L2 local miss rate 5%, memory 200 cycles. AMAT?
**A.** 2 + 0.1 × (15 + 0.05 × 200) = 2 + 0.1 × 25 = **4.5 cycles**.

**Q13.** Buddy system with a 1 MB block. Requests: A = 100 KB, then B = 240 KB. Show the allocation.
**A.** A → 128 KB (internal fragmentation 28 KB): 1 MB → 512 + 512 → 256 + 256 → 128 + 128, A gets the first 128 KB at offset 0. B → 256 KB: it takes the free 256 KB buddy at offset 256 KB (internal fragmentation 16 KB). Free afterwards: 128 KB at 128 KB and 512 KB at 512 KB.

**Q14.** 62 frames shared by P1 (10 pages) and P2 (127 pages) with proportional allocation.
**A.** P1 = 10/137 × 62 ≈ **4**, P2 = 127/137 × 62 ≈ **57**.

**Q15.** Reference string `7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1` with 3 frames. Page faults for FIFO, LRU and OPT?
**A.** FIFO **15**, LRU **12**, OPT **9** (Chapter 11.9 shows the simulator).

---

## 22. Interview Questions and Answers

Short, precise answers. The chapter references point to the detailed explanation.

### 22.1 Fundamentals and program layout

**1. What are the segments of a C program in memory?**
`.text` (code), `.rodata` (constants, string literals), `.data` (initialised globals/statics), `.bss` (zero/uninitialised globals/statics), heap (dynamic), stack (locals, return addresses). See Chapter 2.

**2. Why does `.bss` not take space in flash?**
It is all zeros, so the image stores only its size. Start-up code zero-fills it in RAM.

**3. Where do a `static` local variable and a `const` global live?**
A `static` local lives in `.data` (or `.bss` if zero) and keeps its value between calls. A `const` global lives in `.rodata`, which is **flash** on an MCU.

**4. Difference between `char s[] = "hi";` and `char *s = "hi";`?**
The first is a modifiable array on the stack (or in `.data`) initialised with a copy. The second is a pointer to a **read-only** literal in `.rodata`: writing through it is undefined behaviour.

**5. What does the start-up code do before `main`?**
Sets up the stack pointer (from the vector table on Cortex-M), copies `.data` from flash to RAM, zeroes `.bss`, initialises clocks/FPU, runs C++ constructors, then calls `main`.

**6. What is the difference between LMA and VMA?**
LMA (load address) is where a section is **stored** in the image. VMA (virtual/run address) is where it **lives while running**. For `.data`: LMA in flash, VMA in RAM.

**7. Little vs big endian? How do you test it?**
Little-endian stores the least significant byte at the lowest address (ARM, x86, RISC-V). Test: `uint16_t x = 1; return *(uint8_t *)&x == 1;`. See Chapter 1.7.

**8. Harvard vs von Neumann?**
Von Neumann: one bus and one memory for code and data. Harvard: separate buses, so instruction fetch and data access can happen in parallel. Most MCUs are "modified Harvard": separate buses, one address space.

**9. What is a linker script for?**
It describes the memory regions (FLASH, RAM) and places each output section into them, defines symbols such as `_sdata`/`_ebss`/`_estack`, and reserves heap and stack space.

**10. How do you see how much RAM and flash your firmware uses?**
`arm-none-eabi-size firmware.elf` (flash = text + data, static RAM = data + bss), the linker **map file**, `nm --size-sort`, or `idf.py size-components` on ESP32.

### 22.2 Stack and heap

**11. Stack vs heap?**
Stack: automatic, LIFO, very fast, deterministic, limited size, freed on return. Heap: manual, flexible size and lifetime, slower, can fragment and leak. See the table in Chapter 3.12.

**12. Which way does the stack grow, and how do you check?**
Usually downward (ARM, x86, RISC-V). Compare the address of a local in a caller with one in a `noinline` callee (Chapter 3.8).

**13. What is in a stack frame?**
Return address (or the saved link register), saved callee-saved registers, optionally the frame pointer, local variables, spilled temporaries, and arguments that did not fit in registers.

**14. What causes stack overflow, and how do you detect it on an MCU?**
Deep recursion, large local arrays, `printf`, nested interrupts. Detect with stack painting/high-water marks, canaries (FreeRTOS method 2), MPU guard regions, stack-limit registers (ARMv8-M), and `-fstack-usage` analysis.

**15. What is the difference between `malloc` and `calloc`?**
`calloc(n, size)` zero-initialises memory and checks `n × size` for overflow. `malloc(bytes)` leaves the contents indeterminate.

**16. What happens if `realloc` fails? What is wrong with `p = realloc(p, n);`?**
It returns `NULL` and the **old block remains valid**. Assigning directly to `p` loses the only pointer to the old block: a leak. Use a temporary.

**17. How does `free` know how much to free?**
The allocator stores a **header** with the block size just before the pointer it returned.

**18. What does `malloc(0)` return?**
Implementation-defined: `NULL` or a unique pointer that must not be dereferenced but must be passed to `free`.

**19. Is `free(NULL)` safe?**
Yes, it does nothing.

**20. Why is `malloc` discouraged in embedded systems?**
Non-deterministic timing, fragmentation over long run times, failure handling at run time, not ISR-safe, hidden overhead. MISRA C forbids it. Use static allocation, pools, or allocate only at initialisation.

**21. Can you call `malloc` inside an ISR?**
No. It is not reentrant, its time is unbounded, and it may use locks. Use pre-allocated pools.

**22. What is a memory leak, and how do you find one in firmware?**
Allocated memory that is never freed and no longer referenced. Find it with heap monitoring (free heap trending down), wrappers that track allocations (Chapter 19.4), ESP-IDF heap tracing, or Valgrind/ASan on host builds.

### 22.3 Allocators and fragmentation

**23. Internal vs external fragmentation?**
Internal: wasted space **inside** allocated blocks (rounding up, fixed-size blocks, last page). External: free memory split into **holes** between blocks, none big enough even though the total is.

**24. How do you reduce external fragmentation?**
Coalescing free blocks, fixed-size pools, allocating only at start-up, paging (on systems with an MMU), size-segregated allocators (TLSF, slab), compaction (not possible in C).

**25. First fit vs best fit vs worst fit?**
First fit takes the first hole that fits (fast). Best fit takes the smallest that fits (least waste, leaves tiny slivers). Worst fit takes the largest (generally the worst). First fit and best fit beat worst fit in practice.

**26. How would you implement `malloc`/`free`?**
Header (size, free flag, next) in front of each block, a list of blocks, first fit with splitting, coalescing of neighbouring free blocks on `free`, alignment rounding. Chapter 5.1 has the full code.

**27. What is a memory pool, and why is it good for real time?**
Pre-allocated, equally sized blocks kept in a free list. Allocation and freeing are O(1) pops/pushes, with no external fragmentation and deterministic timing.

**28. Explain the buddy allocator.**
Power-of-two blocks. Allocation splits blocks in half until the size fits. Freeing merges a block with its buddy (address XOR size) whenever the buddy is free. It is fast, but has internal fragmentation. Linux uses it for pages.

**29. What is the slab allocator?**
Caches of pre-sized objects carved from pages, one cache per object type. Fast, with no fragmentation between types. Linux `kmalloc` is built on it (SLUB).

**30. Write `aligned_malloc`.**
Allocate `size + align − 1 + sizeof(void *)`, round the address up to the boundary, store the original pointer just before the aligned address, and have `aligned_free` read it back. Chapter 5.2.

### 22.4 Pointers, alignment and `volatile`

**31. Dangling vs wild vs NULL pointer?**
Dangling: points to freed or out-of-scope memory. Wild: never initialised. NULL: deliberately points to nothing. All three crash or corrupt memory when dereferenced.

**32. How do you change a caller's pointer inside a function?**
Pass a pointer to the pointer (`int **pp`) and assign `*pp = malloc(...)`, or return the new pointer.

**33. `memcpy` vs `memmove`?**
`memmove` handles overlapping regions (it copies backwards when the destination lies above the source). `memcpy` must not be used for overlapping regions. Chapter 6.1.

**34. Why is `sizeof(struct)` larger than the sum of its members?**
Padding: every member is aligned to its natural boundary, and the struct size is rounded up to its largest member alignment so that arrays stay aligned.

**35. How do you minimise padding?**
Order members from largest alignment to smallest. Use packing only when you need an exact wire layout.

**36. What are the dangers of `__attribute__((packed))`?**
Misaligned members cause slow code or faults on CPUs without unaligned access. Taking the address of a packed member creates misaligned pointers. The result is still not portable for endianness.

**37. What does `volatile` do?**
It forces the compiler to perform every read and write of the object exactly as written, without caching it in a register or removing accesses. Use it for hardware registers, ISR-shared flags, and memory changed by DMA.

**38. Does `volatile` make code thread-safe or atomic?**
No. `x++` is still read-modify-write. It does not order non-volatile accesses and gives no cross-core guarantees. Use critical sections, mutexes or atomics.

**39. Can a variable be both `const` and `volatile`?**
Yes. Example: a read-only status register. The program must not write it, but the hardware can change it.

**40. How do you access a register at address `0x40021000`?**
`#define REG (*(volatile uint32_t *)0x40021000u)` and then `REG |= (1u << 3);`, or a CMSIS-style struct overlay. Chapter 8.2.

### 22.5 Virtual memory, paging, TLB

**41. What is virtual memory, and why use it?**
An abstraction that gives each process its own address space, translated to physical memory by the MMU. Benefits: isolation and protection, programs larger than RAM, easy sharing, demand loading, simpler programming.

**42. Paging vs segmentation?**
Paging uses fixed-size pages, is invisible to the programmer, and has internal fragmentation. Segmentation uses variable-size logical units with (segment, offset) addresses and has external fragmentation.

**43. How is a virtual address translated?**
Split it into page number and offset. Look the page number up in the TLB. On a miss, walk the page table to get the frame. Physical address = frame × page size + offset. An invalid entry raises a page fault.

**44. What is the TLB?**
A small, fully or highly associative cache of recent page-number-to-frame translations inside the MMU. It avoids a memory access (or several, with multi-level tables) for each translation.

**45. What happens to the TLB on a context switch?**
It is flushed, unless entries are tagged with an ASID/PCID, which lets entries from several processes coexist. Kernel entries can be marked global.

**46. Why multi-level page tables?**
A flat table is huge (4 MB per process for 32-bit, impossible for 64-bit). Multi-level tables allocate inner tables only for the regions actually used.

**47. What is a page fault? Walk through it.**
A trap raised when the PTE is invalid. The OS checks that the address is legal (if not, SIGSEGV), finds a free frame (or evicts a victim, writing it back if dirty), reads the page from disk, updates the PTE, and restarts the instruction.

**48. Minor vs major page fault?**
Minor: the page is already in RAM (page cache, shared), so only the PTE is updated. Major: the page must be read from storage.

**49. What is demand paging?**
Loading pages only when they are first accessed, instead of loading the whole program at start.

**50. What is copy-on-write?**
After `fork`, parent and child share pages marked read-only. The first write to a page causes a fault, and the kernel copies just that page. Untouched pages are never copied.

**51. What does `mmap` do?**
It maps a file or anonymous memory into the address space. File reads become memory reads served through page faults and the page cache. It is used for large `malloc` blocks, shared libraries, shared memory and device registers.

**52. What is an inverted page table?**
One entry per physical frame, recording which (process, page) occupies it. It saves memory but needs a hashed search, and it makes shared memory awkward.

### 22.6 Page replacement and thrashing

**53. Name the page replacement algorithms.**
FIFO, Optimal (OPT), LRU, LRU approximations (reference bits/aging, second chance/Clock, enhanced second chance), LFU, MFU, NRU.

**54. What is Belady's anomaly?**
More frames can cause **more** faults with some algorithms (FIFO, Clock). Example: `1 2 3 4 1 2 5 1 2 3 4 5` with FIFO gives 9 faults with 3 frames and 10 with 4 frames. Stack algorithms (LRU, OPT) are immune.

**55. Why is OPT not used in practice?**
It needs knowledge of future references. It serves only as a benchmark.

**56. How is LRU approximated in real OSes?**
With the hardware reference bit: the Clock/second-chance algorithm, aging counters, or the (R, M) classes of enhanced second chance.

**57. What is thrashing, and how do you prevent it?**
The system spends more time paging than executing, because processes have fewer frames than their working sets. Prevent it with the working-set model (suspend processes when total demand exceeds memory), page-fault-frequency control, more RAM, or fewer processes.

**58. What is the working set?**
The set of distinct pages a process referenced in the last Δ references. It approximates the current locality.

### 22.7 Cache

**59. Why do caches work?**
Temporal and spatial locality of reference.

**60. Direct-mapped vs set-associative vs fully associative?**
Direct-mapped: one possible line (fast, conflict misses). N-way: N lines per set (the usual compromise). Fully associative: any line (no conflict misses, expensive, used for TLBs).

**61. How do you split an address into tag, index and offset?**
Offset = log₂(line size), index = log₂(number of sets), tag = the remaining bits. Chapter 12.3.

**62. Write-through vs write-back?**
Write-through updates memory on every write (simple, consistent, more traffic). Write-back updates only the cache and marks the line dirty, writing it back on eviction (less traffic, memory can be stale).

**63. What are the 3 C's of cache misses?**
Compulsory (first access), capacity (working set too large), conflict (too many blocks mapped to one set). The 4th C is coherence.

**64. What is cache coherence? Explain MESI.**
Keeping copies of the same line in different cores' caches consistent. MESI states: Modified (dirty, only copy), Exclusive (clean, only copy), Shared (clean, possibly several copies), Invalid. A write to a Shared line invalidates the other copies.

**65. What is false sharing?**
Independent variables written by different cores sit in the same cache line, so the line ping-pongs between cores. Fix it by aligning or padding each core's data to the line size.

**66. Why does DMA need cache maintenance, and what do you do?**
DMA bypasses the CPU cache. For TX, **clean** (write back) the buffer before starting DMA. For RX, **invalidate** before and after the transfer. Alternatively, put the buffers in non-cacheable memory (MPU) or in TCM. Buffers should be aligned to and sized in whole cache lines.

### 22.8 MMU and MPU

**67. MMU vs MPU?**
An MMU translates virtual to physical addresses and protects pages (page tables in RAM, TLB). An MPU only protects a few fixed regions with no translation: cheap and deterministic, for MCUs.

**68. Why don't Cortex-M parts have an MMU?**
Determinism (TLB misses and page walks), silicon cost and power, small RAM, and no need for virtual memory in single-image firmware.

**69. What are the ARMv7-M MPU region rules?**
8 (or 16) regions. The size is a power of two from 32 B to 4 GB, the base must be aligned to the size, there are 8 subregions for regions of 256 B or more, and the highest-numbered region wins on overlap. The background map applies to privileged code if `PRIVDEFENA` is set.

**70. How is the ARMv8-M MPU different?**
Base and limit with 32-byte granularity, attributes taken from MAIR registers, overlapping regions not allowed (an access matching two regions faults), no subregions, separate Secure/Non-secure MPUs, plus stack-limit registers.

**71. Name practical uses of an MPU.**
Stack guards, NULL-pointer trapping, read-only code, execute-never RAM, task isolation (FreeRTOS-MPU), non-cacheable DMA regions, device memory attributes.

**72. What happens on an MPU violation?**
A MemManage fault (or HardFault if MemManage is disabled). `CFSR` (MMFSR bits) gives the cause and `MMFAR` the address when `MMARVALID` is set.

**73. What is RISC-V PMP?**
Physical Memory Protection: up to 16/64 entries (TOR/NA4/NAPOT address matching, R/W/X permissions, lock bit), checked for S/U-mode accesses and for M-mode only when locked. The lowest-numbered match wins. It is RISC-V's MPU equivalent (used on ESP32-C3/C6).

**74. Why do you need `DSB` and `ISB` after enabling the MPU?**
`DSB` makes sure the register writes have completed. `ISB` flushes the pipeline so the following instructions execute with the new memory map.

### 22.9 RTOS, FreeRTOS and ESP32

**75. Compare FreeRTOS heap_1 to heap_5.**
1: allocate only (no free), deterministic. 2: best fit without coalescing (legacy). 3: wraps libc `malloc` with the scheduler suspended. 4: first fit with coalescing (the default choice). 5: heap_4 spread across multiple memory regions.

**76. Static vs dynamic creation of RTOS objects?**
Static (`xTaskCreateStatic`) uses buffers you declare: it cannot fail at run time and all RAM is visible at link time. Dynamic (`xTaskCreate`) uses `pvPortMalloc`: flexible, but it can fail and fragment.

**77. How do you detect task stack overflow in FreeRTOS?**
`configCHECK_FOR_STACK_OVERFLOW` 1 (SP check at context switch) or 2 (also checks the 0xA5 fill pattern at the stack end), the `vApplicationStackOverflowHook`, `uxTaskGetStackHighWaterMark()`, or MPU guard regions.

**78. In what units is the stack size given to `xTaskCreate`?**
Words in vanilla FreeRTOS (256 means 1 KB on 32-bit). **Bytes in ESP-IDF.**

**79. Is `pvPortMalloc` thread-safe? ISR-safe?**
Thread-safe (it suspends the scheduler). Not ISR-safe: never call it from an interrupt.

**80. How do you pass large data between tasks efficiently?**
Send a **pointer** to a buffer from a pool through the queue (by reference), with clear ownership: the consumer returns the buffer to the pool when done. Do not copy large items by value.

**81. What is IRAM vs DRAM on the ESP32?**
The same SRAM accessed through the instruction bus (IRAM, for code) or the data bus (DRAM, for data). On the classic ESP32, IRAM supports only 32-bit word access.

**82. Why must some ESP32 ISRs be marked `IRAM_ATTR`?**
During flash writes or erases, the flash cache is disabled. Code or `const` data in flash cannot be accessed then, so ISRs that must run must be in IRAM, with their data in DRAM (`DRAM_ATTR`). Otherwise: "Cache disabled but cached memory region accessed".

**83. How do you allocate a DMA buffer on the ESP32?**
`heap_caps_malloc(size, MALLOC_CAP_DMA)` or `DMA_ATTR` on a static buffer. It must be in internal RAM.

**84. How do you keep a variable across deep sleep on the ESP32?**
`RTC_DATA_ATTR` (RTC memory). Use `RTC_NOINIT_ATTR` to also survive software resets without being re-initialised.

### 22.10 Barriers, DMA and flash

**85. What is a memory barrier?**
An instruction (or compiler directive) that prevents memory operations from being reordered across it. A compiler barrier only stops compiler reordering. Hardware barriers (DMB/DSB/ISB, `fence`) stop CPU reordering.

**86. DMB vs DSB vs ISB?**
DMB orders memory accesses. DSB waits for them to complete. ISB flushes the pipeline so the following instructions see a new context (MPU, VTOR, CONTROL, new code).

**87. Acquire vs release semantics?**
A release store makes all earlier writes visible before the store. An acquire load stops later accesses from moving before it. Together they safely publish data (as in the SPSC ring buffer in Chapter 17.5).

**88. How does flash differ from RAM?**
Erased state is all 1s, programming can only clear bits, erasing is by sector and slow, endurance is limited, and a bank cannot be read while it is being erased or written.

**89. What is wear leveling?**
Spreading erase/write cycles across all blocks so no block wears out early. It is done by the FTL in managed flash, or by flash-aware file systems (LittleFS, NVS) on raw flash.

**90. How do you emulate EEPROM in flash?**
Two pages with log-structured records: append `{id, value, CRC}` records, read the newest one, copy the live values to the other page when full and then erase the old page, with status headers for power-loss safety.

### 22.11 Linux

**91. `kmalloc` vs `vmalloc`?**
`kmalloc` gives physically contiguous memory: fast, size-limited, usable for DMA. `vmalloc` is only virtually contiguous: large, slower, not for DMA, cannot be used in atomic context.

**92. `GFP_KERNEL` vs `GFP_ATOMIC`?**
`GFP_KERNEL` may sleep and is only for process context. `GFP_ATOMIC` never sleeps and is for interrupt context or when holding a spinlock.

**93. Why `copy_from_user` instead of dereferencing the pointer?**
User pointers may be invalid, malicious or not resident, and SMAP/PAN forbid direct access. `copy_from_user` validates the pointer, handles faults, and returns the number of bytes it could not copy.

**94. RSS vs VSZ vs PSS?**
VSZ: total virtual memory mapped. RSS: resident pages, with shared pages counted in full. PSS: resident pages with shared pages divided among their users (the fairest figure).

**95. Why does `malloc` succeed but the program get killed later?**
Overcommit: virtual memory is promised, and physical pages are only assigned on first touch. If RAM runs out then, the OOM killer kills a process.

**96. How do real-time Linux applications avoid page faults?**
`mlockall(MCL_CURRENT | MCL_FUTURE)`, pre-faulting stacks and buffers at start-up, and no allocation inside the real-time loop.

### 22.12 Coding questions to practise

| Question | Where |
|---|---|
| Detect endianness; swap bytes | 1.7 |
| Reset handler: copy `.data`, zero `.bss` | 2.4 |
| Stack growth direction; stack painting | 3.6, 3.8 |
| `_sbrk` for newlib | 4.8 |
| `malloc`/`free`/`realloc`/`calloc` from scratch | 5.1 |
| `aligned_malloc`/`aligned_free` | 5.2 |
| Fixed-size memory pool (+ ISR-safe version) | 5.3 |
| Buddy allocator | 5.4 |
| `memcpy`, `memmove`, `memset`, `memcmp`, `strcpy`, `strlcpy` | 6.1 |
| Dynamic 2D arrays | 6.4 |
| `sizeof(struct)` puzzles; `offsetof`; `container_of` | 7.3, 7.5 |
| `ALIGN_UP`/`ALIGN_DOWN`; is power of two | 7.6 |
| Register access macros; set/clear/toggle bits | 8.2, 8.6 |
| Page-table address translation | 9.6 |
| FIFO/LRU/OPT/Clock page-fault simulator | 11.9 |
| LRU cache (C and C++), LFU cache, Clock cache | 13 |
| MPU configuration; MemManage handler | 14.6, 14.7 |
| Lock-free SPSC ring buffer | 17.5 |
| Leak detector wrapper | 19.4 |

---

## 23. Last Minute Revision Sheet

**Program layout**
- `.text` (code) and `.rodata` (const) → flash. `.data` → stored in flash, copied to RAM. `.bss` → RAM, zeroed. Then the heap grows up and the stack grows down.
- Flash = text + data. Static RAM = data + bss.

**Stack**
- LIFO, very fast, deterministic, fixed size. Each task has its own stack. ISRs use the MSP on Cortex-M.
- Overflow is silent on an MCU → painting, canaries, MPU guard, `PSPLIM`/`MSPLIM`.
- Never return the address of a local. Avoid recursion and VLAs in firmware.

**Heap**
- `malloc` (garbage contents), `calloc` (zeroed, overflow check), `realloc` (use a temporary pointer), `free` (`free(NULL)` is OK).
- Header before each block → that is how `free` knows the size.
- First, next, best and worst fit. Split on allocation, coalesce on free.
- Internal fragmentation (inside blocks) vs external fragmentation (holes between blocks).
- Embedded: static allocation, pools, allocate only at init; MISRA 21.3.

**Allocators**
- Pool: O(1), no external fragmentation. Buddy: powers of two, buddy = offset XOR size. Slab: per-type object caches. TLSF: O(1) general-purpose allocation.
- `aligned_malloc`: over-allocate by `align − 1 + sizeof(void *)`, round up, store the raw pointer at `[-1]`.

**Alignment and `volatile`**
- Member offset is a multiple of its alignment, struct size a multiple of the largest alignment. Sort members largest first.
- Packed = misaligned. Use `memcpy` for unaligned or aliased data.
- `volatile` = do not optimise the accesses. **Not** atomic, **not** a barrier.
- Write-1-to-clear registers: assign the bit, never use `|=`. Use set/reset registers (BSRR) for atomic GPIO.

**Paging**
- Address = page number | offset. PA = frame × page size + offset.
- 32-bit, 4 KB pages, 4 B PTE → 2²⁰ entries → 4 MB table → multi-level tables (10/10/12). x86-64 and Sv48: 4 levels × 9 bits.
- PTE bits: valid, R/W/X, U/S, accessed, dirty, cache.
- TLB: EAT = h(t_TLB + t_m) + (1 − h)(t_TLB + (n + 1) t_m). ASIDs avoid flushes.
- Page fault: trap → check legal → frame (evict, write back if dirty) → read → update PTE → restart.
- EAT with faults = (1 − p) t_m + p × t_fault. p must be under about 10⁻⁶.
- COW for `fork`, demand paging, `mmap`, shared pages, NX, ASLR, guard pages.

**Replacement**
- FIFO (Belady's anomaly), OPT (benchmark), LRU (stack algorithm, needs hardware support), Clock (reference bit), enhanced (R, M), LFU/MFU.
- Silberschatz string with 3 frames: FIFO 15, LRU 12, OPT 9. Belady: FIFO 9 → 10 when going from 3 to 4 frames.
- Thrashing → working set, page-fault frequency.

**Cache**
- Offset = log₂(line), index = log₂(sets), tag = the rest. AMAT = HT + MR × MP.
- Direct-mapped, N-way, fully associative. 3 C's + coherence. Write-back + write-allocate is the norm.
- MESI. False sharing → pad to 64 B.
- DMA: **clean** before TX, **invalidate** before and after RX. Align buffers to cache lines, or use non-cacheable memory or TCM.

**LRU/LFU coding**
- LRU = hash map + doubly linked list (MRU at the front, evict from the back), O(1).
- LFU = key → (value, freq, position) + freq → list + `min_freq`, O(1). Tie → LRU within the frequency.
- Clock = circular array + reference bit.

**MMU/MPU/PMP**
- MMU: translate + protect, page tables, TLB, Linux. MPU: protect only, 8-16 regions, deterministic, RTOS.
- ARMv7-M: power-of-two size, base aligned to size, 32 B minimum, subregions, highest region wins, AP/XN/TEX-C-B. ARMv8-M: base/limit, 32 B granularity, MAIR, no overlaps.
- Enable the MPU, then `DSB` + `ISB`. Violations → MemManage (CFSR/MMFAR).
- PMP: TOR/NA4/NAPOT, lowest entry wins, the L bit locks it and applies it to M-mode.

**FreeRTOS / ESP32**
- heap_1 (no free), 2 (best fit, no merge), 3 (libc), 4 (first fit + merge), 5 (multiple regions).
- Static APIs + `vApplicationGetIdleTaskMemory`. Malloc-failed and stack-overflow hooks. High-water marks.
- Stack depth: **words** in vanilla FreeRTOS, **bytes** in ESP-IDF. Never `malloc` in an ISR.
- ESP32: IRAM/DRAM are the same SRAM through different buses. Flash is XIP via cache. `IRAM_ATTR`/`DRAM_ATTR` for code and data used while the cache is off. `heap_caps_malloc(MALLOC_CAP_DMA / SPIRAM / INTERNAL)`. `RTC_DATA_ATTR` survives deep sleep.

**Ordering, flash, Linux**
- Compiler barrier vs DMB/DSB/ISB vs `fence`. Acquire/release for publishing data. SPSC ring buffer needs no lock.
- Flash: erased = 0xFF, writes 1 → 0 only, erase by sector, limited endurance → wear leveling, EEPROM emulation, A/B updates.
- Linux: `kmalloc` (contiguous, DMA-able) vs `vmalloc`. `GFP_ATOMIC` in interrupts. `copy_from_user`. `dma_alloc_coherent` vs streaming DMA. `MemAvailable`, PSS, overcommit, OOM killer, `mlockall`.

---

*End of guide. To practise, re-type each "Write it yourself" program from memory, compile it with `-Wall -Wextra -fsanitize=address,undefined`, and explain every line out loud as you would in an interview.*
