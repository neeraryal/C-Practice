# Bit Manipulation — Interview Quick Revision (Hinglish)

**Focus:** C / Embedded-Firmware interview | **Level:** Basic → Intermediate (selected advanced)  
**Revision strategy:** Pehle **Sections 1–8** revise karo. Time mile toh **Sections 9–12**. Ye notes tumhare practice solutions ko correct + organize karke banaye gaye hain; end mein sirf high-value missing topics add kiye hain.

> **Important:** Neeche 32-bit `uint32_t` assume kiya hai. `#include <stdint.h>` aur `#include <stdbool.h>` use karo. Bit positions **LSB se 0-indexed** hain (valid: 0–31).

## 1. Bitwise operators — 30-second recall

| Operator | Kaam | Yaad rakhne ka rule |
|---|---|---|
| `&` AND | Particular bits select / clear | `x & 0 = 0`, `x & 1 = x` |
| `\|` OR | Bits set | `x \| 1 = 1`, `x \| 0 = x` |
| `^` XOR | Bits toggle / duplicates cancel | `x ^ 1 = ~x` (single bit), `a ^ a = 0` |
| `~` NOT | Bits invert | Mask complement banane mein useful |
| `<<` | Left shift | Unsigned value ko bit positions left move karo |
| `>>` | Right shift | Unsigned value ko bit positions right move karo |

**C safety:** `1U << 31` valid hai agar `unsigned int` 32-bit hai; `1 << 31` signed `int` ki wajah se UB ho sakta hai. `uint32_t` ke liye `UINT32_C(1)` use karna clear option hai. **Shift count >= operand ki width ⇒ undefined behavior (UB)**, unsigned hone par bhi.

## 2. Set, clear, toggle, check — MUST

```c
#include <stdint.h>
#include <stdbool.h>

// Nth bit ko 1 karo; baaki bits unchanged.
uint32_t set_bit(uint32_t reg, unsigned int n)
{
    return reg | (UINT32_C(1) << n);   // OR: 0->1, 1->1
}

// Nth bit ko 0 karo; baaki unchanged.
uint32_t clear_bit(uint32_t reg, unsigned int n)
{
    return reg & ~(UINT32_C(1) << n);  // Mask mein Nth bit 0, baaki 1
}

// Nth bit flip karo: 0->1 aur 1->0.
uint32_t toggle_bit(uint32_t reg, unsigned int n)
{
    return reg ^ (UINT32_C(1) << n);   // XOR with 1 = toggle
}

// Nth bit set hai ya nahi? (Missing must-revise question)
bool is_bit_set(uint32_t reg, unsigned int n)
{
    return (reg & (UINT32_C(1) << n)) != 0U;
}

// Invalid Nth position ko safely handle karne ka example.
uint32_t safe_set_bit(uint32_t reg, unsigned int n)
{
    if (n >= 32U) return reg;          // UB avoid; error policy requirement par depend karegi
    return reg | (UINT32_C(1) << n);
}
```

**Quick example:** `reg = 0x20`, `n = 3` → `set_bit = 0x28` → `clear_bit(0x28, 3) = 0x20`.

## 3. Mask banana + field extract — MUST

**Intuition:** Pehle `width` consecutive 1s banao, phir `start_bit` par shift karo.

```c
// Example: width = 4, start_bit = 4
// (1U << 4) - 1U  = 0000 1111  (0x0F)
// << 4             = 1111 0000  (0xF0)

// General formula (sirf 1 <= width < 32, start_bit + width <= 32 ke liye):
uint32_t mask = ((UINT32_C(1) << width) - 1U) << start_bit;
```

**Bits [7:4] extract:**

```c
uint32_t extract_status(uint32_t reg)
{
    // Option 1: pehle mask, phir right shift
    return (reg & 0xF0U) >> 4;

    // Equivalent option 2: (reg >> 4) & 0x0FU;
}
```

`reg = 0x0000ACB5` → `reg & 0xF0 = 0xB0` → `>> 4 = 0xB`.

**Mistake fixed:** Tumhare original extract code mein parameter `reg` tha, lekin expression mein `num` likha tha. **`width == 32` par `(1U << width)` mat karna — UB.** Us case mein `UINT32_MAX` mask use karo.

## 4. Multiple bits update / register field write — MUST

Question: bits **[10:8]** mein 3-bit `mode` store karna hai, baaki bits unchanged.

```c
uint32_t set_mode(uint32_t reg, uint32_t mode)
{
    uint32_t mask = 0x7U << 8;     // 3 ones ko positions 10,9,8 par rakho
    reg &= ~mask;                  // Step 1: purani field CLEAR
    reg |= (mode & 0x7U) << 8;     // Step 2: nayi field SET; mode ko 3 bits tak limit
    return reg;
}

// Same logic, one-liner:
uint32_t set_mode_one_line(uint32_t reg, uint32_t mode)
{
    return (reg & ~(0x7U << 8)) | ((mode & 0x7U) << 8);
}
```

**Why clear + OR?** Sirf OR existing `1` ko `0` nahi bana sakta. Pehle field zero karo, phir desired bits set karo.

**Interview trap:** `mode = 0xFF` aaye toh bina `(mode & 0x7)` ke adjacent bits bhi change ho sakti hain. **Masking** invalid value truncate karta hai; **validation** (`if (mode > 7)`) invalid input reject karta hai. Hardware requirement ke hisaab se choose karo.

## 5. Count set bits / popcount — MUST

```c
// Approach 1: Har iteration mein LSB check, phir shift.
int count_set_bits_shift(uint32_t num)
{
    int count = 0;
    while (num != 0U) {
        count += (int)(num & 1U);   // LSB 1 hai toh count++
        num >>= 1;                  // Agla bit LSB par lao
    }
    return count;
}

// Approach 2: Brian Kernighan — har iteration mein lowest set bit clear.
int count_set_bits_fast(uint32_t num)
{
    int count = 0;
    while (num != 0U) {
        num &= (num - 1U);          // Rightmost 1 remove
        count++;
    }
    return count;
}
```

- Approach 1: maximum **32 iterations** (positive input par significant-bit count).
- Approach 2: exactly **k iterations**, jahan `k` = number of set bits; sparse inputs ke liye useful.
- Space dono ka **O(1)**; fixed 32-bit input ke liye worst-case time **O(1)**.

## 6. Power of two + lowest set bit — MUST

```c
bool is_power_of_two(uint32_t num)
{
    // Powers of 2 mein exactly ek set bit hoti hai.
    // num = 0 ka special case zaroor check karo.
    return (num != 0U) && ((num & (num - 1U)) == 0U);
}

uint32_t isolate_lowest_set_bit(uint32_t num)
{
    // Example: 0010 1000 -> 0000 1000
    return num & ~(num - 1U);
}

// Equivalent unsigned trick: num & (-num)
```

**Remember:**
- `num & (num - 1)` → **lowest set bit remove**.
- `num & ~(num - 1)` → **lowest set bit isolate**.
- `0U - 1U` (32-bit unsigned arithmetic) → `0xFFFFFFFF` **well-defined wraparound**. Signed overflow is UB.

## 7. Reverse all 32 bits — MUST

**Approach A — simple, fixed 32 iterations (interview mein pehle ye explain karo):**

```c
uint32_t reverse_bits_loop(uint32_t num)
{
    uint32_t res = 0;
    for (unsigned int i = 0; i < 32U; i++) {
        res = (res << 1) | (num & 1U);  // LSB ko result ke right end mein jodo
        num >>= 1;
    }
    return res;
}
```

**Approach B — tumhara `while(num)` + remaining left shift:**

```c
uint32_t reverse_bits_while(uint32_t num)
{
    uint32_t res = 0;
    unsigned int count = 0;

    while (num != 0U) {
        res = (res << 1) | (num & 1U);
        num >>= 1;
        count++;
    }
    if (count != 0U)
        res <<= (32U - count);      // Original leading zeros ko output ke end mein lao
    return res;                     // num = 0 ho toh shift by 32 se bachega
}
```

**Approach C — masks se parallel groups swap (advanced, already practised):**

```c
uint32_t reverse_bits_masks(uint32_t num)
{
    // 1-bit pair swap: odd -> right, even -> left
    num = ((num & 0xAAAAAAAAU) >> 1)  | ((num & 0x55555555U) << 1);
    // 2-bit groups swap
    num = ((num & 0xCCCCCCCCU) >> 2)  | ((num & 0x33333333U) << 2);
    // 4-bit groups (nibbles) swap
    num = ((num & 0xF0F0F0F0U) >> 4)  | ((num & 0x0F0F0F0FU) << 4);
    // 8-bit groups (bytes) swap
    num = ((num & 0xFF00FF00U) >> 8)  | ((num & 0x00FF00FFU) << 8);
    // 16-bit halves swap (mask ki bhi zarurat nahi)
    num = (num >> 16) | (num << 16);
    return num;
}
```

**Intuition:** Jo upper group hai woh right move karega, lower group left; mask se sirf wahi bits pick karo; OR se merge. Group size: **1 → 2 → 4 → 8 → 16**. `0x0000000D` reverse → `0xB0000000`.

**Complexity:** 32-bit input ke liye sab **O(1) time, O(1) space**. Mask method mein **5 fixed stages**, par actual speed architecture/compiler par depend karegi.

## 8. XOR — one unique number — MUST

```c
// Har duplicate exactly 2 baar, ek element sirf ek baar.
uint32_t find_unique(const uint32_t *arr, int n)
{
    uint32_t res = 0U;
    for (int i = 0; i < n; i++)
        res ^= arr[i];              // x ^ x = 0; x ^ 0 = x
    return res;
}
```

Example: `4 ^ 1 ^ 2 ^ 1 ^ 2 = 4`. **O(n) time, O(1) space**.

---

# Extra MUST-Revise Topics (tumhari pasted list mein nahi the)

## 9. Two unique numbers — LeetCode 260 (high-value XOR follow-up)

```c
// Har duplicate exactly twice; do distinct values ek-ek baar.
void find_two_unique(const uint32_t *arr, int n,
                     uint32_t *a, uint32_t *b)
{
    uint32_t x = 0U;
    for (int i = 0; i < n; i++)
        x ^= arr[i];                 // x = unique_a ^ unique_b

    uint32_t mask = x & ~(x - 1U);  // Koi ek differing set bit select
    *a = 0U;
    *b = 0U;

    for (int i = 0; i < n; i++) {
        if ((arr[i] & mask) != 0U)
            *a ^= arr[i];            // Group 1
        else
            *b ^= arr[i];            // Group 2
    }
}
```

**Intuition:** `a ^ b` ki jis position par 1 hai, us bit par `a` aur `b` different hain. Us bit se grouping karo; equal duplicates same group mein cancel honge. **Rightmost bit compulsory nahi**, XOR result ki koi bhi set bit chalegi. **O(n), O(1)**.

## 10. Single Number II — LeetCode 137 (quick recall; advanced `ones/twos` skip kar sakte ho)

**Condition:** Har duplicate **3 baar**, ek number **1 baar**. Simple XOR fail: `x ^ x ^ x = x`.

**Logic:** Har bit position (0…31) par set bits count karo. `count % 3` unique number ki bit dega. **32 × n = O(n)** (fixed bit width).

```c
// 32-bit int platform / LeetCode setting.
int single_number_ii(const int *arr, int n)
{
    uint32_t bits = 0U;

    for (unsigned int bit = 0; bit < 32U; bit++) {
        int count = 0;
        for (int j = 0; j < n; j++) {
            count += (int)(((uint32_t)arr[j] >> bit) & 1U);
        }
        if ((count % 3) != 0)
            bits |= (UINT32_C(1) << bit);
    }

    // Signed result ko safely reconstruct karo (32-bit signed-int range).
    if ((bits & 0x80000000U) != 0U)
        return -1 - (int)(~bits);
    return (int)bits;
}
```

`arr[j]` ko unsigned mein cast karne se negative input ka bitwise right shift portable hota hai. 32-bit `int` assumed.

## 11. Endianness + nibble/byte swapping (Embedded interview MUST)

**Endian ka matlab:** Multi-byte number ke bytes **memory mein kis order mein store** hote hain.

`uint32_t x = 0x12345678` (increasing memory addresses):

- **Little-endian:** `78 56 34 12` (least-significant byte pehle).
- **Big-endian:** `12 34 56 78` (most-significant byte pehle).

**Important:** Register value ki `>>` / `<<` operations endianness se nahi badalti; memory-byte interpretation / serialization mein difference aata hai.

```c
// 8-bit number ke upper/lower nibbles interchange:
uint8_t swap_nibbles(uint8_t x)
{
    return (uint8_t)(((x & 0xF0U) >> 4) | ((x & 0x0FU) << 4));
}

// 32-bit number mein adjacent bytes swap:
// 0xAABBCCDD -> 0xBBAADDCC
uint32_t swap_adjacent_bytes(uint32_t x)
{
    return ((x & 0xFF00FF00U) >> 8) | ((x & 0x00FF00FFU) << 8);
}

// Upper/lower 16-bit halves swap:
// 0xAABBCCDD -> 0xCCDDAABB
uint32_t swap_halves(uint32_t x)
{
    return (x >> 16) | (x << 16);
}
```

**Interview trap:** Byte swapping aur *bit reversal* same cheez nahi hain.

## 12. Real hardware registers: `volatile`, read-modify-write, W1C (MUST CONCEPT)

```c
// Illustrative memory-mapped peripheral register (address example only).
volatile uint32_t * const CTRL = (volatile uint32_t *)0x40000000U;

// Normal read-modify-write register ho toh:
// *CTRL |= (1U << 3);
```

- **`volatile` kyun?** Register hardware / ISR ke through change ho sakta hai. Compiler ko access optimize karke remove/cache nahi karna chahiye. **`volatile` atomicity ya thread safety guarantee nahi karta.**
- **Read-modify-write (`|=`, `&=`) kab risky?** ISR / second core same register update kare toh lost updates ho sakte hain. Hardware ke **SET/CLEAR alias registers** ho toh unhe prefer karo; otherwise synchronization requirements samjho.
- **W1C (write-one-to-clear) register:** Status bit ko `1` write karne se clear hoti hai. Aise register par blindly `reg |= mask` / generic read-modify-write use mat karo — doosre pending status flags accidentally clear ho sakte hain. **Always datasheet ke register semantics check karo.**

---

## 13. One-screen cheat sheet — last-minute revision

| Task | Expression (valid `n` assumed) |
|---|---|
| Set Nth bit | `x \| (UINT32_C(1) << n)` |
| Clear Nth bit | `x & ~(UINT32_C(1) << n)` |
| Toggle Nth bit | `x ^ (UINT32_C(1) << n)` |
| Check Nth bit | `(x & (UINT32_C(1) << n)) != 0U` |
| Clear lowest set bit | `x & (x - 1U)` |
| Isolate lowest set bit | `x & ~(x - 1U)` (or `x & -x` unsigned) |
| Power of two | `x != 0U && !(x & (x - 1U))` |
| Lowest `width` bits = 1 | `(1U << width) - 1U` (**width < 32**) |
| Extract bits [7:4] | `(x >> 4) & 0xFU` |
| Update bits [10:8] | `(x & ~(7U << 8)) \| ((mode & 7U) << 8)` |
| One unique among pairs | XOR every element |
| Two unique among pairs | XOR all → differing bit mask → XOR two groups |
| One unique among triples | Per-bit count `% 3` |

### C-specific traps jo interviewer pooch sakta hai

1. **`1 << 31` vs `1U << 31`:** 32-bit signed `int` case mein pehla UB ho sakta hai; unsigned use karo.
2. **`1U << 32`:** **UB**; extra bit silently discard hone ki guarantee nahi.
3. **Unsigned `0U - 1U`:** modulo wrap-around, **well-defined** (32-bit unsigned mein `0xFFFFFFFF`).
4. **Signed overflow:** UB; negative signed right shift C17/earlier mein implementation-defined. Bit manipulation mein `uint32_t` safer.
5. **`x << 4` vs `x <<= 4`:** pehla variable update **nahi** karta; doosra karta hai.
6. **Bit field write:** Pehle clear, phir OR; input field width se bada ho toh mask **ya** validate.
7. **`volatile` ≠ atomic**; hardware flags ka behavior datasheet se verify karo.
8. **Complexity:** `32 × n` = **O(n)** for fixed 32-bit words; har nested loop O(n²) nahi hota.

**Final recommendation:** Monday se pehle Sections **2–8 + 11–13** ek baar confidently explain karke revise kar lo. LeetCode 260 ka grouping intuition yaad rakho; `ones/twos` state-machine trick low priority rakho. Uske baad C pointers, memory, RTOS, Linux aur firmware-debugging par shift karo.
