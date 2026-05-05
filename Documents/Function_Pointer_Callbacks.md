# Function Pointers & Callbacks in C — Mental Model (Revision)

## Purpose

This document explains:

- What a function pointer **REALLY** is
- Why callbacks exist
- How callback registration works
- The exact mental model to reason about callbacks
- A minimal, correct example

Read this when confused. This is the ground truth.

---

## 1. Core Mental Model (Non-Negotiable)

A **function** in C has an **ADDRESS** (just like a variable).

A **function pointer** is a **VARIABLE** that stores that address.

A **callback** is NOT special:
- It is **JUST** a function pointer
- Passed/stored so it can be called later

**"Registering a callback"** means:
- Store a function's address somewhere safe

**"Triggering a callback"** means:
- Jump to that stored address and execute code

That's it. No magic. No hidden mechanism.

---

## 2. Why Callbacks Exist (The Real Reason)

If a function **hard-codes** another function call inside it:

```
download() → notify()
```

Then:
- Behavior is **FIXED**
- Code is **NOT reusable**
- Library controls the user (bad)

Callbacks **invert control**:
- **user** decides WHAT happens
- **system** decides WHEN it happens

This is called: **"Inversion of Control"**

ALL of these depend on callbacks:
- Interrupt handlers
- RTOS tasks
- Drivers
- Event systems
- HAL libraries
- Linux kernel hooks

---

## 3. Function Pointer Syntax — Mental Rule

> **RULE:** "typedef syntax is IDENTICAL to variable declaration syntax."

If this is valid:
```c
void (*ptr)(int);
```

Then typedef is:
```c
typedef void (*TYPE_NAME)(int);
```

Nothing new. No exception.

Parentheses **EXIST** because: `*` has lower precedence than `()`

Without parentheses:
```c
void *f(int);  // function returning pointer — WRONG
```

---

## 4. Callback Type Definition

Define the **SHAPE** of callbacks once.

```c
typedef void (*cb_hello_print)(void);
```

**Meaning:**
`cb_hello_print` is a TYPE that represents:
> pointer to function returning `void` taking no arguments

---

## 5. Callback Storage (Registration Slot)

You **MUST** store the callback somewhere.

```c
cb_hello_print print_hello_ptr = NULL;
```

**Mental model:** `print_hello_ptr` is:
- `NULL` → no callback registered
- Non-`NULL` → valid function address stored

---

## 6. Register / Deregister — What Really Happens

**REGISTER:**
```c
void register_hello_print_callback(cb_hello_print cb) {
    print_hello_ptr = cb;
}
```
> **Meaning:** Store function address for later use.

**DEREGISTER:**
```c
void de_register_hello_print_callback(void) {
    print_hello_ptr = NULL;
}
```
> **Meaning:** Forget the function address.

---

## 7. Safe Callback Usage Rule (Mandatory)

> **RULE: NEVER call a callback without checking NULL.**

**Why:** Calling `NULL` = jump to address `0` = crash / undefined behavior

**Correct:**
```c
if (print_hello_ptr != NULL)
    print_hello_ptr();
```

---

## 8. Callback Implementations (User Side)

These are **NORMAL** functions. No special syntax.

```c
void greet_bhai(void) {
    printf("Hello Bhai !!\n");
}

void greet_bhen(void) {
    printf("Hello Bhen !!\n");
}
```

---

## 9. Event / Trigger Function (Core Idea)

This function **DOES NOT know**:
- Which function will be called
- What that function does

It **ONLY** knows:
- IF callback exists → call it

```c
void greetings(void) {
    printf("Hi, ");

    if (print_hello_ptr != NULL)
        print_hello_ptr();
    else
        printf("Register the callback first.\n");
}
```

---

## 10. Complete Minimal Example

```c
#include <stdio.h>

typedef void (*cb_hello_print)(void);

cb_hello_print print_hello_ptr = NULL;

void greet_bhai(void) {
    printf("Hello Bhai !!\n");
}

void greet_bhen(void) {
    printf("Hello Bhen !!\n");
}

void register_hello_print_callback(cb_hello_print cb) {
    print_hello_ptr = cb;
}

void de_register_hello_print_callback(void) {
    print_hello_ptr = NULL;
}

void greetings(void) {
    printf("Hi, ");

    if (print_hello_ptr != NULL)
        print_hello_ptr();
    else
        printf("Register the callback first.\n");
}

int main(void) {
    register_hello_print_callback(greet_bhai);
    greetings();

    de_register_hello_print_callback();
    greetings();

    return 0;
}
```

---

## 11. Execution Trace (Mental Dry Run)

1. `register_hello_print_callback(greet_bhai)` → store address of `greet_bhai`
2. `greetings()` → print `"Hi, "` → callback exists → jump to `greet_bhai()`
3. `de_register_hello_print_callback()` → callback pointer = `NULL`
4. `greetings()` → print `"Hi, "` → no callback → print warning

---

## 12. One-Line Memory Anchors (Exam / Interview)

| Concept | One-liner |
|---|---|
| Function pointer | Variable storing a function address |
| Callback | Function pointer called later |
| Register | Store function address |
| Trigger | Call stored address |
| `typedef` | Not magic — only a name |
| Parentheses | Control meaning due to precedence |
| NULL check | **Mandatory** before every call |

---

*End of File*