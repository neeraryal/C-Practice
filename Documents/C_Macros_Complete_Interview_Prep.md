# C Macros (`#define`) — Complete Interview Preparation (Hinglish)

> **Target:** Embedded C / Firmware / Qualcomm-style coding interview  
> **Goal:** Sirf syntax ratna nahi; macro **kaise expand hota hai**, code mein **kab use karna hai**, aur **bugs kaise avoid karne hain** — sab samajhna.  
> **Reading order:** Basics → coding patterns → tricky bugs → embedded use cases → practice → **5-minute quick interview prep (bilkul last mein)**.

---

## 1. Macro kya hota hai? Preprocessor ka role

Macro ko `#define` se define karte hain. C source code compilation se pehle **preprocessor** macro references ko unke replacement tokens se expand karta hai. Matlab macro koi runtime function ya variable nahi hota — preprocessor code ko transform karta hai, aur phir compiler transformed C code compile karta hai.

```c
#define BUFFER_SIZE 128
#define PI 3.14159

int buffer[BUFFER_SIZE];
double area = PI * 5 * 5;
```

Preprocessing ke baad compiler ko effectively yeh dikhega:

```c
int buffer[128];
double area = 3.14159 * 5 * 5;
```

**Interview answer (English mein bol sakte ho):** "A macro is a preprocessor definition that performs token replacement before C compilation. Function-like macros expand inline in source code, but they don't provide function-style type checking or single evaluation of arguments."

**Syntax dhyan do:**

```c
#define LIMIT 100       // Correct
#define LIMIT = 100     // Galat: replacement mein '=' bhi aa jayega
```

**Important confusion clear:** Macro ki *definition* mein `=` nahi chahiye, lekin **macro body** mein normal C assignment (`=`, `|=`, `&=`, etc.) bilkul use kar sakte ho.

```c
#define RESET(x) ((x) = 0)
#define SET_BIT(r, p) ((r) |= (1U << (p)))
```

### Macro ke types

**Object-like macro:** Constant/configuration replacement.

```c
#define MAX_RETRIES 3
#define UART_BAUD_RATE 115200
```

**Function-like macro:** Arguments leta hai, lekin actual function call nahi hota.

```c
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
```

**Syntax trap:** `#define SQUARE (x) ...` mein `SQUARE` aur `(` ke beech space hai, toh yeh **function-like** macro nahi banega. Function-like macro ke liye naam ke immediately baad `(` hona zaroori hai: `#define SQUARE(x) ...`.

---

## 2. Parentheses kyu zaroori hain? Macro expansion ka real problem

Macro ke parameters code mein replace hote hain. Operator precedence ki wajah se bina parentheses unexpected answer aa sakta hai.

```c
#define SQUARE(x) x * x
int result = SQUARE(2 + 3);
```

Expansion:

```c
int result = 2 + 3 * 2 + 3;  // 11, not 25
```

Correct:

```c
#define SQUARE(x) ((x) * (x))
// SQUARE(2 + 3) -> ((2 + 3) * (2 + 3)) -> 25
```

**Do levels par parentheses lagao:** (1) har argument use ke around `(x)`, (2) poore expression ke around `(...)`.

```c
// Incomplete protection:
#define MAX_BAD(a, b) (a) > (b) ? (a) : (b)

int ans = 2 * MAX_BAD(10, 20);
// Expands: 2 * (10) > (20) ? (10) : (20)
// Returns 20, but intended 40.

// Correct:
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
```

**Interview rule:** "I parenthesize macro parameters and the complete expression because macro expansion can change the operator-precedence context."

---

## 3. Macro arguments multiple times evaluate ho sakte hain (side effects)

Yeh **macro vs function ka sabse important difference** hai. Macro replacement mein parameter jitni baar use hoga, expression utni baar evaluate ho sakta hai.

```c
#define DOUBLE(x) ((x) + (x))

int a = 5;
int result = DOUBLE(a++);
```

Expansion:

```c
int result = ((a++) + (a++));
```

Yahan same variable `a` ko `+` ke operands mein unsequenced modify kiya ja raha hai, isliye **undefined behavior (UB)** hai. Iska koi guaranteed output nahi hota.

Ek doosra example:

```c
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int x = 10, y = 20;
int m = MAX(x++, y++);
```

Expanded expression mein comparison ke time dono arguments evaluate honge, phir selected argument **dobara** evaluate hoga. `x++` aur `y++` yahan different variables ko modify karte hain; is specific example ko automatically UB mat bolo, but increments unexpected honge. Isi liye side-effect arguments (`i++`, `--j`, assignment, state-changing calls) macros mein avoid karo.

### Safer alternative: `static inline`

```c
static inline int double_value(int x)
{
    return x + x;
}

int b = 5;
int ans = double_value(b++);  // ans = 10, b = 6
```

Function argument `b++` **once** evaluate hota hai. `inline` compiler ko inline expansion ki possibility batata hai, **guarantee nahi** deta. `static inline` header mein small helper functions ke liye common option hai.

> **Interview mein:** Macro argument ko multiple baar use karna risky hai. Type safety / reliable argument evaluation chahiye toh function ya `static inline` function prefer karo.

---

## 4. Macro vs Function vs `static inline` — when to use what?

| Point | Macro | Regular function | `static inline` function |
|---|---|---|---|
| Kaun process karta hai? | Preprocessor | Compiler | Compiler |
| Function-style parameter type checking? | Nahi | Haan | Haan |
| Argument expression evaluate | Multiple baar ho sakta hai | Har supplied argument expression ek baar | Har supplied argument expression ek baar |
| Call overhead | Macro ka function call nahi | Ho sakta hai | Compiler eliminate kar sakta hai |
| Type flexibility | Different types par same replacement kaam kar sakta hai | Declared types | Declared types |
| Debugging | Expansion ki wajah se relatively hard | Usually easier | Usually easier |

**Practical decision:**

- `#define`: build configuration, conditional compilation, generic token transformations, `__FILE__/__LINE__` logging, compile-time naming.
- `static inline`: small operations jahan type checking, predictable evaluation aur debugging important ho.
- Regular function: bigger behavior, reusable modules, stronger abstraction.

**C keyword trap:** `double` function ka naam nahi ho sakta, kyunki `double` reserved keyword hai. `double_value()` jaisa naam use karo.

---

## 5. Common coding macros — MAX, MIN, SWAP, ARRAY_SIZE

### 5.1 `MAX` / `MIN`

```c
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
```

```c
int a = MAX(10 + 5, 12 + 8); // 20
float b = MAX(3.5f, 2.5f);   // 3.5
```

Yeh integers aur floats ke expressions par work kar sakta hai, lekin arguments repeated evaluation ka risk rehta hai. Different signed/unsigned types mix karne par C usual arithmetic conversions ka dhyan rakho.

### 5.2 `SWAP` — multi-statement macro

```c
#define SWAP(a, b)         \
    do {                   \
        int swap_tmp = (a);\
        (a) = (b);         \
        (b) = swap_tmp;    \
    } while (0)

int x = 10, y = 20;
SWAP(x, y); // x = 20, y = 10
```

Yeh **int variables** ke liye hai, har type ke liye generic nahi. `a` aur `b` assignable lvalues hone chahiye. Temporary name ka caller ke variable naam se collision ho sakta hai, aur side-effecting arguments avoid karne chahiye. Production code mein type-specific function ya safer design prefer ho sakta hai.

**Why `do { } while (0)`?** Multi-statement macro ko ek C statement ki tarah use karne deta hai:

```c
if (ready)
    SWAP(x, y);
else
    puts("Not ready");
```

Agar raw multiple statements macro body mein rakhe, toh `if-else` mein syntax/logic errors aa sakte hain. `while (0)` sirf **once** run hota hai. Macro definition ke end mein semicolon mat lagao; caller `SWAP(x, y);` likhega.

### 5.3 `ARRAY_SIZE` — real array vs pointer

```c
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

int a[] = {10, 20, 30, 40, 50};
char b[] = {'A', 'B', 'C'};

printf("%zu\n", ARRAY_SIZE(a)); // 5
printf("%zu\n", ARRAY_SIZE(b)); // 3
```

`sizeof(a)` = poore array ka byte size; `sizeof(a[0])` = ek element ka byte size. Division se element count milta hai.

**Important limitation:** Pointer par apply mat karo:

```c
int numbers[10];
int *p = numbers;

ARRAY_SIZE(numbers); // 10
ARRAY_SIZE(p);       // WRONG for counting elements
```

64-bit machine par `sizeof(p)` often 8 aur `sizeof(p[0])` often 4 hota hai, toh galti se `2` aa sakta hai. Function parameters `int arr[]` bhi function ke andar actually pointer parameters ban jaate hain; wahan bhi array count available nahi hota. `%zu` `size_t` print karne ka format hai.

---

## 6. `do { } while (0)` vs normal expression macro

**Expression macro** tab jab caller ko value chahiye:

```c
#define IS_EVEN(x) (((x) % 2) == 0)
int ok = IS_EVEN(6); // 1
```

**Statement macro** tab jab multiple statements perform karne hon:

```c
#define PRINT_STATUS(code)           \
    do {                             \
        printf("Status: %d\n", (code)); \
        puts("Logged");             \
    } while (0)
```

`do-while(0)` **normal value-returning expression ka replacement nahi** hai. Standard C mein `do` statement khud value return nahi karta. `if-else` ka use macro mein allowed hai, lekin simple bit expression ke liye unnecessary ho sakta hai.

---

## 7. Variadic macros: `...` aur `__VA_ARGS__`

Jab arguments ki count fixed nahi hoti, variadic macro use karte hain. `...` variable arguments accept karta hai aur `__VA_ARGS__` un tokens ko macro body mein substitute karta hai. Yeh **runtime array/pack nahi**; preprocessor token replacement hai.

```c
#define LOG(...) printf(__VA_ARGS__)

LOG("Hello\n");
LOG("Temp=%d, Voltage=%d\n", 35, 220);
```

Second call effectively banta hai:

```c
printf("Temp=%d, Voltage=%d\n", 35, 220);
```

### Practical `DEBUG_LOG` macro

```c
#define DEBUG_LOG(...)            \
    do {                          \
        printf("[DEBUG] ");       \
        printf(__VA_ARGS__);      \
        printf("\n");            \
    } while (0)

DEBUG_LOG("Firmware started");
DEBUG_LOG("Sensor: %d", 25);
```

Output:

```text
[DEBUG] Firmware started
[DEBUG] Sensor: 25
```

**Interview note:** Yahan multiple `printf` calls hain; agar single `printf` requirement ho aur first argument string literal format ho, tab adjacent string-literal concatenation wala pattern bhi use kar sakte hain. Logging function reentrancy/thread safety alag design concern hai.

### Named macro parameter + `...`

```c
#define LOG_FORMAT(fmt, ...) printf(fmt, __VA_ARGS__)
LOG_FORMAT("Value=%d", 10);
// -> printf("Value=%d", 10);
```

Mapping: `fmt` ko pehla argument (`"Value=%d"`) milega, aur `__VA_ARGS__` ko remaining (`10`). Lekin `LOG_FORMAT("Hello")` ki tarah no variadic argument dene par comma handling ka issue aata hai; portable basic pattern ke liye all-variadic `LOG(...)` convenient hai. Optional argument solutions compiler/standard support par depend karte hain.

**Do not put a trailing semicolon inside macro definition:**

```c
#define BAD_LOG(...) printf(__VA_ARGS__); // bad for if/else
```

Caller semicolon lagayega, jis se extra null statement ban sakta hai.

---

## 8. Conditional compilation: `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#error`

Embedded firmware mein same source multiple board variants/configurations ke liye build hota hai. **Conditional compilation** unwanted code ko preprocessing ke time hi remove kar sakti hai.

### `#ifdef` vs `#if` — sabse common confusion

```c
#define DEBUG 0

#ifdef DEBUG
printf("A\n"); // Included: DEBUG defined hai
#endif

#if DEBUG
printf("B\n"); // Excluded: DEBUG ki value 0 hai
#endif
```

**Difference:** `#ifdef X` = "X defined hai?"; `#if X` = "X ka preprocessor value/expression true hai?"

### Debug enable/disable — practical release build

```c
#include <stdio.h>
#define DEBUG 1

#if DEBUG
    #define DEBUG_LOG(...)                     \
        do {                                   \
            printf("[DEBUG] ");                \
            printf(__VA_ARGS__);              \
            printf("\n");                     \
        } while (0)
#else
    #define DEBUG_LOG(...) ((void)0)
#endif

int main(void)
{
    int temp = 35;
    DEBUG_LOG("Temp=%d", temp);
}
```

`DEBUG = 0` par logging call expand hokar no-op ban jayega; normal `printf` execute nahi hoga aur debug-log arguments bhi evaluate nahi honge.

### Board configuration + compile-time error

```c
#define BOARD_A

#if defined(BOARD_A)
    #define UART_BAUD_RATE 9600
#elif defined(BOARD_B)
    #define UART_BAUD_RATE 115200
#else
    #error "No supported board selected"
#endif
```

`#elif def BOARD_B` **invalid** hai. Correct: `#elif defined(BOARD_B)`; `#ifdef` single standalone directive hota hai, `#elif` mein `defined(...)` use karo.

`#error` **compile/build stage par diagnostic** deta hai, runtime par nahi. Upar code assumes exactly one board selected; production build mein dono defined hon toh explicitly reject karna bhi useful hai:

```c
#if defined(BOARD_A) && defined(BOARD_B)
    #error "Select only one board"
#endif
```

### `#undef`

```c
#define MODE 1
#undef MODE
#define MODE 2
// Ab MODE -> 2
```

`#undef` macro ki existing definition remove karta hai.

---

## 9. Header guards — why and how?

Same `.h` file ek translation unit mein indirectly multiple baar include ho sakti hai. Guard ke bina typedef/struct definitions jaise declarations clash kar sakte hain.

**`sensor.h`:**

```c
#ifndef SENSOR_H
#define SENSOR_H

typedef struct {
    int temperature;
    int humidity;
} SensorData;

void sensor_init(void);
int sensor_read(void);

#endif // SENSOR_H
```

First inclusion par `SENSOR_H` defined nahi hoga → contents process honge → `SENSOR_H` defined ho jayega. Same translation unit mein next inclusion par contents skip ho jayenge.

**Nuance:** Header guard header ko **alag `.c` files** mein include hone se nahi rokta. It prevents repeated processing in the **same translation unit**. `#pragma once` bhi commonly used hai, lekin portable standard preprocessor guard `#ifndef/#define/#endif` hai.

---

## 10. Stringification (`#`) — parameter ka name print karna

Macro parameter ke immediately pehle `#` lagao toh argument **string literal** ban jata hai.

```c
#define PRINT_VAR(x) printf(#x " = %d\n", (x))

int temperature = 35;
PRINT_VAR(temperature);
```

Expansion:

```c
printf("temperature" " = %d\n", temperature);
```

C adjacent string literals ko concatenate kar deta hai, so output:

```text
temperature = 35
```

**Tumhari practice wali mistake:**

```c
#define PRINT_VAR_BAD(x) printf("#x = %d", (x))
```

`"#x"` already literal string hai; yahan `#` operator ki tarah kaam **nahi** karta. Sahi syntax `#x` quotes **ke bahar** hai.

### Advanced but useful: expansion + stringify (two levels)

```c
#define VERSION 42
#define STR(x) #x
#define XSTR(x) STR(x)

STR(VERSION)  // "VERSION"
XSTR(VERSION) // "42"
```

Reason: `#` ke directly saath used parameter ko stringify karte waqt normal prescan expansion nahi hota. Ek extra macro level pehle expansion karwa sakta hai. Is topic par sirf ek simple explanation enough hai.

---

## 11. Token pasting (`##`) — do tokens join karna

`##` left aur right preprocessing tokens ko combine karke **ek token** banane ki koshish karta hai.

```c
#define CONCAT(a, b) a##b

int sensor1 = 100;
printf("%d\n", CONCAT(sensor, 1));
// CONCAT(sensor, 1) -> sensor1
```

### Variable declaration wala interview question

```c
#define DECLARE_VAR(name, num) int name##num

DECLARE_VAR(sensor, 1); // -> int sensor1;
sensor1 = 100;
```

**Tumhari earlier mistake:** `name##num` se sirf `sensor1` name banta hai. **Declaration** chahiye toh `int` bhi add karna hoga.

**Caution:** `##` ka result valid preprocessing token hona chahiye. Yeh runtime strings ko concatenate karne ka operator nahi hai. Macro-expanded operands paste karne ke liye kabhi two-level pattern chahiye hota hai, similar to stringification.

---

## 12. Predefined macros: `__FILE__`, `__LINE__` (practical debugging)

`__FILE__` current file name/path ka string literal hai, aur `__LINE__` current source line ka integer number hai. Useful for error logs.

```c
#include <stdio.h>

#define ERROR_LOG(msg)                              \
    do {                                            \
        printf("[ERROR] %s:%d - %s\n",              \
               __FILE__, __LINE__, (msg));          \
    } while (0)

int main(void)
{
    ERROR_LOG("UART Timeout");
    ERROR_LOG("Sensor init failed");
    return 0;
}
```

Example output (line numbers depend on source):

```text
[ERROR] main.c:12 - UART Timeout
[ERROR] main.c:13 - Sensor init failed
```

**Interview constraint:** Agar "single `printf` call" bola hai toh five separate `printf`s ki jagah format specifiers ek call mein combine karo. `__LINE__` ke liye `%d`, `__FILE__` aur message ke liye `%s`.

---

## 13. Embedded C bit macros (short but complete)

Ye macro topic ka **sirf ek use case** hai, poori macro prep nahi. Registers handle karte waqt common patterns yaad rakho.

```c
#include <stdint.h>

#define SET_BIT(reg, pos)    ((reg) |=  (1U << (pos)))
#define CLEAR_BIT(reg, pos)  ((reg) &= ~(1U << (pos)))
#define TOGGLE_BIT(reg, pos) ((reg) ^=  (1U << (pos)))
#define CHECK_BIT(reg, pos) (!!((reg) & (1U << (pos))))

uint32_t reg = 0x0F;
SET_BIT(reg, 3);     // unchanged: bit 3 already set
CLEAR_BIT(reg, 1);   // 0x0D
TOGGLE_BIT(reg, 5);  // 0x2D = decimal 45
```

**Operators yaad rakho:** `|` set, `& ~mask` clear, `^` toggle, `& mask` check. `1U` ka `U` unsigned literal ko represent karta hai (especially bit 31 relevant). Assume 32-bit `uint32_t` and `pos` in **0..31**.

### Check bit mein `!!` ka logic

```c
!0  // 1
!8  // 0
!!8 // 1
```

`reg & mask` ka result set bit ke position ke hisaab se `8`, `32`, etc. aa sakta hai. `!!(...)` usko exactly **0 or 1** mein normalize kar deta hai.

Equivalent, shift-based check:

```c
#define CHECK_BIT_SHIFT(reg, pos) \
    (((reg) & (1U << (pos))) >> (pos))
```

Shift-based expression mein `pos` twice use hota hai; `!!` wale mein once. Register macros mein side-effecting arguments avoid karo.

### `WRITE_BIT` — without if/else

```c
#define WRITE_BIT(reg, pos, value) \
    ((reg) = (((reg) & ~(1U << (pos))) | (((value) & 1U) << (pos))))

uint32_t r = 0x0F;
WRITE_BIT(r, 2, 0); // 0x0B
WRITE_BIT(r, 5, 1); // 0x2B (43)
```

Logic: Pehle target bit clear, phir `value` ka lowest bit desired position par shift, phir OR, phir assign. `value` ko `0/1` assume kiya gaya hai; `& 1U` lowest bit limit karta hai.

### Consecutive bits clear / extract

```c
// count = 1..31, start + count <= 32
#define CLEAR_BITS(reg, start, count) \
    ((reg) &= ~(((1U << (count)) - 1U) << (start)))

#define GET_BITS(reg, start, count) \
    (((reg) >> (start)) & ((1U << (count)) - 1U))

uint32_t a = 0xFFFFFFFF;
CLEAR_BITS(a, 4, 4);       // a = 0xFFFFFF0F

uint32_t b = 0x00000AB0;
uint32_t field = GET_BITS(b, 4, 8); // 0xAB
```

**Important precedence trap:** `(1U << count) - 1U` hai; `1U << count - 1` C precedence ke according `1U << (count - 1)` ho jayega. **count = 32** ke liye `1U << 32` invalid shift hai; is pattern ko bina redesign kiye 32-bit full-width field par mat use karo.

### Hardware register caution

`reg |= mask` generally **read-modify-write** hai. Interrupt, another task, hardware-updated bits ya write-one-to-clear status registers mein naïve read-modify-write unsafe ho sakta hai. MCU-specific atomic set/clear registers, synchronization aur register specification use karo. `volatile` compiler accesses ko relevant rules ke under preserve karta hai, **atomicity guarantee nahi** karta.

---

## 14. Extra useful coding pattern: Power of two macro

```c
#define IS_POWER_OF_TWO(n) (((n) != 0U) && (((n) & ((n) - 1U)) == 0U))
```

Isko `uint32_t` jaisa **unsigned** `n` ke liye use karo. Powers of 2 (`1`, `2`, `4`, `8`, `16`, ...) mein exactly ek set bit hota hai. `n - 1` us bit ko clear karke lower bits set karta hai; AND ka result zero hota hai. `n = 0` ko explicitly reject karna zaroori hai.

```c
IS_POWER_OF_TWO(8U);  // 1
IS_POWER_OF_TWO(10U); // 0
IS_POWER_OF_TWO(0U);  // 0
```

**Limitation:** `n` macro mein multiple baar evaluate hota hai. `IS_POWER_OF_TWO(i++)` pass mat karo; real helper chahiye toh `static inline` function better hai.

---

## 15. Macro debugging: expansion actually kaise inspect karein?

Interviewer jab pooche "output kyu different aaya?", mentally macro expand karna seekho. Local GCC/Clang ke saath preprocessed code dekh sakte ho:

```bash
gcc -E file.c        # preprocessing output (headers bhi expand honge)
gcc -E -P file.c     # line markers hata kar output
```

**Debugging approach:**

1. Macro invocation find karo, jaise `MAX(2+3, 4)`.
2. Parameters ko **exactly** macro body mein substitute karo.
3. C operator precedence aur associativity apply karo.
4. Side effects / repeated evaluation check karo.
5. Decide karo code valid, incorrect output, compile error, ya undefined behavior hai.

**Three distinctions kabhi mix mat karna:**

- **Compile error:** Code invalid hai (`#elif def BOARD_B`, missing bracket, keyword-as-function-name).
- **Wrong but defined result:** `#define SQUARE(x) x*x` with `SQUARE(2+3)` gives `11`.
- **Undefined behavior:** `#define DOUBLE(x) ((x)+(x))` with `DOUBLE(i++)`.

---

## 16. Common Interview Mistakes — hamari practice se

| Mistake | Kyu wrong / risk? | Correct thought |
|---|---|---|
| `#define SET_BIT(r,p) (r) & (1 < p)` | `<` comparison hai, `&` register set nahi karta | `((r) |= (1U << (p)))` |
| `#define MAX(a,b) (a)>(b)?(a):(b)` | Entire expression parentheses missing | `(((a)>(b))?(a):(b))` |
| `#define PRINT_VAR(x) printf("#x...")` | `#` string ke andar literal hai | `printf(#x "...", (x))` |
| `#define DECLARE_VAR(a,b) a##b` | Name banega, declaration nahi | `int a##b` |
| `#elif def BOARD_B` | Invalid syntax | `#elif defined(BOARD_B)` |
| `DEBUG_LOG(...) printf(...);` | Macro ke andar `;` if/else break kar sakta hai | `do { ... } while (0)` |
| Many `printf`s when asked one | Requirement miss ho gayi | One format string, one `printf` |
| `ARRAY_SIZE(ptr)` | Pointer ko array samajh liya | Only real array expressions |
| `DOUBLE(a++)` | Same variable unsequenced modified | Use inline function |
| `static inline int double(int x)` | `double` C keyword hai | `double_value` |
| `1U << count - 1` | Means `1U << (count - 1)` | `(1U << count) - 1U` |

---

## 17. Independent coding practice (answers right below)

Pehle khud attempt karo; sirf uske baad solutions dekho. **Theory + coding dono** cover hote hain.

**Q1.** `CUBE(x)` macro likho with safe parentheses. `CUBE(1+2)` ka result?

**Q2.** Ek macro `PRINT_VAR(x)` likho jo `speed = 60` print kare (given `int speed=60`).

**Q3.** `DEBUG = 0` ho toh `DEBUG_LOG(...)` completely compile out kaise karoge?

**Q4.** `LOG("Value %d", 5)` for `#define LOG(...) printf(__VA_ARGS__)` expansion kya hoga?

**Q5.** `DECLARE_VAR(channel, 2)` se `int channel2;` kaise generate karoge?

**Q6.** `int nums[12]; int *ptr=nums;` — `ARRAY_SIZE(nums)` vs `ARRAY_SIZE(ptr)` ka difference?

**Q7.** `MAX(i++, j++)` risky kyu hai, aur safer alternative kya hai?

**Q8.** `#define FLAG 0`, `#ifdef FLAG`, `#if FLAG` — kaunsa code included hoga?

### Compact answer key

```c
// Q1
#define CUBE(x) ((x) * (x) * (x)) // CUBE(1+2) -> 27

// Q2
#define PRINT_VAR(x) printf(#x " = %d\n", (x))

// Q3
#if DEBUG
    #define DEBUG_LOG(...) do { printf(__VA_ARGS__); } while (0)
#else
    #define DEBUG_LOG(...) ((void)0)
#endif

// Q4: printf("Value %d", 5);

// Q5
#define DECLARE_VAR(name, num) int name##num

// Q6: 12 for nums, pointer formula does NOT give array count.
// Q7: repeated evaluation; prefer a function/static inline with suitable types.
// Q8: #ifdef FLAG included, #if FLAG excluded.
```

---

# 18. LAST 5 MINUTES — Quick Interview Prep (Read Just Before Interview)

> **Timing:** 0:00–1:00 core rules, 1:00–2:00 ready-to-write macros, 2:00–3:00 preprocessing features, 3:00–4:00 traps, 4:00–5:00 verbal Q&A.

### ⏱ 0:00–1:00 — 5 lines that must be clear

1. **Macro = preprocessor token replacement**, not normal runtime function.
2. **Syntax:** `#define NAME replacement` / `#define FUNC(x) replacement` (**no space** before `(` in function-like definition).
3. **Parentheses:** Arguments `(x)` and entire expression `(...)`.
4. **Side effects:** Same parameter repeated → `i++` evaluate multiple times; sometimes UB.
5. **Better tool:** `static inline` for type checking and single argument evaluation.

### ⏱ 1:00–2:00 — Write these without looking

```c
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define SWAP(a,b)            \
    do {                     \
        int t = (a);          \
        (a) = (b);            \
        (b) = t;              \
    } while (0)
```

`SWAP` above is int-only; avoid temp-name collision. `ARRAY_SIZE` pointer ke liye nahi.

### ⏱ 2:00–3:00 — Preprocessor essentials

```c
#ifdef BOARD_A        // defined hai?
#elif defined(BOARD_B) // another defined check
#else
#error "Invalid board"
#endif

#if DEBUG             // value nonzero hai?
#endif

#define STR(x) #x      // stringification
#define CAT(a,b) a##b  // token pasting
#define LOG(...) printf(__VA_ARGS__)

// Header guard:
#ifndef SENSOR_H
#define SENSOR_H
// header content
#endif
```

`__FILE__` = file string; `__LINE__` = line number. `#undef NAME` existing macro definition ko remove karta hai.

### ⏱ 3:00–4:00 — 6 traps, one glance

- `SQUARE(2+3)` without parentheses → `11`, not `25`.
- `2 * MAX_BAD(10,20)` without whole-expression parentheses → `20`, not `40`.
- `DOUBLE(i++)` with `((i++)+(i++))` → **undefined behavior**.
- `#ifdef DEBUG` is true even when `#define DEBUG 0`.
- `"#x"` is **not** stringification; `#x` outside quotes is.
- `ARRAY_SIZE(pointer)` **does not calculate element count**.

### ⏱ 4:00–5:00 — Interviewer ke 6 one-line answers

**Q: Macro vs function?**  
A: "Macro is expanded by preprocessor, has no function-parameter type checking and may evaluate arguments multiple times. A function evaluates each supplied argument expression once."

**Q: Why `do { } while (0)`?**  
A: "It makes multiple statements behave as one statement, especially inside `if-else`, and runs exactly once."

**Q: `#` vs `##`?**  
A: "`#` turns a macro argument into a string literal; `##` joins preprocessing tokens."

**Q: `#ifdef` vs `#if`?**  
A: "`#ifdef` checks whether defined; `#if` evaluates a preprocessor expression."

**Q: How do you disable logging in release build?**  
A: "Use `#if DEBUG` for enabled implementation and `#else` define the log macro as `((void)0)`; its arguments won't be evaluated."

**Q: How can I see the expanded macro?**  
A: "Use `gcc -E file.c` to inspect preprocessed source."

**Final 10-second checklist:** Parentheses ✓ · side effects ✓ · `do-while(0)` ✓ · `#`/`##` ✓ · `__VA_ARGS__` ✓ · `#if`/`#ifdef` ✓ · `ARRAY_SIZE` pointer trap ✓ · `__FILE__`/`__LINE__` ✓.
