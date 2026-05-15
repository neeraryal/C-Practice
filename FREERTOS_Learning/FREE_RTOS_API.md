# FreeRTOS API Reference — Interview Ready

> Complete topic-wise API reference with parameters, purpose, use cases, and alternatives.

---

## Table of Contents

1. [Task Management](#1-task-management)
2. [Queue](#2-queue)
3. [Semaphore — Binary](#3-semaphore--binary)
4. [Semaphore — Counting](#4-semaphore--counting)
5. [Mutex](#5-mutex)
6. [Mailbox](#6-mailbox)
7. [Task Notifications](#7-task-notifications)
8. [Event Groups / Event Bits](#8-event-groups--event-bits)
9. [Software Timers](#9-software-timers)
10. [Stream Buffer](#10-stream-buffer)
11. [Message Buffer](#11-message-buffer)
12. [Memory Management](#12-memory-management)
13. [Critical Sections and Interrupt Control](#13-critical-sections-and-interrupt-control)
14. [Interview Topics to Know Cold](#14-interview-topics-to-know-cold)
15. [Study Notes](#15-study-notes)

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

**Use when:** Dynamic allocation acceptable. Simpler, most common usage.

**Fails when:** Heap exhausted — returns error instead of crashing.

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
```

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

Topics identified as gaps during interview session — prioritize these:

| Topic | Priority | Why |
|---|---|---|
| PendSV interrupt — context switch internals | HIGH | Asked in every senior embedded interview. How exactly registers saved/restored, why PendSV specifically chosen over other interrupts |
| Tickless Idle — deep dive | MEDIUM | Internet of Things battery powered device interviews. How tick count corrected on wake, light sleep vs deep sleep on ESP32 |
| Priority Ceiling — manual implementation | MEDIUM | FreeRTOS does not have built-in Priority Ceiling. Know how to implement with `vTaskPrioritySet()` |
| Stack Overflow Detection Method 1 vs Method 2 | REVIEW | Know difference — pointer check vs paint pattern, why Method 2 catches more cases |
| Task Notification — all eNotifyAction types | MEDIUM | Lightweight alternative to semaphore/queue — interviewers test this specifically |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | HIGH | Wrong value causes hard fault. Know what it means and how to set correctly on Cortex-M |

---

*FreeRTOS API Reference — compiled from interview session*