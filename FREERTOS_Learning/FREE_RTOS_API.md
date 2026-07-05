# FreeRTOS API Reference — Complete Guide

> Complete topic-wise API reference with parameters, purpose, use cases, examples, and alternatives.
> Covers the classic single-core (Cortex-M / ESP32) FreeRTOS kernel plus ESP-IDF and SMP notes where relevant.

---

## Table of Contents

0. [Core Concepts & Foundations](#0-core-concepts--foundations)
1. [Task Management](#1-task-management)
2. [Scheduler Control](#1b-scheduler-control)
3. [Queue](#2-queue)
4. [Queue Sets](#2b-queue-sets)
5. [Semaphore — Binary](#3-semaphore--binary)
6. [Semaphore — Counting](#4-semaphore--counting)
7. [Mutex](#5-mutex)
8. [Mailbox](#6-mailbox)
9. [Task Notifications](#7-task-notifications)
10. [Event Groups / Event Bits](#8-event-groups--event-bits)
11. [Software Timers](#9-software-timers)
12. [Stream Buffer](#10-stream-buffer)
13. [Message Buffer](#11-message-buffer)
14. [Memory Management](#12-memory-management)
15. [Critical Sections and Interrupt Control](#13-critical-sections-and-interrupt-control)
16. [Hook Functions (Callbacks)](#13b-hook-functions-callbacks)
17. [FreeRTOSConfig.h Reference](#13c-freertosconfigh-reference)
18. [Interview Topics to Know Cold](#14-interview-topics-to-know-cold)
19. [Study Notes](#15-study-notes)
20. [Appendix A — Data Types](#appendix-a--data-types)
21. [Appendix B — Return Codes](#appendix-b--return-codes)
22. [Appendix C — Common Bugs & Gotchas](#appendix-c--common-bugs--gotchas)

---

## 0. Core Concepts & Foundations

Read this once — every API below assumes these ideas.

### What FreeRTOS is
A **real-time kernel** (not a full OS): a scheduler, plus inter-task communication and
synchronization primitives. No file system, no networking, no drivers in the core — those
are add-ons (FreeRTOS+TCP, FAT, ESP-IDF components, etc.).

### The Scheduler
- **Preemptive, priority-based** by default. The **highest-priority task that is Ready runs**.
- Equal-priority Ready tasks share the CPU via **time-slicing** (round-robin) when
  `configUSE_TIME_SLICING = 1`.
- A **tick interrupt** (default 1000 Hz → `configTICK_RATE_HZ`) drives time-slicing and delays.
- Preemption means a higher-priority task becoming Ready **immediately** interrupts a
  lower-priority running task — you rarely need to call `taskYIELD()` manually.

### Task States (know the diagram cold)

```
                 vTaskResume() / event
        ┌──────────────────────────────────────┐
        │                                       │
   ┌─────────┐   scheduler picks it      ┌──────────┐
   │ READY   │ ────────────────────────► │ RUNNING  │
   │         │ ◄──────────────────────── │          │
   └─────────┘   preempted / yields      └──────────┘
        ▲                                    │  │
        │ delay expires /                    │  │ vTaskDelay(), block on
        │ event occurs                       │  │ queue/sem/notify (with timeout)
        │                                    │  ▼
   ┌──────────┐                         ┌──────────┐
   │ BLOCKED  │ ◄────────────────────── │          │
   └──────────┘                         └──────────┘
        │  vTaskSuspend()                    │ vTaskSuspend()
        ▼                                    ▼
   ┌──────────────────────  SUSPENDED  ──────────────────────┐
   └─────────────────────────────────────────────────────────┘
```

- **Running** — currently executing (only one per core).
- **Ready** — able to run, waiting for the CPU (a higher/equal task has it).
- **Blocked** — waiting for a timeout or an event (queue item, semaphore, notification).
  Consumes no CPU. A blocked task always has a timeout (may be `portMAX_DELAY` = forever).
- **Suspended** — removed from scheduling by `vTaskSuspend()`; only `vTaskResume()` returns it.

### The Idle Task
- Created automatically when the scheduler starts, at priority **0** (`tskIDLE_PRIORITY`).
- Responsibilities: frees memory of tasks deleted with `vTaskDelete()`, runs the **idle hook**,
  and (if enabled) performs **tickless idle** for low power.
- **Never let the idle task starve** — give every other task a `vTaskDelay`/blocking point,
  or deleted-task memory is never reclaimed.

### Naming Conventions (decode any API at a glance)
| Prefix | Meaning | Example |
|---|---|---|
| `x` | returns/holds a `BaseType_t` or handle type | `xQueueSend`, `xTaskCreate` |
| `v` | returns `void` | `vTaskDelay` |
| `ul` | unsigned long (32-bit) | `ulTaskNotifyTake` |
| `ux` | unsigned base type | `uxQueueMessagesWaiting` |
| `pv` | pointer to void | `pvParameters` |
| `pc` | pointer to char | `pcName` |
| `px` | pointer to a struct/BaseType | `pxCreatedTask` |
| `pd*` | `#define` constant | `pdTRUE`, `pdMS_TO_TICKS` |
| `...FromISR` | interrupt-safe variant — never blocks | `xQueueSendFromISR` |

### Ticks and Time
```c
TickType_t ticks = pdMS_TO_TICKS(250);   // convert ms → ticks (always use this macro)
// portTICK_PERIOD_MS = ms per tick (e.g. 1 at 1000 Hz)
// portMAX_DELAY = block forever (only truly "forever" if configUSE_TIMERS/INCLUDE_vTaskSuspend=1)
```

### Static vs Dynamic Allocation
- **Dynamic** (`xTaskCreate`, `xQueueCreate`, …): kernel calls `pvPortMalloc`. Needs
  `configSUPPORT_DYNAMIC_ALLOCATION = 1`.
- **Static** (`xTaskCreateStatic`, `xQueueCreateStatic`, …): *you* supply the RAM. No heap,
  deterministic, preferred in safety-critical / certified firmware. Needs
  `configSUPPORT_STATIC_ALLOCATION = 1`.

### Minimal program skeleton
```c
#include "FreeRTOS.h"
#include "task.h"

void vBlink(void *pv) {
    for (;;) {
        toggle_led();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void) {
    hardware_init();
    xTaskCreate(vBlink, "blink", 128, NULL, 2, NULL);
    vTaskStartScheduler();   // never returns if it succeeds
    for (;;);                // only reached if heap for idle/timer task was too small
}
```

---

## 1. Task Management

### Purpose
Create, delete, suspend, and control tasks — the fundamental unit of execution in FreeRTOS.

---

### `xTaskCreate()`

```c
BaseType_t xTaskCreate(
    TaskFunction_t  pvTaskCode,      // Function pointer — task entry function
    const char*     pcName,          // Human readable name — for debugging only
    uint16_t        usStackDepth,    // Stack size in WORDS (not bytes) on most ports
    void*           pvParameters,    // Argument passed to task function
    UBaseType_t     uxPriority,      // Priority — 0 (lowest/Idle) to configMAX_PRIORITIES-1
    TaskHandle_t*   pxCreatedTask    // Output handle — pass NULL if not needed
);
// Returns: pdPASS on success, errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY on failure
```

**Parameters in detail:**

| Parameter | Type | Notes |
|---|---|---|
| `pvTaskCode` | `TaskFunction_t` | Signature `void fn(void *)`. Must **never return** — loop forever or call `vTaskDelete(NULL)`. |
| `pcName` | `const char*` | Copied into the TCB, truncated to `configMAX_TASK_NAME_LEN` (default 16). Debug/trace only. |
| `usStackDepth` | `uint16_t` | In **words**, not bytes. On a 32-bit MCU 1 word = 4 bytes, so `128` = 512 bytes. Size it with `uxTaskGetStackHighWaterMark`. |
| `pvParameters` | `void*` | Passed straight to the task. Common pattern: pointer to a config struct. Must stay valid for the task's lifetime (don't point to a stack local of `main`). |
| `uxPriority` | `UBaseType_t` | `0` = idle priority (lowest). Max is `configMAX_PRIORITIES - 1`. Values ≥ `configMAX_PRIORITIES` are silently capped. |
| `pxCreatedTask` | `TaskHandle_t*` | Receives a handle for later `vTaskDelete`/`vTaskSuspend`/notify. `NULL` if you never need to reference the task. |

**Use when:** Dynamic allocation acceptable. Simpler, most common usage.

**Fails when:** Heap exhausted — returns `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` (`pdFAIL`) instead of crashing. **Always check the return value.**

**Complete example — passing a parameter struct:**
```c
typedef struct { gpio_t pin; uint32_t period_ms; } BlinkCfg_t;

void vBlinkTask(void *pv) {
    BlinkCfg_t *cfg = (BlinkCfg_t *)pv;   // recover the argument
    for (;;) {
        gpio_toggle(cfg->pin);
        vTaskDelay(pdMS_TO_TICKS(cfg->period_ms));
    }
}

// must be static/global so it outlives the create call
static BlinkCfg_t redCfg = { .pin = LED_RED, .period_ms = 250 };

TaskHandle_t hBlink = NULL;
if (xTaskCreate(vBlinkTask, "red", 128, &redCfg, 2, &hBlink) != pdPASS) {
    // handle out-of-memory
}
```

---

### `xTaskCreateStatic()`

```c
TaskHandle_t xTaskCreateStatic(
    TaskFunction_t  pvTaskCode,
    const char*     pcName,
    uint32_t        ulStackDepth,
    void*           pvParameters,
    UBaseType_t     uxPriority,
    StackType_t*    puxStackBuffer,      // YOU provide stack array
    StaticTask_t*   pxTaskBuffer         // YOU provide Task Control Block memory
);
// Returns: Task handle — never NULL (memory provided by caller)
```

**Why use over xTaskCreate:** No heap dependency. Stack size known at compile time. No fragmentation. Preferred in production firmware. Idle Task cleanup not required.

**Alternate:** `xTaskCreate()` — simpler but heap dependent.

---

### `xTaskCreatePinnedToCore()` — ESP32 Specific

```c
BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t  pvTaskCode,
    const char*     pcName,
    uint32_t        usStackDepth,
    void*           pvParameters,
    UBaseType_t     uxPriority,
    TaskHandle_t*   pxCreatedTask,
    BaseType_t      xCoreID          // 0, 1, or tskNO_AFFINITY
);
```

**Why use:** Pin task to specific core on ESP32 dual core. WiFi stack pinned to Core 0 by Espressif — pin your tasks to Core 1 to avoid contention.

**Alternate:** `xTaskCreate()` — unpinned, scheduler decides core.

---

### `vTaskDelete()`

```c
void vTaskDelete(TaskHandle_t xTaskToDelete);
// Pass NULL to delete calling task itself
```

**Important:** Stack and Task Control Block NOT freed immediately. Idle Task cleans up later. If Idle Task starved — memory leak. Use `xTaskCreateStatic()` to avoid this entirely.

---

### `vTaskDelay()`

```c
void vTaskDelay(TickType_t xTicksToDelay);
// Helper macro: pdMS_TO_TICKS(ms) converts milliseconds to ticks
// Example: vTaskDelay(pdMS_TO_TICKS(100)); // delay 100ms
```

**Use when:** Simple delay needed. Task blocks for specified ticks from NOW.

**Problem:** Drift accumulates over time — period = execution time + delay time. Not suitable for strict periodic tasks.

**Alternate:** `vTaskDelayUntil()` — fixed period, no drift.

---

### `vTaskDelayUntil()`

```c
void vTaskDelayUntil(
    TickType_t* pxPreviousWakeTime,   // Pointer to last wake tick — updated automatically
    TickType_t  xTimeIncrement        // Period in ticks
);

// Usage pattern:
TickType_t xLastWakeTime = xTaskGetTickCount();
while(1) {
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); // Exactly 10ms period
    doWork();
}
```

**Why use over vTaskDelay:** Absolute period maintained. Execution time does NOT accumulate into period. Correct for motor control loops, sensor sampling, anything needing fixed frequency.

---

### `vTaskSuspend()` / `vTaskResume()`

```c
void vTaskSuspend(TaskHandle_t xTaskToSuspend);  // NULL = suspend self
void vTaskResume(TaskHandle_t xTaskToResume);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume); // Call from ISR
```

**Use when:** Need to stop task temporarily without deleting it. RAM retained. No cleanup needed.

**Alternate:** `vTaskDelete()` — permanent. Task Notifications — cleaner for activation/deactivation.

---

### `vTaskPrioritySet()` / `uxTaskPriorityGet()`

```c
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
// NULL = calling task

UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);
```

**Use when:** Implementing manual Priority Ceiling. FreeRTOS Mutex uses this internally for Priority Inheritance.

---

### Debugging APIs

```c
// Stack watermark — minimum free stack words ever recorded
// 0 = already overflowed
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);

// Print all task states, priorities, stack watermarks
void vTaskList(char* pcWriteBuffer);

// Print CPU time per task (requires configGENERATE_RUN_TIME_STATS = 1)
void vTaskGetRunTimeStats(char* pcWriteBuffer);

// Current tick count
TickType_t xTaskGetTickCount(void);

// Number of tasks currently existing
UBaseType_t uxTaskGetNumberOfTasks(void);
```

---

### Task Introspection & Utility APIs

```c
// Handle of the calling task
TaskHandle_t xTaskGetCurrentTaskHandle(void);

// Look up a task handle by its name (slow — scans the task list)
TaskHandle_t xTaskGetHandle(const char* pcNameToQuery);

// Current state of a task
eTaskState eTaskGetState(TaskHandle_t xTask);
// Returns: eRunning, eReady, eBlocked, eSuspended, eDeleted, eInvalid

// Fill a TaskStatus_t with everything about a task (name, state, prio, stack, runtime)
void vTaskGetInfo(
    TaskHandle_t      xTask,
    TaskStatus_t*     pxTaskStatus,
    BaseType_t        xGetFreeStackSpace,   // pdTRUE = also compute high-water mark (slow)
    eTaskState        eState                // eInvalid = let the kernel fetch it
);

// Snapshot of ALL tasks at once — preferred over vTaskList for machine parsing
UBaseType_t uxTaskGetSystemState(
    TaskStatus_t*     pxTaskStatusArray,    // caller-provided array
    UBaseType_t       uxArraySize,          // number of elements available
    uint32_t*         pulTotalRunTime       // optional — total run-time counter
);

// Store/retrieve one user pointer per task (a "thread-local" slot)
void  vTaskSetApplicationTaskTag(TaskHandle_t xTask, TaskHookFunction_t pxTagValue);
void  vTaskSetThreadLocalStoragePointer(TaskHandle_t xTask, BaseType_t xIndex, void* pvValue);
void* pvTaskGetThreadLocalStoragePointer(TaskHandle_t xTask, BaseType_t xIndex);
```

---

### `taskYIELD()` / `taskYIELD_FROM_ISR()`

```c
taskYIELD();   // voluntarily give up the CPU to another Ready task of >= priority
```

**Use when:** Cooperative hand-off between equal-priority tasks, or forcing an immediate
reschedule after you changed something. Rarely needed with preemption enabled — the
scheduler already switches on every relevant event.

---

### Deleting the calling task cleanly

```c
void vSomeOneShotTask(void *pv) {
    do_one_time_work();
    vTaskDelete(NULL);   // NULL = "delete me". Never returns.
}
```

**Reminder:** the TCB + stack are reclaimed by the **idle task**, so the idle task must get
to run. If you delete a task from *another* task, the memory frees on the next idle slice.

---

## 1b. Scheduler Control

### Purpose
Start the scheduler and, when unavoidable, pause/resume all scheduling. Prefer the lighter
`vTaskSuspendAll`/critical-section tools over stopping the whole scheduler.

---

### `vTaskStartScheduler()`

```c
void vTaskStartScheduler(void);
// Call once from main() after creating your initial tasks.
// Does NOT return on success. Returns only if there was not enough heap to
// create the idle task (and timer task if configUSE_TIMERS = 1).
```

---

### `vTaskSuspendAll()` / `xTaskResumeAll()`

```c
void       vTaskSuspendAll(void);   // stop context switches — interrupts STILL run
BaseType_t xTaskResumeAll(void);    // returns pdTRUE if a yield happened on resume
```

**Difference from a critical section:** interrupts keep firing (ISRs still execute), only
**task switching** is frozen. Cheaper than disabling interrupts for medium-length
non-blocking work that touches task-shared data. **You must not call any blocking API while
the scheduler is suspended.**

```c
vTaskSuspendAll();
{
    // safe from other TASKS (not from ISRs) — no blocking calls allowed
    update_shared_list();
}
xTaskResumeAll();
```

---

### `xTaskGetSchedulerState()`

```c
BaseType_t xTaskGetSchedulerState(void);
// taskSCHEDULER_NOT_STARTED | taskSCHEDULER_RUNNING | taskSCHEDULER_SUSPENDED
```

**Use when:** shared code needs to behave differently before vs after the scheduler starts
(e.g. a driver init that may run in `main()` or in a task).

---

## 2. Queue

### Purpose
Thread-safe FIFO buffer for passing data between tasks or from Interrupt Service Routine to task. Data COPIED in and out — not passed by reference.

---

### `xQueueCreate()`

```c
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,    // Maximum number of items queue can hold
    UBaseType_t uxItemSize        // Size of each item in bytes
);
// Returns: Handle or NULL on failure

// Static variant — you supply the storage
QueueHandle_t xQueueCreateStatic(
    UBaseType_t     uxQueueLength,
    UBaseType_t     uxItemSize,
    uint8_t*        pucQueueStorage,   // array of (uxQueueLength * uxItemSize) bytes
    StaticQueue_t*  pxQueueBuffer      // control-structure storage
);
```

**Key idea — data is COPIED.** `uxItemSize` is the size of the *thing you copy*. To move
large or variable objects, make the item a **pointer** (`sizeof(void*)`) and manage the
buffer's lifetime yourself (the receiver frees it or returns it to a pool).

**Complete producer/consumer example:**
```c
typedef struct { uint8_t id; float value; } Reading_t;

QueueHandle_t xReadingQ;

void vProducer(void *pv) {
    Reading_t r;
    for (;;) {
        r.id = 1; r.value = read_sensor();
        // block up to 10 ms if the queue is full
        if (xQueueSend(xReadingQ, &r, pdMS_TO_TICKS(10)) != pdTRUE) {
            // queue stayed full — drop or count the overflow
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vConsumer(void *pv) {
    Reading_t r;
    for (;;) {
        if (xQueueReceive(xReadingQ, &r, portMAX_DELAY) == pdTRUE) {
            printf("id=%u val=%.2f\n", r.id, r.value);
        }
    }
}

void app_init(void) {
    xReadingQ = xQueueCreate(8, sizeof(Reading_t));   // 8 items deep
    configASSERT(xReadingQ != NULL);
    xTaskCreate(vProducer, "prod", 256, NULL, 2, NULL);
    xTaskCreate(vConsumer, "cons", 256, NULL, 2, NULL);
}
```

---

### `xQueueSend()` / `xQueueSendToBack()`

```c
BaseType_t xQueueSend(
    QueueHandle_t xQueue,
    const void*   pvItemToQueue,   // Pointer to data to copy in
    TickType_t    xTicksToWait     // How long to block if queue full. 0 = don't wait. portMAX_DELAY = wait forever
);
// Returns: pdTRUE on success, errQUEUE_FULL on timeout
```

---

### `xQueueSendToFront()`

```c
BaseType_t xQueueSendToFront(
    QueueHandle_t xQueue,
    const void*   pvItemToQueue,
    TickType_t    xTicksToWait
);
```

**Why use over xQueueSend:** Inserts at FRONT — highest priority message jumps queue. Emergency/urgent message use case.

---

### `xQueueSendFromISR()` — CRITICAL

```c
BaseType_t xQueueSendFromISR(
    QueueHandle_t xQueue,
    const void*   pvItemToQueue,
    BaseType_t*   pxHigherPriorityTaskWoken  // Set to pdTRUE if higher priority task unblocked
);
// MUST call portYIELD_FROM_ISR(pxHigherPriorityTaskWoken) after
```

**Why separate API exists:** Interrupt Service Routines CANNOT block. Standard `xQueueSend()` can block — illegal in ISR context. FromISR version never blocks — returns immediately with error if queue full.

**Always pair with:**
```c
portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
// Forces immediate context switch if higher priority task was unblocked
```

---

### `xQueueReceive()`

```c
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void*         pvBuffer,        // Buffer to copy received data into
    TickType_t    xTicksToWait
);
// Item REMOVED from queue after receive
```

---

### `xQueuePeek()`

```c
BaseType_t xQueuePeek(
    QueueHandle_t xQueue,
    void*         pvBuffer,
    TickType_t    xTicksToWait
);
// Item NOT removed — stays in queue
```

**Use when:** Want to inspect front item without consuming it. Mailbox read pattern.

---

### `uxQueueMessagesWaiting()`

```c
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);
// Returns number of items currently in queue
```

---

### Queue vs Alternatives

| Need | Use |
|---|---|
| Pass data task to task | Queue |
| Signal without data | Binary Semaphore or Task Notification |
| Latest value only | Mailbox (Queue size 1 + xQueueOverwrite) |
| Single uint32 value fast | Task Notification |

---

### Other Queue APIs

```c
// Space remaining before the queue is full
UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue);

// Empty a queue — discards all items
BaseType_t  xQueueReset(QueueHandle_t xQueue);

// Delete a queue and free its memory (dynamic queues only)
void        vQueueDelete(QueueHandle_t xQueue);

// ISR-safe inspection
BaseType_t  xQueueIsQueueEmptyFromISR(const QueueHandle_t xQueue);
BaseType_t  xQueueIsQueueFullFromISR(const QueueHandle_t xQueue);
UBaseType_t uxQueueMessagesWaitingFromISR(const QueueHandle_t xQueue);

// Give a queue a name in the kernel registry (for debuggers like Ozone/Tracealyzer)
void        vQueueAddToRegistry(QueueHandle_t xQueue, const char* pcName);
```

---

## 2b. Queue Sets

### Purpose
Block on **several queues and/or semaphores at once**. A task waits on the *set*; when any
member has data/is signalled, the set tells you which one. This is the FreeRTOS answer to
"select()" — needed when one task must service multiple sources and Task Notifications /
Event Groups don't fit.

> Requires `configUSE_QUEUE_SETS = 1`.

---

### APIs

```c
QueueSetHandle_t xQueueCreateSet(UBaseType_t uxEventQueueLength);
// uxEventQueueLength MUST equal the SUM of the lengths of all queues/semaphores added
// (each counting-sem counts as its max count, each binary sem/mutex as 1).

BaseType_t xQueueAddToSet(QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet);
BaseType_t xQueueRemoveFromSet(QueueSetMemberHandle_t xQueueOrSemaphore, QueueSetHandle_t xQueueSet);

// Blocks until any member is ready; returns the handle of the ready member (or NULL on timeout)
QueueSetMemberHandle_t xQueueSelectFromSet(QueueSetHandle_t xQueueSet, TickType_t xTicksToWait);
QueueSetMemberHandle_t xQueueSelectFromSetFromISR(QueueSetHandle_t xQueueSet);
```

**Complete example — one task serving a data queue and a control semaphore:**
```c
QueueHandle_t     xDataQ  = xQueueCreate(5, sizeof(Reading_t));
SemaphoreHandle_t xStopSem = xSemaphoreCreateBinary();
QueueSetHandle_t  xSet    = xQueueCreateSet(5 + 1);   // 5 + 1

xQueueAddToSet(xDataQ,  xSet);
xQueueAddToSet(xStopSem, xSet);

void vServiceTask(void *pv) {
    for (;;) {
        QueueSetMemberHandle_t m = xQueueSelectFromSet(xSet, portMAX_DELAY);
        if (m == xDataQ) {
            Reading_t r;
            xQueueReceive(xDataQ, &r, 0);   // guaranteed non-blocking now
            process(&r);
        } else if (m == xStopSem) {
            xSemaphoreTake(xStopSem, 0);
            break;
        }
    }
    vTaskDelete(NULL);
}
```

**Caveats:**
- You must still **receive/take** from the returned member — the set only tells you *which*.
- A queue or semaphore can be in **only one set** and must be empty when added.
- Mutexes should not be added (priority inheritance interacts badly). Use for queues,
  binary and counting semaphores.
- **Alternative:** if the sources are simple flags, an **Event Group** is lighter than a queue set.

---

## 3. Semaphore — Binary

### Purpose
Signaling between tasks or from Interrupt Service Routine to task. No data transferred — pure event notification. No ownership.

---

### `xSemaphoreCreateBinary()`

```c
SemaphoreHandle_t xSemaphoreCreateBinary(void);
// Created in EMPTY state (0) — must Give before first Take succeeds
```

---

### `xSemaphoreGive()` / `xSemaphoreTake()`

```c
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

BaseType_t xSemaphoreTake(
    SemaphoreHandle_t xSemaphore,
    TickType_t        xTicksToWait
);
```

---

### `xSemaphoreGiveFromISR()` — Classic ISR to Task Pattern

```c
BaseType_t xSemaphoreGiveFromISR(
    SemaphoreHandle_t xSemaphore,
    BaseType_t*       pxHigherPriorityTaskWoken
);

// Full ISR to Task pattern:
SemaphoreHandle_t xSemaphore;

void IRAM_ATTR sensorISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void sensorTask(void* param) {
    while(1) {
        xSemaphoreTake(xSemaphore, portMAX_DELAY); // Blocks until ISR fires
        processSensorData();
    }
}
```

**Why Binary Semaphore over Mutex for ISR signaling:** Mutex has ownership — only taker can give. ISR and task are different contexts — ISR cannot "own" mutex. Binary Semaphore has no ownership — ISR can Give, task can Take.

**Why Binary Semaphore over Task Notification:** Multiple tasks can wait on one semaphore. Task Notification is per-task — only one task can wait.

---

## 4. Semaphore — Counting

### Purpose
Manage pool of identical resources. Tracks count of available resources. Multiple tasks can acquire simultaneously up to pool size.

---

### `xSemaphoreCreateCounting()`

```c
SemaphoreHandle_t xSemaphoreCreateCounting(
    UBaseType_t uxMaxCount,      // Maximum count value (pool size)
    UBaseType_t uxInitialCount   // Starting count
);
```

**Use case — 3 UART ports:**
```c
SemaphoreHandle_t uartPool = xSemaphoreCreateCounting(3, 3);

// Task acquires one port
xSemaphoreTake(uartPool, portMAX_DELAY); // count: 3→2
// use port
xSemaphoreGive(uartPool);                // count: 2→3

// When count = 0 → all tasks block until a port is released
```

**Use case — event counting:**
```c
// ISR fires multiple times before task processes
// Binary Semaphore would lose events — only holds 0 or 1
// Counting Semaphore accumulates — each Give increments count
// Task calls Take once per event — processes all accumulated events
SemaphoreHandle_t eventCounter = xSemaphoreCreateCounting(10, 0);
```

**Alternate:** Queue with dummy data for counting. Task Notifications with eIncrement action.

---

### Semaphore Utility APIs (all semaphore types)

```c
// Current count: 0/1 for binary & mutex, 0..max for counting.
UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t xSemaphore);

// Give/Take from an ISR
BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, BaseType_t* pxHigherPriorityTaskWoken);
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t* pxHigherPriorityTaskWoken);

// Which task currently holds a mutex (NULL if free). Debugging aid.
TaskHandle_t xSemaphoreGetMutexHolder(SemaphoreHandle_t xMutex);

// Free the object (dynamic only)
void vSemaphoreDelete(SemaphoreHandle_t xSemaphore);

// Static creation variants — you provide the StaticSemaphore_t
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t* pxBuffer);
SemaphoreHandle_t xSemaphoreCreateCountingStatic(UBaseType_t max, UBaseType_t initial, StaticSemaphore_t* pxBuffer);
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t* pxBuffer);
```

**Return-value rule (Take):** `xSemaphoreTake` returns `pdTRUE` only if it actually obtained
the semaphore. On timeout it returns `pdFALSE` — **always branch on it**; treating a timeout
as success is the classic cause of "it works until the bus is busy" bugs.

---

## 5. Mutex

### Purpose
Mutual exclusion — protect shared resource with OWNERSHIP guarantee. Only task that locked can unlock. Has Priority Inheritance built in.

---

### `xSemaphoreCreateMutex()`

```c
SemaphoreHandle_t xSemaphoreCreateMutex(void);
// Created in GIVEN state — ready to take immediately
```

**What makes Mutex different from Binary Semaphore:**

| Property | Mutex | Binary Semaphore |
|---|---|---|
| Ownership | YES — only taker can give | NO |
| Priority Inheritance | YES — automatic | NO |
| Use case | Protect shared resource | Signal between tasks/ISR |
| Can Give from ISR | NO — ISR has no ownership | YES |
| Recursive | No (use Recursive Mutex) | No |

---

### `xSemaphoreCreateRecursiveMutex()`

```c
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);

// Must use matching recursive APIs:
xSemaphoreTakeRecursive(xMutex, xTicksToWait);
xSemaphoreGiveRecursive(xMutex);
```

**Why use:** Same task can lock same mutex multiple times without deadlocking itself. Each Take must be matched by a Give. Use when recursive functions need to acquire same mutex.

**Alternate:** Non-recursive mutex — simpler, but causes deadlock if same task tries to lock twice.

---

### Mutex UART Protection Example

```c
SemaphoreHandle_t uartMutex = xSemaphoreCreateMutex();

void Task1(void* param) {
    while(1) {
        xSemaphoreTake(uartMutex, portMAX_DELAY);
        // Only Task1 can release — no other task can steal UART mid-transmission
        HAL_UART_Transmit(&huart1, data1, len1, HAL_MAX_DELAY);
        xSemaphoreGive(uartMutex);
    }
}
```

---

## 6. Mailbox

### Purpose
Share LATEST state value between tasks. Any task can read without consuming. Writer overwrites without waiting for reader. Queue of size 1 with special APIs.

---

### Create Mailbox

```c
// Mailbox = Queue of length 1
QueueHandle_t xMailbox = xQueueCreate(1, sizeof(SensorData_t));
```

---

### `xQueueOverwrite()`

```c
BaseType_t xQueueOverwrite(
    QueueHandle_t xQueue,
    const void*   pvItemToQueue
);
// Overwrites existing value — NEVER blocks even if queue full
// Queue must be length 1

BaseType_t xQueueOverwriteFromISR(
    QueueHandle_t xQueue,
    const void*   pvItemToQueue,
    BaseType_t*   pxHigherPriorityTaskWoken
);
```

---

### Read Mailbox Without Consuming

```c
// Peek — read without removing
xQueuePeek(xMailbox, &sensorData, portMAX_DELAY);
// Multiple tasks can peek same value
// Value stays for next reader
```

**Mailbox vs Queue:**

| | Queue | Mailbox |
|---|---|---|
| Size | N items | 1 item only |
| Old data | Preserved until read | Overwritten by new data |
| Multiple readers | Each gets different item | All readers see same latest value |
| Use case | Message passing | Shared state / latest value |

**Real use case:** Temperature sensor task writes latest reading to mailbox every 100ms. Multiple display/control tasks all peek same latest value independently.

---

## 7. Task Notifications

### Purpose
Lightweight, fast signaling directly to a specific task. Built into Task Control Block — no separate object needed. Faster and uses less RAM than semaphore/queue.

---

### `xTaskNotifyGive()` / `ulTaskNotifyTake()`

```c
// Simplified notify — acts like binary or counting semaphore
BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);

// FromISR version
vTaskNotifyGiveFromISR(
    TaskHandle_t xTaskToNotify,
    BaseType_t*  pxHigherPriorityTaskWoken
);

// Receiving task
uint32_t ulTaskNotifyTake(
    BaseType_t xClearCountOnExit,   // pdTRUE = binary semaphore behavior
                                     // pdFALSE = counting semaphore behavior
    TickType_t xTicksToWait
);
```

---

### `xTaskNotify()` / `xTaskNotifyWait()`

```c
// Full featured — can send 32-bit value
BaseType_t xTaskNotify(
    TaskHandle_t xTaskToNotify,
    uint32_t     ulValue,
    eNotifyAction eAction    // How to update notification value:
                             // eNoAction — just unblock task
                             // eSetBits — OR value into notification
                             // eIncrement — increment notification value
                             // eSetValueWithOverwrite — overwrite value
                             // eSetValueWithoutOverwrite — only if task not already notified
);

// Receiving task waits for notification
BaseType_t xTaskNotifyWait(
    uint32_t ulBitsToClearOnEntry,   // Clear these bits when entering wait
    uint32_t ulBitsToClearOnExit,    // Clear these bits when exiting wait
    uint32_t* pulNotificationValue,  // Output — notification value
    TickType_t xTicksToWait
);
```

---

### Task Notification vs Semaphore

| | Task Notification | Semaphore / Queue |
|---|---|---|
| RAM overhead | Zero — in Task Control Block | Separate object needed |
| Speed | Faster | Slightly slower |
| Multiple waiters | NO — only target task | YES — multiple tasks can wait |
| Send to specific task | YES | NO — whoever takes first gets it |
| Use case | One sender, one receiver | Multiple senders or receivers |

**Best use case for Task Notifications:**
```c
// ISR signals exactly one known task — fastest possible mechanism
void IRAM_ATTR gpioISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xHandlerTask, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void handlerTask(void* param) {
    while(1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for ISR signal
        handleGPIOEvent();
    }
}
```

---

### `eNotifyAction` — all five actions explained

`xTaskNotify(handle, value, action)` updates the target's 32-bit notification value
according to `action`:

| Action | Effect on notification value | Return | Mimics |
|---|---|---|---|
| `eNoAction` | Unchanged — just unblocks the task | always `pdPASS` | a binary semaphore signal |
| `eSetBits` | `value \|= ulValue` (OR-in bits) | always `pdPASS` | lightweight event group |
| `eIncrement` | `value += 1` (`ulValue` ignored) | always `pdPASS` | counting semaphore |
| `eSetValueWithOverwrite` | `value = ulValue` (always) | always `pdPASS` | mailbox (latest wins) |
| `eSetValueWithoutOverwrite` | `value = ulValue` **only if no pending notification** | `pdFAIL` if one was already pending | mailbox that refuses to clobber unread data |

```c
// Send a status word to a worker, overwriting any stale one:
xTaskNotify(hWorker, STATUS_ERROR, eSetValueWithOverwrite);

// OR-in event flags without losing previously set ones:
xTaskNotify(hWorker, EVT_RX_DONE, eSetBits);
```

---

### Waiting: `xTaskNotifyWait()` bit-clear semantics

```c
uint32_t notifiedValue;
xTaskNotifyWait(
    0x00,              // ulBitsToClearOnEntry: clear these bits BEFORE waiting (0 = keep)
    0xFFFFFFFF,        // ulBitsToClearOnExit:  clear these bits AFTER receiving (all = reset)
    &notifiedValue,    // receives the value at the moment of unblocking
    portMAX_DELAY);
if (notifiedValue & EVT_RX_DONE) { /* ... */ }
```

- **ClearOnEntry** lets you discard leftover bits so you only react to *new* ones.
- **ClearOnExit = 0xFFFFFFFF** resets the value to 0 after reading — "consume" semantics.

---

### Notification management & indexed notifications

```c
// Clear a pending notification state (undo a give that hasn't been taken)
BaseType_t xTaskNotifyStateClear(TaskHandle_t xTask);

// Clear specific bits of the notification value without waiting
uint32_t   ulTaskNotifyValueClear(TaskHandle_t xTask, uint32_t ulBitsToClear);
```

**Indexed notifications** (FreeRTOS v10.4+, `configTASK_NOTIFICATION_ARRAY_ENTRIES > 1`): each
task has an **array** of notifications so different senders can target different "channels"
without interfering. Every classic call maps to an `...Indexed` form:

```c
xTaskNotifyGiveIndexed(handle, uxIndex);
ulTaskNotifyTakeIndexed(uxIndex, xClearOnExit, xTicksToWait);
xTaskNotifyIndexed(handle, uxIndex, value, action);
xTaskNotifyWaitIndexed(uxIndex, clearEntry, clearExit, &value, xTicksToWait);
// The non-indexed calls are just index 0.
```

**Why:** lets one driver use index 0 for "transfer complete" while another subsystem uses
index 1 for "abort", on the same task, with zero cross-talk.

**Big limitation (know for interviews):** a task notification goes to **exactly one known
task**. It cannot be given to a semaphore/queue, cannot be waited on by multiple tasks, and
(mostly) cannot be used as a broadcast. When you need many waiters or many senders, use a
semaphore, queue, or event group.

---

## 8. Event Groups / Event Bits

### Purpose
Wait for MULTIPLE events simultaneously. Single task can block until ANY or ALL of several events occur. Up to 24 usable bits per event group.

---

### `xEventGroupCreate()`

```c
EventGroupHandle_t xEventGroupCreate(void);
```

---

### `xEventGroupSetBits()`

```c
EventBits_t xEventGroupSetBits(
    EventGroupHandle_t xEventGroup,
    EventBits_t        uxBitsToSet    // Bitmask — set these bits
);

// FromISR version
BaseType_t xEventGroupSetBitsFromISR(
    EventGroupHandle_t xEventGroup,
    EventBits_t        uxBitsToSet,
    BaseType_t*        pxHigherPriorityTaskWoken
);
```

---

### `xEventGroupWaitBits()`

```c
EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t xEventGroup,
    EventBits_t        uxBitsToWaitFor,  // Which bits to watch
    BaseType_t         xClearOnExit,     // pdTRUE = clear bits when condition met
    BaseType_t         xWaitForAllBits,  // pdTRUE = wait for ALL bits (AND)
                                          // pdFALSE = wait for ANY bit (OR)
    TickType_t         xTicksToWait
);
// Returns: bit pattern at time of return
```

**Real use case:**
```c
#define SENSOR_READY_BIT    (1 << 0)
#define GPS_READY_BIT       (1 << 1)
#define NETWORK_READY_BIT   (1 << 2)

EventGroupHandle_t xInitGroup = xEventGroupCreate();

// System init task — wait for ALL subsystems ready
xEventGroupWaitBits(
    xInitGroup,
    SENSOR_READY_BIT | GPS_READY_BIT | NETWORK_READY_BIT,
    pdTRUE,   // clear bits after
    pdTRUE,   // wait for ALL (AND)
    portMAX_DELAY
);
startMainApplication();
```

---

### `xEventGroupClearBits()` / `xEventGroupGetBits()`

```c
EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear);
EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup); // Read without waiting
EventBits_t xEventGroupGetBitsFromISR(EventGroupHandle_t xEventGroup);
void        vEventGroupDelete(EventGroupHandle_t xEventGroup);
```

---

### `xEventGroupSync()` — task rendezvous / barrier

```c
EventBits_t xEventGroupSync(
    EventGroupHandle_t xEventGroup,
    EventBits_t        uxBitsToSet,       // bits THIS task sets to say "I've arrived"
    EventBits_t        uxBitsToWaitFor,   // wait until all these are set (everyone arrived)
    TickType_t         xTicksToWait
);
```

**Use when:** several tasks must all reach a sync point before any proceeds (a barrier).
Atomically sets this task's bit and waits for the others — impossible to race with separate
Set + Wait calls.

```c
#define TASK_A_BIT (1<<0)
#define TASK_B_BIT (1<<1)
#define TASK_C_BIT (1<<2)
#define ALL_SYNC   (TASK_A_BIT | TASK_B_BIT | TASK_C_BIT)

// each task, at the barrier:
xEventGroupSync(xBarrier, TASK_A_BIT, ALL_SYNC, portMAX_DELAY);
// all three continue together from here
```

---

### Event Group vs Task Notification

| | Event Group | Task Notification |
|---|---|---|
| Multiple events AND/OR | YES | Limited — only one 32-bit value |
| Multiple tasks waiting | YES — all unblocked when bits set | NO — one target task only |
| Bits persist until cleared | YES | Cleared on take |
| Use case | Synchronize multiple subsystems | Fast single task signaling |

---

## 9. Software Timers

### Purpose
Execute callback function after a delay or periodically — without dedicating a task. Timer callbacks run in Timer Service Task context.

---

### `xTimerCreate()`

```c
TimerHandle_t xTimerCreate(
    const char*     pcTimerName,      // Debug name
    TickType_t      xTimerPeriod,     // Period in ticks
    UBaseType_t     uxAutoReload,     // pdTRUE = repeating, pdFALSE = one-shot
    void*           pvTimerID,        // ID passed to callback — use to identify timer
    TimerCallbackFunction_t pxCallbackFunction  // void callback(TimerHandle_t xTimer)
);
```

---

### Timer Control APIs

```c
BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);  // Restart period from now
BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, TickType_t xNewPeriod, TickType_t xTicksToWait);
void* pvTimerGetTimerID(TimerHandle_t xTimer);  // Get ID inside callback
```

**One-shot vs Auto-reload:**
```c
// One-shot — fires once, stops
TimerHandle_t xTimeout = xTimerCreate("timeout", pdMS_TO_TICKS(5000), pdFALSE, 0, timeoutCallback);

// Auto-reload — fires repeatedly every period
TimerHandle_t xHeartbeat = xTimerCreate("heartbeat", pdMS_TO_TICKS(1000), pdTRUE, 0, heartbeatCallback);
```

**Critical rule:** Timer callbacks MUST NOT block. They run in Timer Service Task — blocking starves all other timer callbacks.

**Alternate:** Dedicated task with `vTaskDelayUntil()` — more flexible, can block, has own stack.

---

### The Timer Service (Daemon) Task

Software timers do **not** each get a task. One shared **timer daemon task** runs all
callbacks. It is created by `vTaskStartScheduler()` when `configUSE_TIMERS = 1`. Three config
values govern it:

| Config | Meaning |
|---|---|
| `configTIMER_TASK_PRIORITY` | Priority of the daemon — set high so timers fire promptly |
| `configTIMER_QUEUE_LENGTH` | Depth of the command queue (`xTimerStart`/`Stop`/… are queued to the daemon) |
| `configTIMER_TASK_STACK_DEPTH` | Daemon stack — your callbacks run on it, so size for them |

**Consequence:** `xTimerStart` etc. don't act immediately — they **send a command** to the
daemon's queue. The `xTicksToWait` argument is how long to block if that command queue is
full, *not* the timer period.

---

### Timer Query & FromISR APIs

```c
BaseType_t   xTimerIsTimerActive(TimerHandle_t xTimer);        // pdTRUE if running
TickType_t   xTimerGetPeriod(TimerHandle_t xTimer);
TickType_t   xTimerGetExpiryTime(TimerHandle_t xTimer);        // tick at which it next fires
const char*  pcTimerGetName(TimerHandle_t xTimer);
void         vTimerSetTimerID(TimerHandle_t xTimer, void* pvNewID);
BaseType_t   xTimerDelete(TimerHandle_t xTimer, TickType_t xTicksToWait);

// ISR-safe control — all take pxHigherPriorityTaskWoken and need portYIELD_FROM_ISR after
BaseType_t xTimerStartFromISR(TimerHandle_t xTimer, BaseType_t* pxHigherPriorityTaskWoken);
BaseType_t xTimerStopFromISR(TimerHandle_t xTimer, BaseType_t* pxHigherPriorityTaskWoken);
BaseType_t xTimerResetFromISR(TimerHandle_t xTimer, BaseType_t* pxHigherPriorityTaskWoken);
BaseType_t xTimerChangePeriodFromISR(TimerHandle_t xTimer, TickType_t xNewPeriod, BaseType_t* pxHigherPriorityTaskWoken);
```

---

### `xTimerPendFunctionCall()` — Centralized Deferred Interrupt Processing

```c
// From a task
BaseType_t xTimerPendFunctionCall(
    PendedFunction_t xFunctionToPend,   // void fn(void* pvParam1, uint32_t ulParam2)
    void*            pvParameter1,
    uint32_t         ulParameter2,
    TickType_t       xTicksToWait
);

// From an ISR — run heavy/API-heavy work in the daemon task instead of the ISR
BaseType_t xTimerPendFunctionCallFromISR(
    PendedFunction_t xFunctionToPend,
    void*            pvParameter1,
    uint32_t         ulParameter2,
    BaseType_t*      pxHigherPriorityTaskWoken
);
```

**Why it matters (asked in interviews):** this is the cleanest **"deferred interrupt
processing"** mechanism. The ISR stays tiny — it just queues a function pointer + args to the
timer daemon, which then runs your handler in **task context** where you may call any
(non-FromISR) FreeRTOS API, allocate memory, log, etc. No dedicated task, no semaphore needed.

```c
void deferred_work(void *p1, uint32_t p2) {   // runs in daemon task, task context
    process_packet((packet_t *)p1, p2);
}

void IRAM_ATTR uart_isr(void) {
    BaseType_t woken = pdFALSE;
    xTimerPendFunctionCallFromISR(deferred_work, pkt, len, &woken);
    portYIELD_FROM_ISR(woken);
}
```

---

## 10. Stream Buffer

### Purpose
Pass continuous byte stream between ONE sender and ONE receiver. Optimized for variable-length data like serial/audio streams. Single producer, single consumer ONLY.

---

### `xStreamBufferCreate()`

```c
StreamBufferHandle_t xStreamBufferCreate(
    size_t xBufferSizeBytes,     // Total buffer size in bytes
    size_t xTriggerLevelBytes    // Minimum bytes before receiver unblocks
);
```

---

### Stream Buffer APIs

```c
size_t xStreamBufferSend(
    StreamBufferHandle_t xStreamBuffer,
    const void*          pvTxData,
    size_t               xDataLengthBytes,
    TickType_t           xTicksToWait
);

size_t xStreamBufferReceive(
    StreamBufferHandle_t xStreamBuffer,
    void*                pvRxData,
    size_t               xBufferLengthBytes,
    TickType_t           xTicksToWait
);

// FromISR variants — the classic "UART RX ISR fills, task drains" pattern
size_t xStreamBufferSendFromISR(StreamBufferHandle_t xSB, const void* pvTx, size_t xLen, BaseType_t* pxWoken);
size_t xStreamBufferReceiveFromISR(StreamBufferHandle_t xSB, void* pvRx, size_t xLen, BaseType_t* pxWoken);

// Utility / status
size_t     xStreamBufferBytesAvailable(StreamBufferHandle_t xSB);
size_t     xStreamBufferSpacesAvailable(StreamBufferHandle_t xSB);
BaseType_t xStreamBufferIsEmpty(StreamBufferHandle_t xSB);
BaseType_t xStreamBufferIsFull(StreamBufferHandle_t xSB);
BaseType_t xStreamBufferReset(StreamBufferHandle_t xSB);        // only when no task is blocked on it
BaseType_t xStreamBufferSetTriggerLevel(StreamBufferHandle_t xSB, size_t xTriggerLevelBytes);
void       vStreamBufferDelete(StreamBufferHandle_t xSB);
```

**Trigger level explained:** `xStreamBufferReceive` unblocks when **either** `xTriggerLevelBytes`
have arrived **or** the timeout expires — whichever comes first. Set it to 1 for lowest
latency, higher to batch and reduce wake-ups.

**Return value:** send/receive return the **number of bytes actually transferred**, which may
be *less* than requested on timeout — always use the return value, not the requested length.

**Complete UART RX example:**
```c
StreamBufferHandle_t xUartRx;   // created with xStreamBufferCreate(256, 1)

void IRAM_ATTR uart_rx_isr(void) {
    uint8_t b = UART_DR;
    BaseType_t woken = pdFALSE;
    xStreamBufferSendFromISR(xUartRx, &b, 1, &woken);
    portYIELD_FROM_ISR(woken);
}

void vUartTask(void *pv) {
    uint8_t buf[64];
    for (;;) {
        size_t n = xStreamBufferReceive(xUartRx, buf, sizeof(buf), pdMS_TO_TICKS(20));
        if (n) parse(buf, n);
    }
}
```

**Stream Buffer vs Queue:**

| | Stream Buffer | Queue |
|---|---|---|
| Data type | Raw bytes — variable length | Fixed size items |
| Multiple senders | NO — one sender only | YES |
| Overhead | Lower | Higher |
| Use case | UART receive, audio, serial | Structured messages between tasks |

---

## 11. Message Buffer

### Purpose
Pass discrete variable-length messages between ONE sender and ONE receiver. Each send/receive is one complete message with length header.

---

### `xMessageBufferCreate()`

```c
MessageBufferHandle_t xMessageBufferCreate(size_t xBufferSizeBytes);
```

```c
size_t xMessageBufferSend(
    MessageBufferHandle_t xMessageBuffer,
    const void*           pvTxData,
    size_t                xDataLengthBytes,
    TickType_t            xTicksToWait
);

size_t xMessageBufferReceive(
    MessageBufferHandle_t xMessageBuffer,
    void*                 pvRxData,
    size_t                xBufferLengthBytes,
    TickType_t            xTicksToWait
);

// FromISR variants + status
size_t xMessageBufferSendFromISR(MessageBufferHandle_t xMB, const void* pvTx, size_t xLen, BaseType_t* pxWoken);
size_t xMessageBufferReceiveFromISR(MessageBufferHandle_t xMB, void* pvRx, size_t xLen, BaseType_t* pxWoken);
size_t     xMessageBufferSpacesAvailable(MessageBufferHandle_t xMB);
BaseType_t xMessageBufferIsEmpty(MessageBufferHandle_t xMB);
BaseType_t xMessageBufferIsFull(MessageBufferHandle_t xMB);
BaseType_t xMessageBufferReset(MessageBufferHandle_t xMB);
void       vMessageBufferDelete(MessageBufferHandle_t xMB);
```

**Storage overhead:** each message is stored with a small **length header** (`size_t`, usually
4 bytes). A buffer of N bytes therefore holds *less* than N bytes of payload. Size the buffer
as `(max_msg_len + sizeof(size_t)) * expected_count`.

**Send returns 0** if the message (plus its header) doesn't fit before the timeout — the
message is **all-or-nothing**, never partially written. **Receive returns 0** if your
`xBufferLengthBytes` is smaller than the next message (the message stays in the buffer).

**Message Buffer vs Stream Buffer:**

| | Stream Buffer | Message Buffer |
|---|---|---|
| Message boundaries | NO — continuous byte stream | YES — each receive gets one complete message |
| Use case | Audio, raw serial | JSON packets, protocol frames |

---

## 12. Memory Management

### Purpose
Heap allocation for dynamic FreeRTOS objects.

---

### Heap Schemes — Choose One

| Scheme | Algorithm | Fragmentation | Free | Use When |
|---|---|---|---|---|
| heap_1 | Simple — no free | None | NO | Allocate at startup only |
| heap_2 | Best fit | YES | YES | Fixed size allocations |
| heap_3 | Wraps malloc/free | Depends on libc | YES | Already using malloc |
| heap_4 | First fit, coalesces adjacent | Minimal | YES | **Most common — general use** |
| heap_5 | heap_4 across multiple RAM regions | Minimal | YES | Multiple RAM banks (STM32H7) |

---

### Heap APIs

```c
void* pvPortMalloc(size_t xWantedSize);    // FreeRTOS heap allocation
void  vPortFree(void* pv);                  // FreeRTOS heap free

// Heap stats
size_t xPortGetFreeHeapSize(void);          // Current free heap bytes
size_t xPortGetMinimumEverFreeHeapSize();   // Minimum ever — use to size heap
```

**Production rule:** Allocate all objects at startup. Never call `pvPortMalloc()` in runtime task loops — fragmentation risk.

Total heap size is set by `configTOTAL_HEAP_SIZE` in `FreeRTOSConfig.h`. Right-size it by
running the app under load and reading `xPortGetMinimumEverFreeHeapSize()` — leave headroom.

---

### Malloc-Failed Hook

```c
// Enable with configUSE_MALLOC_FAILED_HOOK = 1. Called whenever pvPortMalloc returns NULL.
void vApplicationMallocFailedHook(void) {
    // Log free-heap stats, blink an error code, then reset — do NOT silently continue.
    configASSERT(0);
}
```

---

### Static Allocation Callbacks (required when `configSUPPORT_STATIC_ALLOCATION = 1`)

If you use static allocation, the kernel needs RAM for the **idle task** (and the **timer
task** if `configUSE_TIMERS = 1`) but cannot malloc it — so **you must provide** these
callbacks, or the link fails / the scheduler won't start:

```c
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t  **ppxIdleTaskStackBuffer,
                                   uint32_t      *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTCB;
    static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer   = &xIdleTCB;
    *ppxIdleTaskStackBuffer = xIdleStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t  **ppxTimerTaskStackBuffer,
                                    uint32_t      *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTCB;
    static StackType_t  xTimerStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer   = &xTimerTCB;
    *ppxTimerTaskStackBuffer = xTimerStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}
```

---

## 13. Critical Sections and Interrupt Control

### Purpose
Protect very short sequences that must not be interrupted. Disables interrupts — use minimally.

---

```c
// Task context — disables ALL interrupts up to configMAX_SYSCALL_INTERRUPT_PRIORITY
taskENTER_CRITICAL();
// VERY SHORT code here — never block, never call FreeRTOS blocking API
taskEXIT_CRITICAL();

// ISR context
taskENTER_CRITICAL_FROM_ISR();
taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

// Suspend scheduler without disabling interrupts
// Interrupts still fire but no context switch occurs
vTaskSuspendAll();
xTaskResumeAll();
```

**Critical Section vs Mutex:**

| | Critical Section | Mutex |
|---|---|---|
| Disables interrupts | YES | NO |
| Other tasks blocked | YES | Only tasks wanting same mutex |
| Duration | Microseconds max | Can be longer |
| Use case | Hardware register access, flag flip | Shared peripheral, data structure |

**Nesting:** `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` may be nested — interrupts are only
re-enabled when the outermost pair exits. The FromISR pair returns/consumes a saved status so
it nests safely across ISRs:

```c
UBaseType_t s = taskENTER_CRITICAL_FROM_ISR();
// touch shared data
taskEXIT_CRITICAL_FROM_ISR(s);
```

---

## 13b. Hook Functions (Callbacks)

### Purpose
User-supplied callbacks the kernel invokes at defined moments. Each is gated by a config
macro. Keep them short and non-blocking.

| Hook | Enable macro | Context | Use for |
|---|---|---|---|
| `vApplicationIdleHook()` | `configUSE_IDLE_HOOK` | Idle task | low-power `WFI`, cheap background work — **must not block** |
| `vApplicationTickHook()` | `configUSE_TICK_HOOK` | Tick ISR | tiny periodic bookkeeping — **FromISR APIs only, must be fast** |
| `vApplicationMallocFailedHook()` | `configUSE_MALLOC_FAILED_HOOK` | caller of malloc | catch out-of-heap |
| `vApplicationStackOverflowHook()` | `configCHECK_FOR_STACK_OVERFLOW` | context switch | catch stack overflow |
| `vApplicationDaemonTaskStartupHook()` | `configUSE_DAEMON_TASK_STARTUP_HOOK` | timer daemon task | one-time init that must run in a task after the scheduler starts |
| `vApplicationGetIdleTaskMemory()` | `configSUPPORT_STATIC_ALLOCATION` | startup | supply idle-task RAM |
| `vApplicationGetTimerTaskMemory()` | static + `configUSE_TIMERS` | startup | supply timer-task RAM |

```c
void vApplicationIdleHook(void) {
    __WFI();   // sleep the core until the next interrupt — cheap power win
}

void vApplicationTickHook(void) {
    // runs every tick, in interrupt context — keep it to a few instructions
}
```

---

## 13c. FreeRTOSConfig.h Reference

The single most important file — it tailors the kernel to your app. Key macros grouped:

### Scheduling
| Macro | Typical | Meaning |
|---|---|---|
| `configUSE_PREEMPTION` | 1 | 1 = preemptive, 0 = cooperative (yield-only) |
| `configUSE_TIME_SLICING` | 1 | round-robin among equal-priority Ready tasks |
| `configMAX_PRIORITIES` | 5–32 | number of priority levels (0..N-1). RAM cost per level |
| `configTICK_RATE_HZ` | 1000 | tick frequency; `pdMS_TO_TICKS` uses it |
| `configUSE_16_BIT_TICKS` | 0 | 0 → 32-bit `TickType_t` (longer max delay) |
| `configIDLE_SHOULD_YIELD` | 1 | idle yields to equal-prio (=0) app tasks promptly |

### Memory
| Macro | Meaning |
|---|---|
| `configSUPPORT_DYNAMIC_ALLOCATION` | enable `xTaskCreate` etc. |
| `configSUPPORT_STATIC_ALLOCATION` | enable `...Static` creators |
| `configTOTAL_HEAP_SIZE` | heap bytes for heap_1/2/4 |
| `configMINIMAL_STACK_SIZE` | idle-task stack (words); baseline for your own |

### Features (compile OUT what you don't use to save flash/RAM)
| Macro | Enables |
|---|---|
| `configUSE_MUTEXES` | mutexes + priority inheritance |
| `configUSE_RECURSIVE_MUTEXES` | recursive mutexes |
| `configUSE_COUNTING_SEMAPHORES` | counting semaphores |
| `configUSE_TIMERS` | software timers + daemon task |
| `configUSE_QUEUE_SETS` | queue sets |
| `configUSE_TASK_NOTIFICATIONS` | task notifications (on by default) |
| `configTASK_NOTIFICATION_ARRAY_ENTRIES` | number of indexed notifications per task |
| `INCLUDE_vTaskDelete`, `INCLUDE_vTaskSuspend`, `INCLUDE_xTaskGetSchedulerState`, … | pull in individual optional APIs |

### Interrupts (Cortex-M — get these wrong and you hard-fault)
| Macro | Meaning |
|---|---|
| `configPRIO_BITS` | priority bits implemented by your MCU (e.g. 4 on many STM32) |
| `configKERNEL_INTERRUPT_PRIORITY` | priority of the tick/PendSV (lowest) |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | **highest** priority from which `...FromISR` APIs may be called — see §14 |

### Debug / stats
| Macro | Meaning |
|---|---|
| `configCHECK_FOR_STACK_OVERFLOW` | 0/1/2 — overflow detection method |
| `configUSE_TRACE_FACILITY` | extra TCB fields for `vTaskList`, tracers |
| `configGENERATE_RUN_TIME_STATS` | enable `vTaskGetRunTimeStats` |
| `configASSERT(x)` | **define this** — catches kernel misuse early; disable in release |

---

## 14. Interview Topics to Know Cold

### FromISR Rule — Always Asked

Any FreeRTOS API called from Interrupt Service Routine context MUST use the `FromISR` version. Standard APIs can block — illegal in ISR. FromISR APIs never block.

```c
// Task context          ISR context
xQueueSend()       →    xQueueSendFromISR()
xSemaphoreGive()   →    xSemaphoreGiveFromISR()
xTaskNotifyGive()  →    vTaskNotifyGiveFromISR()
xEventGroupSetBits() →  xEventGroupSetBitsFromISR()
```

Always follow with `portYIELD_FROM_ISR(pxHigherPriorityTaskWoken)`.

---

### vTaskDelay() vs vTaskDelayUntil() — Always Asked

```c
// vTaskDelay — delay FROM NOW — period drifts
// Period = execution time + delay time

// vTaskDelayUntil — delay UNTIL absolute tick — period fixed
// Period = constant regardless of execution time
// Use for: motor control, sensor sampling, anything needing fixed frequency
```

---

### Priority Inversion — Always Asked

Low priority task holds Mutex → High priority task blocks on Mutex → Medium priority task preempts low priority task → High priority task starved.

**Fix:** Mutex with Priority Inheritance — FreeRTOS `xSemaphoreCreateMutex()` does this automatically. Low priority task priority raised to match highest waiter immediately on contention.

---

### Deadlock Prevention — Always Asked

Two techniques practical in embedded:

1. **Lock ordering** — always acquire mutexes in same global order across all tasks.
2. **Timeout with backoff** — use `xTicksToWait` instead of `portMAX_DELAY`, release and retry.

---

### Stack Overflow Detection — Always Asked

Enable in `FreeRTOSConfig.h`:

```c
#define configCHECK_FOR_STACK_OVERFLOW 1  // Stack pointer boundary check
#define configCHECK_FOR_STACK_OVERFLOW 2  // Stack paint pattern (0xA5) check — more reliable

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    // Log task name, halt system
    configASSERT(0);
}
```

**Method 1 vs Method 2:**
- **Method 1** checks, at each context switch, whether the stack pointer is past the end of the
  task's stack. Fast, but misses overflows that happened *and returned* between switches.
- **Method 2** additionally checks that the last 16 bytes, pre-painted with `0xA5A5A5A5`, are
  intact. Catches deeper/transient overflows Method 1 misses, at a small extra cost. **Prefer
  Method 2** in development.

---

### PendSV & Context Switch Internals (Cortex-M) — HIGH

**What happens on a switch:**
1. The CPU automatically stacks 8 registers on an exception entry (**R0–R3, R12, LR, PC, xPSR**) —
   the "hardware-saved" frame.
2. The FreeRTOS **PendSV handler** (`xPortPendSVHandler`) then manually saves the remaining
   **R4–R11** onto the *current* task's stack, stores that task's stack pointer into its TCB,
   calls `vTaskSwitchContext()` to pick the next task, loads the new task's SP, and restores
   its R4–R11.
3. On exception return, the hardware automatically restores R0–R3/R12/LR/PC/xPSR of the new
   task — and execution resumes exactly where that task left off.

**Why PendSV specifically:**
- PendSV is set to the **lowest** interrupt priority, so it only runs after all other ISRs
  finish — a context switch never preempts a real device ISR mid-flight.
- The switch is *requested* (by setting the PendSV pending bit) from the tick ISR or from any
  API that unblocks a higher-priority task; PendSV then performs it at a safe moment. This
  cleanly decouples "decide to switch" from "perform the switch" and avoids re-entrancy.
- SysTick drives the tick; `SVC` is used once to start the first task.

---

### configMAX_SYSCALL_INTERRUPT_PRIORITY — HIGH

Defines the **highest** interrupt priority from which it is legal to call `...FromISR` APIs.

- ISRs at or **below** this priority (numerically ≥, since lower number = higher priority on
  Cortex-M) may call FreeRTOS FromISR APIs. The kernel masks interrupts up to this level inside
  critical sections (via `BASEPRI`), so those ISRs can be briefly deferred safely.
- ISRs **above** it (higher priority, smaller number) must **never** call any FreeRTOS API — but
  they enjoy zero kernel-induced latency (great for time-critical motor/PWM ISRs).

**Cortex-M gotcha:** these macros are written in terms of the hardware priority register, which
puts the priority in the **upper** `configPRIO_BITS`. So a "logical" priority 5 with 4 prio
bits is written `5 << (8 - 4) = 0x50`. Getting the shift wrong is the #1 cause of the
"works until an interrupt fires, then HardFault / `configASSERT` trips in `port.c`" bug.

```c
// Example for an MCU with 4 priority bits:
#define configPRIO_BITS                          4
#define configKERNEL_INTERRUPT_PRIORITY          (15 << 4)  // lowest — tick/PendSV
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (5  << 4)  // FromISR allowed at prio 5..15
```

---

### Tickless Idle (Low Power) — MEDIUM

When enabled (`configUSE_TICKLESS_IDLE = 1`), if the idle task sees that **all** tasks will be
blocked for many ticks, it stops the periodic tick, puts the core into deep sleep for (nearly)
that whole span, then on wake **corrects the tick count** by the elapsed time. Net effect: the
MCU sleeps for long stretches instead of waking 1000×/s just to service empty ticks — critical
for coin-cell IoT devices.

- The kernel calls `portSUPPRESS_TICKS_AND_SLEEP(xExpectedIdleTime)`; ports override
  `vPortSuppressTicksAndSleep` for their sleep mode.
- On **ESP32**, "light sleep" retains RAM and resumes fast; "deep sleep" powers down and
  effectively reboots — tickless idle maps to light sleep.
- Tune `configEXPECTED_IDLE_TIME_BEFORE_SLEEP` (minimum idle ticks before it bothers sleeping).

---

### Priority Ceiling (manual) — MEDIUM

FreeRTOS mutexes implement **priority inheritance**, not **priority ceiling**. To emulate a
ceiling protocol, raise a task to the resource's ceiling priority *before* taking it and
restore afterward:

```c
UBaseType_t saved = uxTaskPriorityGet(NULL);
vTaskPrioritySet(NULL, RESOURCE_CEILING_PRIO);   // climb to ceiling
xSemaphoreTake(xMutex, portMAX_DELAY);
// ... critical work ...
xSemaphoreGive(xMutex);
vTaskPrioritySet(NULL, saved);                   // restore
```

This bounds priority inversion to a single critical section and prevents deadlock among tasks
that share the same ceiling — at the cost of manual bookkeeping.

---

### Choosing the Right Primitive

| Scenario | Use |
|---|---|
| ISR signals one known task | Task Notification (`vTaskNotifyGiveFromISR`) |
| ISR signals multiple tasks | Binary Semaphore |
| Pass data between tasks | Queue |
| Share latest sensor value | Mailbox (`xQueueOverwrite` + `xQueuePeek`) |
| Protect shared peripheral | Mutex |
| Count available resources | Counting Semaphore |
| Wait for multiple events AND/OR | Event Group |
| Periodic callback, no dedicated task | Software Timer |
| Raw byte stream (UART, audio) | Stream Buffer |
| Variable length discrete messages | Message Buffer |
| Zero RAM overhead signaling | Task Notification |

---

## 15. Study Notes

Topics originally identified as gaps — **now covered in §14** above. Use this as a checklist:

| Topic | Priority | Covered in |
|---|---|---|
| PendSV interrupt — context switch internals | HIGH | §14 → *PendSV & Context Switch Internals* |
| Tickless Idle — deep dive | MEDIUM | §14 → *Tickless Idle* |
| Priority Ceiling — manual implementation | MEDIUM | §14 → *Priority Ceiling (manual)* |
| Stack Overflow Detection Method 1 vs Method 2 | REVIEW | §14 → *Stack Overflow Detection* |
| Task Notification — all eNotifyAction types | MEDIUM | §7 → *eNotifyAction — all five actions* |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | HIGH | §14 → *configMAX_SYSCALL_INTERRUPT_PRIORITY* |

**Further self-test questions:**
- Difference between binary semaphore, mutex, and a task notification used as a signal?
- When does `xQueueSend` copy vs when should you queue a pointer?
- Why must the idle task be allowed to run? What breaks if it can't?
- What is priority inversion, and exactly how does inheritance fix it?
- Why can't you call `xQueueSend` (non-ISR) from an ISR? What actually goes wrong?
- Difference between suspending the scheduler and a critical section?
- SMP (dual-core ESP32): what does `tskNO_AFFINITY` mean and when do you pin a task?

---

## Appendix A — Data Types

| Type | Meaning |
|---|---|
| `BaseType_t` | Most efficient int for the architecture (32-bit on Cortex-M, 8-bit on AVR). Used for booleans and small counts. |
| `UBaseType_t` | Unsigned `BaseType_t`. Priorities, counts. |
| `TickType_t` | Tick counter — 16- or 32-bit per `configUSE_16_BIT_TICKS`. |
| `TaskHandle_t` | Opaque handle to a task. |
| `QueueHandle_t` | Opaque handle to a queue. |
| `SemaphoreHandle_t` | Opaque handle to any semaphore/mutex. |
| `EventGroupHandle_t` | Opaque handle to an event group. |
| `TimerHandle_t` | Opaque handle to a software timer. |
| `StreamBufferHandle_t` / `MessageBufferHandle_t` | Opaque handles to stream/message buffers. |
| `StackType_t` | Word type of a stack slot (`uint32_t` on Cortex-M). |
| `StaticTask_t`, `StaticQueue_t`, … | Storage structs for static creation. |

**Constants you'll use constantly:** `pdTRUE`(1), `pdFALSE`(0), `pdPASS`(1), `pdFAIL`(0),
`portMAX_DELAY`, `tskIDLE_PRIORITY`(0), `configMINIMAL_STACK_SIZE`, `pdMS_TO_TICKS(ms)`,
`portTICK_PERIOD_MS`, `tskNO_AFFINITY` (SMP).

---

## Appendix B — Return Codes

| Code | Value | Where |
|---|---|---|
| `pdPASS` / `pdTRUE` | 1 | success / true |
| `pdFAIL` / `pdFALSE` | 0 | failure / false / timeout |
| `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY` | -1 | create-* out of heap |
| `errQUEUE_FULL` | 0 | `xQueueSend` timed out (queue stayed full) |
| `errQUEUE_EMPTY` | 0 | `xQueueReceive` timed out (queue stayed empty) |
| `errQUEUE_BLOCKED` / `errQUEUE_YIELD` | internal | scheduler-internal, not user-facing |

**Habit:** compare against the named constant (`== pdTRUE`), not against `0`/`1` literals.

---

## Appendix C — Common Bugs & Gotchas

| Symptom | Likely cause |
|---|---|
| HardFault the moment an interrupt fires | ISR priority above `configMAX_SYSCALL_INTERRUPT_PRIORITY`, or wrong priority-bit shift |
| Task never runs | Higher-priority task never blocks (starvation); or you forgot `vTaskStartScheduler()` |
| Deleted tasks leak RAM | Idle task starved — no task ever blocks/delays |
| Random corruption / crash after a while | Stack too small — enable `configCHECK_FOR_STACK_OVERFLOW = 2`, check high-water marks |
| Queue "loses" data | Queued a pointer to a stack local that went out of scope; or sender overran a full queue and ignored the return code |
| `xQueueSend` from ISR does nothing / asserts | Used the non-ISR API in an ISR — must use `...FromISR` |
| Higher-priority task not woken after ISR | Forgot `portYIELD_FROM_ISR(pxHigherPriorityTaskWoken)` |
| Mutex "give" from ISR fails | Mutexes have ownership — can't be given from an ISR; use a binary semaphore |
| Periodic task drifts | Used `vTaskDelay` instead of `vTaskDelayUntil` |
| Timer callback hangs the system | Blocked inside a timer callback — they run on the shared daemon task |
| `configASSERT` fires in `port.c`/`queue.c` | Almost always an interrupt-priority misconfiguration or calling an API from the wrong context |
| Works in debug, fails in release | `configASSERT` compiled out was hiding a real misuse; fix the root cause |

---

*FreeRTOS API Reference — expanded into a complete guide. Covers classic single-core kernel
plus ESP-IDF/SMP notes. Verify port-specific details against your `FreeRTOSConfig.h` and the
official docs at freertos.org.*