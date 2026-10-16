# FreeRTOS API Notes — Hinglish Interview Edition

> **Goal:** Ratna nahi, samajhna: **problem → task state → right primitive → API parameters → code → failure case → counter-question**.
> Companion: [40 scenario-based questions](./FREE_RTOS_SCENARIOS.md).
> Examples assume standard FreeRTOS C headers; **ESP-IDF 5.x / ESP32 differences** specially marked. Real project mein `FreeRTOSConfig.h` aur port docs check karo.

## Reading order (60-second map)
1. [Foundations, ticks, scheduler](#1-foundations-ticks--scheduler)
2. [Task creation and life-cycle](#2-task-creation--life-cycle)
3. [Blocking and periodic timing](#3-blocking--periodic-timing)
4. [Queues / producer-consumer](#4-queues--producer-consumer)
5. [Binary & counting semaphores](#5-binary--counting-semaphores)
6. [Mutex, recursive mutex, priority inversion](#6-mutex--recursive-mutex--priority-inversion)
7. [Direct task notifications](#7-direct-task-notifications)
8. [Event groups](#8-event-groups)
9. [Software timers](#9-software-timers)
10. [Stream/message buffers, mailbox, queue sets](#10-stream-message-buffers-mailbox--queue-sets)
11. [ISR & context switches](#11-isr--context-switches)
12. [Memory, stack, debugging, configuration](#12-memory-stack-debugging--configuration)
13. [Decision matrix, common mistakes](#13-quick-decision-matrix)
14. [Rapid interview & counter-questions](#14-rapid-fire-interview--counter-questions)
15. [Further reading](#15-official-references)

---

## 1. Foundations, ticks & scheduler

**FreeRTOS kya karta hai?** Kernel tasks ko schedule karta hai, unke beech data pass/synchronize karata hai, timers aur memory utilities provide karta hai. Task = independent function + stack + TCB. Standard FreeRTOS embedded kernel *by itself* file system ya TCP/IP stack nahi hota.

**Scheduler ka simple rule (single-core preemptive):** Highest-priority **Ready** task CPU lega. High-priority task agar block nahi hota aur continuously run karta rahe to lower-priority task starve ho sakta hai. Equal priority tasks time-slicing se CPU share kar sakte hain jab `configUSE_TIME_SLICING` enabled ho. **ESP32 SMP mein multi-core scheduling/affinity ke additional rules hain.**

### Task state ko story ki tarah samjho

| State | Meaning | Typical transition |
|---|---|---|
| Running | Abhi CPU pe execute ho raha hai | preemption / yield / blocking call |
| Ready | CPU ke liye eligible, turn ka wait | scheduler selects |
| Blocked | Time/event ka wait; **CPU waste nahi hota** | delay expires / queue item / semaphore / timeout |
| Suspended | Explicitly suspend kiya; normal timeout se automatically ready nahi | `vTaskResume` / `xTaskResumeFromISR` |
| Deleted | Task finish; kernel cleanup may be deferred | `vTaskDelete` |

**Important:** `xQueueSend(q, &v, 0)` fail hone se task **automatically Blocked nahi hota**. Zero wait means immediately return. `vTaskDelay()` ke andar jaate hi specified tick delay (nonzero) ke liye task Blocked hota hai. High-priority busy-loop CPU occupy karega, even if queue full.

### Tick frequency, interval, tick count — alag concepts

- `configTICK_RATE_HZ = 100` → **100 ticks/second**, tick interval **10 ms**.
- `configTICK_RATE_HZ = 1000` → **1000 ticks/second**, tick interval **1 ms**.
- `xTaskGetTickCount()` → scheduler ke tick counter ka current value; **milliseconds nahi**.
- `pdMS_TO_TICKS(500)` → 500 ms ko configured ticks mein convert karta hai; 100 Hz pe 50 ticks.
- `portTICK_PERIOD_MS` = tick interval in milliseconds (integer macro; sub-1-ms tick / some ports pe unsuitable ho sakta hai).
- Tick-based delay *minimum exact execution time guarantee* nahi: interrupts, priorities, scheduling aur conversion round-off matter karte hain.
- Tickless idle mein suppressed periodic tick interrupts ke bawajood kernel elapsed ticks account karta hai.

```c
// Maan lo tick rate 100 Hz hai: one tick = 10ms.
// Current scheduler tick lo; yeh milliseconds nahi hai.
TickType_t now = xTaskGetTickCount();  // output TICKS mein
// 500 milliseconds ko tick units mein convert kar rahe hain.
TickType_t gap = pdMS_TO_TICKS(500);   // 50 ticks
// Task ab delay ke liye Blocked, baaki Ready tasks CPU use kar sakti hain.
vTaskDelay(gap);                       // lagbhag 500ms Blocked (tick granularity)
```

**Counter:** `vTaskDelay(500 / portTICK_PERIOD_MS)` kab zero ho sakta hai? Jab requested milliseconds tick period se chhote hon. Zero tick ka matlab actual wait nahi. `pdMS_TO_TICKS` use karo aur minimum one tick required ho to consciously handle karo.

### Preemptive vs cooperative

- `configUSE_PREEMPTION = 1`: higher-priority task ready hote hi scheduler switch request kar sakta hai. ISR se immediate switch on exit ke liye port yield macro important.
- Cooperative mode mein running task voluntarily yield/block karegi; **tick aana alone arbitrary preemption guarantee nahi**.
- `taskYIELD()` ready task ko chance deta hai, **Blocked state nahi** banata. Higher-priority task continuously Ready hai to lower-priority task ko yield se help nahi milegi.

### Task vs ISR vs timer callback

| Context | Allowed thinking |
|---|---|
| Normal task | Block, queue/semaphore take, mutex lock, delay (context ke hisaab se) |
| ISR | Short/fast, only `...FromISR` APIs, **no blocking**, wake flag + port-specific yield |
| Software timer callback | Timer service task mein chalti hai; **blocking avoid**, lamba work worker task ko bhejo |

---

## 2. Task creation & life-cycle

**Interview story:** Sensor reading aur LED control ko alag tasks chahiye. Dono ke independent stacks honge; scheduler priorities/ready state ke hisaab se chalata hai.

### `xTaskCreate()` — ALL 6 PARAMETERS (most important)

```c
BaseType_t xTaskCreate(
    // 1: Task function ka address; function signature void task(void*) honi chahiye.
    TaskFunction_t pvTaskCode,     // entry function
    // 2: Debug aur logs mein dikhne wala task name.
    const char *pcName,           // task name
    // 3: Task stack size; vanilla FreeRTOS words, ESP-IDF bytes.
    configSTACK_DEPTH_TYPE usStackDepth, // stack size: units port ke hisaab se
    // 4: Task ko pass kiya hua object pointer, object ki lifetime valid ho.
    void *pvParameters,           // task data pointer
    // 5: Priority; higher number = higher priority, allowed range ke andar.
    UBaseType_t uxPriority,       // priority
    // 6: Task handle store karne ka output pointer; zarurat na ho to NULL.
    TaskHandle_t *pxCreatedTask   // output task handle pointer
);
```

| Param | Exactly kya? | Why / interview trap |
|---|---|---|
| `pvTaskCode` | `void task(void *arg)` compatible function pointer | Kernel isi entry function ko task context mein run karega; task must not casually return |
| `pcName` | Human-readable task name | Debug/log; max configured length |
| `usStackDepth` / `ulStackDepth` | Stack size | **Vanilla FreeRTOS: StackType_t words; ESP-IDF: bytes** (IDF docs/version confirm). Stack heap allocation se separate concept hai |
| `pvParameters` | Task ko pass hone wala pointer | Address of stable object pass karo; caller ke expired local variable ka pointer use mat karo |
| `uxPriority` | Task priority, higher number = higher priority | Importance nahi, responsiveness/deadline ke hisaab se choose karo |
| `pxCreatedTask` | TaskHandle_t receive karne ka out pointer | Later notify/delete/priority/change ke liye; handle unnecessary ho to `NULL` |

**Return:** `pdPASS` success; otherwise allocation/config related failure (often `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`).

```c
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>

// Later notification/suspend/debug ke liye task handle save hoga.
static TaskHandle_t sensorHandle = NULL;
// Static ID ka address valid rahega; local temporary address mat dena.
static int sensorId = 42; // task ke jeevan tak valid rehna chahiye

static void sensorTask(void *arg) {
    // void* argument ko int* cast karke pointed ID read karo.
    int id = *(int *)arg;
    // Task continuously service karega; task function unexpectedly return nahi karega.
    for (;;) {
        // Tick count print karenge, milliseconds nahi.
        printf("sensor %d at tick %lu\n", id,
               (unsigned long)xTaskGetTickCount());
        // 500ms equivalent ticks tak ye task Blocked rahegi.
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void startTasks(void) {
    // xTaskCreate ke 6 arguments: entry, name, stack, data, priority, handle.
    BaseType_t ok = xTaskCreate(sensorTask, "sensor", 2048,
                                &sensorId, 3, &sensorHandle);
    // Creation fail hua to turant debug assertion mile.
    configASSERT(ok == pdPASS);
}
// Dhyan: ESP-IDF mein stack size BYTES, vanilla FreeRTOS mein STACK WORDS hai.
```

**Follow-ups:** `pvParameters=(void *)1` possible? Haan agar consciously integer encode kar rahe ho, but portable C mein `(intptr_t)` cast carefully use karo; normal data ke liye actual object pointer safer. `Node*` pass karoge to function mein `(Node *)arg` cast karo.

### `xTaskCreatePinnedToCore()` (ESP-IDF)
```c
// ESP-IDF: last argument 1 means CPU core 1; 4096 stack BYTES hain.
xTaskCreatePinnedToCore(sensorTask, "sensor", 4096,
                        // Pointer, priority, handle aur target core specify kar rahe hain.
                        &sensorId, 5, &sensorHandle, 1);
```
Same six params + `xCoreID` (core 0/1, or `tskNO_AFFINITY`). **Use when:** specific core affinity/driver constraint/latency analysis. **Not magic:** core pin karne se shared locks, interrupts, cache contention automatically solve nahi hote.

### `xTaskCreateStatic()`
Dynamic heap avoid karna ho to application stack buffer + TCB provide karegi. Params: `pvTaskCode`, `pcName`, `ulStackDepth`, `pvParameters`, `uxPriority`, `pxStackBuffer` (caller-owned stack), `pxTaskBuffer` (caller-owned TCB). Returns `TaskHandle_t` or `NULL`. Arrays/storage task lifetime tak valid hone chahiye; enable static allocation in config. ESP-IDF size interpretation differs; static stack buffer element requirements port documentation se verify karo.

### `vTaskDelete()`
`vTaskDelete(NULL)` means **current task ko delete karo**, handle pass means target task. No immediate blanket guarantee of free RAM: dynamically allocated task resources may need idle-task cleanup; don't starve idle. Ownership of app-owned mallocs, peripherals, mutexes and handles **automatically** clean nahi hoti.

### `vTaskSuspend()` / `vTaskResume()`
Param `xTaskToSuspend` / `xTaskToResume` handle; `NULL` with suspend = self. Use administrative pause; **event wait ke liye suspend/resume mat choose karo** (lost-wakeup races). Queue/semaphore/notification stronger pattern.

### `vTaskPrioritySet()` / `uxTaskPriorityGet()`
Params (task handle, new priority) / (task handle). Handle `NULL` = current task. Priority dynamic update rarely needed; mutating priority to fix design without timing analysis risky.

### `xTaskGetCurrentTaskHandle()` / `pcTaskGetName()`
Current task ka handle / named task ka label for diagnostics. `pcTaskGetName(xTask)` uses target handle; some APIs `NULL` as current task accept karte hain; docs check.

---

## 3. Blocking & periodic timing

### `vTaskDelay(xTicksToDelay)`
Ek param: number of **ticks**. `vTaskDelay(pdMS_TO_TICKS(100))` = next ~100 ms later **Ready** hone ka request; CPU pe exactly us moment Running guarantee nahi. Example: 100ms processing + 500ms delay → cycle **~600ms**, drift possible.

### `vTaskDelayUntil(&xLastWakeTime, xFrequency)`
- `pxPreviousWakeTime`: pointer to last scheduled wake tick; initialized once via `xTaskGetTickCount()`; API updates it.
- `xTimeIncrement`: fixed period in ticks.
- Better when **every 100 ms periodic loop** chahiye; absolute schedule avoids accumulating execution-time drift.
- Missed deadline? Calculated wake already past, may return immediately; do not claim hard real-time 1kHz guarantee from tick loop.

```c
static void controlTask(void *arg) {
    // Is demo mein passed argument use nahi ho raha.
    (void)arg;
    // Last scheduled wake tick sirf ek baar initialize hota hai.
    TickType_t last = xTaskGetTickCount();
    // 100ms ko ticks mein convert karke fixed period banao.
    const TickType_t period = pdMS_TO_TICKS(100);
    // Tick conversion zero na ho jaaye, isliye safety check.
    configASSERT(period > 0);
    for (;;) {
        // Control work ko apne execution budget mein finish karna hoga.
        read_and_control(); // execution budget ke andar complete hona chahiye
        // Fixed schedule maintain karke repeated vTaskDelay wali drift avoid karta hai.
        vTaskDelayUntil(&last, period);
    }
}
```

For newer FreeRTOS versions `xTaskDelayUntil()` returns status; `vTaskDelayUntil()` widely used. Enable appropriate INCLUDE flag. < 1-tick resolution or very tight jitter demands hardware timer/peripheral, measure WCET.

### Timeout param pattern you must know

Almost every blocking API has `xTicksToWait` / `xBlockTime`:
- `0` = **don't wait**, immediate success/fail.
- `pdMS_TO_TICKS(50)` = wait **up to** ~50ms when resource unavailable.
- `portMAX_DELAY` = long/indefinite wait **only under relevant config**, e.g. `INCLUDE_vTaskSuspend` matters for indefinite blocking behavior. On finite tick modes behavior/config differs.
- Wake by event, timeout, or relevant task state operation; API return value tells what happened.

### `xTaskGetTickCount()`, `xTaskGetTickCountFromISR()`
Return `TickType_t`. First for task context, second for ISR (if supported by port). Tick count can wrap; calculate time deltas using unsigned tick arithmetic carefully; avoid naïve comparisons across rollover.

### `taskYIELD()`
No args, no blocking; current task voluntarily yields. **Difference:** `vTaskDelay(1)` actually waits at least until next tick; `taskYIELD()` doesn't. **ISR yield macros are distinct**.

---

## 4. Queues & producer-consumer

**Queue kab?** Producer data items bana raha hai, consumer another task process karega; har item ki identity/payload zaroori, burst absorb karna hai. Queue item ko **copy** karti hai, pointer deep-copy nahi hota.

```c
// Queue har Sample ke sizeof(Sample) bytes copy karegi.
typedef struct { uint32_t seq; int temperature; } Sample;
// Shared queue handle producer aur consumer dono use karenge.
static QueueHandle_t q;

static void producer(void *arg) {
    (void)arg;
    // Producer ka local variable safe hai kyunki queue VALUE copy karegi.
    Sample s = {0};
    for (;;) {
        // Har reading ko sequence ID de kar dropped samples spot kar sakte hain.
        s.seq++;
        // Yahan dummy sensor value hai; real project mein sensor read hoga.
        s.temperature = 25;
        // Third param 0: full queue par turant FAIL, producer SEND se Blocked nahi hota.
        if (xQueueSend(q, &s, 0) != pdPASS) {
            // Full queue: drop count, bounded retry ya backpressure decide karo.
        }
        // Producer 10ms delay se CPU release karta hai; ye line Blocked banati hai.
        vTaskDelay(pdMS_TO_TICKS(10)); // har 10 milliseconds, 10 seconds nahi
    }
}

static void consumer(void *arg) {
    (void)arg;
    // Consumer output ke liye enough storage provide karta hai.
    Sample item;
    for (;;) {
        // Empty queue par consumer Blocked; item milte hi dequeue aur copy.
        if (xQueueReceive(q, &item, portMAX_DELAY) == pdPASS) {
            // Consumer actual data processing karega.
            process_sample(item);
            // Consumer ko intentionally slow banaya: 100ms/item vs 10ms/item.
            vTaskDelay(pdMS_TO_TICKS(100)); // demo ka jaanbujhkar slow consumer
        }
    }
}

void demo(void) {
    // Queue capacity 5 aur item size sizeof(Sample) hai.
    q = xQueueCreate(5, sizeof(Sample));
    // Handle NULL ho to allocation fail hua.
    configASSERT(q);
    // Producer priority 3: bina delay ka busy loop lower tasks ko starve karega.
    xTaskCreate(producer, "producer", 2048, NULL, 3, NULL);
    // Consumer priority 2 hai, producer ki blocking se run ka chance milega.
    xTaskCreate(consumer, "consumer", 2048, NULL, 2, NULL);
}
```

**Trace:** Producer every 10 ms, consumer consumes one every 100 ms. Queue capacity 5 eventually full. `xQueueSend(q,&s,0)` **returns immediately with `errQUEUE_FULL` / `pdFAIL`**; producer does **not** block on send. If producer high priority and replaces delay with endless retry, consumer lower priority **starve** ho sakta hai → queue kabhi drain hi nahi hogi. Policy: finite block timeout, data drop/backpressure, or consumer redesign.

**Counter:** `xQueueReceive` with wait while queue empty → consumer **Blocked**. `xQueueSend` with nonzero wait while full → producer **Blocked** until space/timeout. Both states depend on queue condition AND timeout. `xQueueReceive` removes element, `xQueuePeek` does not.

### Queue memory/copy model
`xQueueCreate(5,sizeof(Sample))` allocates storage for up to five **value copies**, plus control metadata. Queue-pointer pattern `xQueueCreate(5,sizeof(Sample *))` queues addresses, so pointee must remain valid until consumer returns buffer; use static objects / pool + ownership protocol, never pointer to expired stack local.

### APIs: quick choices
- `xQueueSendToBack` / `xQueueSend`: normal FIFO enqueue.
- `xQueueSendToFront`: urgent item front (beware unfairness).
- `xQueueReceive`: dequeue + copy into caller's output.
- `xQueuePeek`: inspect without removal.
- `uxQueueMessagesWaiting`: momentary count (race if used for correctness).
- `xQueueReset`: discard queued items; synchronize callers before reset.
- `vQueueDelete`: cleanup, only when nobody still uses the queue.
- `xQueueSendFromISR` / `xQueueReceiveFromISR`: ISR-safe, **no wait argument**.
- `xQueueOverwrite`: only **length-1 queue / mailbox**, latest value wins.

---

## 5. Binary & counting semaphores

**Semaphore gives permission/event; payload nahi.**

### Binary semaphore (0/1)
`xSemaphoreCreateBinary()` no params; newly created semaphore initially unavailable, so task waits until `Give`. `xSemaphoreTake(sem, ticks)` acquires; `xSemaphoreGive(sem)` signals. Use e.g. GPIO ISR -> worker task. Multiple gives while full **collapse**, no event count; from ISR use `xSemaphoreGiveFromISR`. **No ownership, no priority inheritance.** Do **not** replace mutex for priority-sensitive resource sharing.

### Counting semaphore (0..N)
`xSemaphoreCreateCounting(uxMaxCount, uxInitialCount)`:
- max: legal highest count / capacity.
- initial: tokens available on creation, must be ≤ max.
- Events: start 0, give per event, take per event. Bounded capacity; excess give can fail. Resource pool: start N, take before acquiring one, give after releasing.
- Sem count alone doesn't identify which specific resource; for resource IDs use queue of handles.

```c
// Counting semaphore pending events ki bounded count hold karega.
static SemaphoreHandle_t events;

void init_events(void) {
    // 10 max count, 0 initial tokens: pehle event ka wait.
    events = xSemaphoreCreateCounting(10, 0);
    // Semaphore creation success check karo.
    configASSERT(events);
}
void worker(void *arg) {
    (void)arg;
    for (;;) {
        // Count zero par worker Blocked; token milte hi successful take.
        if (xSemaphoreTake(events, portMAX_DELAY) == pdTRUE) {
            // Ek token = ek event ka processing.
            handle_one_event();
        }
    }
}
// ISR: BaseType_t hpw = pdFALSE;  // initial wake flag
// xSemaphoreGiveFromISR(events, &hpw);
// portYIELD_FROM_ISR(hpw);  // apne port ka macro verify karo
```

**Comparison:** Single known receiving task + no payload → **task notification counter** often lighter. Many eligible waiters / independent reusable resource count → **counting semaphore**. Data payload chahiye → **queue**.

---

## 6. Mutex, recursive mutex & priority inversion

**Mutex = resource ownership lock**, not event. E.g. same I2C bus / UART / shared structure pe 2 tasks; one task lock, access, **same task unlock**. Mutex supports **priority inheritance** (implementation limitations apply); ISR mein take/give nahi. Binary semaphore lacks ownership and inheritance.

```c
// Same UART resource ko do tasks overlap se bachane ka shared mutex.
static SemaphoreHandle_t uartLock;
void init_uart(void) {
    // Mutex initially available hota, ownership aur inheritance support karta hai.
    uartLock = xSemaphoreCreateMutex();
    // NULL lock use mat karo.
    configASSERT(uartLock);
}
void send_packet(const uint8_t *data, size_t len) {
    // Maximum 200ms equivalent ticks tak lock ka wait kar sakte hain.
    if (xSemaphoreTake(uartLock, pdMS_TO_TICKS(200)) == pdTRUE) {
        // UART transaction lock ke andar complete karo, async DMA alag design mangta hai.
        uart_send(data, len); // Give se pehle UART transaction complete/safe ho
        // Sirf jis task ne take kiya wahi give kare; successful path unlock karo.
        xSemaphoreGive(uartLock);
    } else {
        // Timeout par unlocked UART call nahi karni, failure handle karo.
        record_uart_timeout();
    }
}
```

### Priority inversion with 3 tasks (must explain)
1. Low priority **L** mutex holds; before release, high priority **H** requests → H Blocked.
2. Medium priority **M** (mutex doesn't need) Ready ho kar L ko preempt karta hai; H indirectly waits behind M = **unbounded priority inversion risk**.
3. With inheritance, L temporarily inherits H's higher priority while it holds mutex and H waits; L runs, unlocks, H proceeds. Does not fix deadlocks or long critical sections.
4. Multi-mutex/ESP-IDF specifics mein inheritance details simplified ho sakte hain. Always short lock scope + lock ordering.

### Recursive mutex
`xSemaphoreCreateRecursiveMutex()` / `xSemaphoreTakeRecursive(m,ticks)` / `xSemaphoreGiveRecursive(m)`. Same owning task multiple lock levels le sakta hai; **matching number of gives** required. Normal mutex same task repeat take se deadlock/timeout ho sakta hai. Prefer refactor to single ownership when practical.

**Critical section vs mutex:** Very short shared state update + no blocking operation → critical section/atomic primitive. Bus I/O or operations that can block → mutex (critical section ke andar delay/I2C/network wait mat karo). SMP multi-core protection architecture-specific.

---

## 7. Direct task notifications

**Use case:** ISR ko *exactly one known task* wake karna hai. Task ke TCB mein 32-bit notification value/state; separate semaphore object ki need nahi. **Receiver is task**, unrelated tasks cannot wait on same notification slot. Task notification array enabled ho to multiple slots, but task simultaneously only one slot pe block kar sakta hai.

```c
// ISR ko specific receiver task ka handle chahiye.
static TaskHandle_t handlerTaskHandle;
static void handlerTask(void *unused) {
    (void)unused;
    for (;;) {
        // pdFALSE means count se ek token consume; count 0 par task wait mein Blocked.
        uint32_t count = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        // pdFALSE ek token ghatata hai, purana count return hota hai.
        // Count > 1 hua to agli calls bache hue tokens process karengi.
        // Har loop one event handle karega; pending tokens next iteration mein milenge.
        process_one_irq_event();
    }
}
// Sirf FreeRTOS API call ke liye allowed ISR context mein:
// BaseType_t hpw = pdFALSE;  // wake flag start false
// vTaskNotifyGiveFromISR(handlerTaskHandle, &hpw);  // ISR se signal
// portYIELD_FROM_ISR(hpw);  // ISR exit par zarurat se switch
```

- `xTaskNotifyGive(handle)` increment notification count, from task context.
- `vTaskNotifyGiveFromISR(handle,&higherPriorityWoken)` ISR equivalent.
- `ulTaskNotifyTake(pdTRUE,ticks)` clear full value to 0 on exit = binary/batch-style behavior; **may coalesce events**.
- `ulTaskNotifyTake(pdFALSE,ticks)` decrement by 1 on exit = counting-style behavior.
- `xTaskNotify(handle,value,eAction)` flexible mode: set bits, increment, overwrite, no-overwrite, no-action.
- `xTaskNotifyWait(clearOnEntry,clearOnExit,&outValue,ticks)` wait on pending notification + optional bit clear. **Not equivalent to count-taking**; notification state and value semantics matter.
- `xTaskNotifyFromISR` ISR variant, see action and wake parameters.
- `xTaskNotifyIndexed` etc: specific indexed slot, index within `configTASK_NOTIFICATION_ARRAY_ENTRIES`.

**eNotifyAction quick map:** `eNoAction` = state only; `eSetBits` = OR mask; `eIncrement` = count++; `eSetValueWithOverwrite` = latest 32-bit value; `eSetValueWithoutOverwrite` = can fail if still pending.

**Interview trap:** `ulTaskNotifyTake(pdFALSE)` function returns **pre-decrement** count, not "exactly number processed". One call = one token consumed. Notification value can overflow on extreme counts; no magical unbounded lossless guarantee.

---

## 8. Event groups

**Use case:** App should run only when Wi-Fi connected **AND** sensor calibrated **AND** storage ready; or react when *any* fault flag set. Each condition gets one bit in `EventBits_t`; no payload or arrival count. Bits remain set until cleared; races require protocol.

```c
// WIFI readiness ko bit 0 assign kiya.
#define WIFI_BIT   (1U << 0)
// Sensor readiness ko alag bit 1 assign kiya.
#define SENSOR_BIT (1U << 1)
// Event group multiple status flags ek jagah rakhta hai.
static EventGroupHandle_t readyFlags;

void network_task(void *arg) {
    (void)arg;
    // Successful init ke baad hi ready bit set karna hai.
    wifi_connect();
    // WIFI ready condition signal; same bit baar-baar set karna count nahi badhata.
    xEventGroupSetBits(readyFlags, WIFI_BIT);
    // Current one-shot initialization task khatam, self-delete.
    vTaskDelete(NULL);
}
void app_task(void *arg) {
    (void)arg;
    // App required conditions complete hone tak wait karegi.
    EventBits_t bits = xEventGroupWaitBits(
        // Mask specify karta ki WIFI AUR SENSOR dono chahiye.
        readyFlags, WIFI_BIT | SENSOR_BIT,
        pdFALSE,    // flags ko automatically clear MAT karo
        pdTRUE,     // saari requested conditions ka AND wait
        portMAX_DELAY
    );
    (void)bits;
    // All required bits set hone ke baad run hota hai.
    run_app();
    // Current one-shot initialization task khatam, self-delete.
    vTaskDelete(NULL);
}
```

- `xEventGroupCreate()` returns handle or NULL.
- `xEventGroupSetBits(group,bits)` OR given bits; from ISR different API behavior (**deferred to timer task**); check return.
- `xEventGroupWaitBits(group,mask,clearOnExit,waitForAll,ticks)`: **five params**, AND/OR controlled by `waitForAll`; auto-clear only if appropriate; returns observed bits.
- `xEventGroupClearBits(group,bits)` clears bits.
- `xEventGroupGetBits(group)` snapshot, not atomic wait.
- `xEventGroupSync(group,bitToSet,allBits,ticks)`: barrier for multiple tasks, announce+wait atomic wrt sync semantics; each participant unique bit. Limited usable bits (typically 8 or 24 depending on tick width/config).
- **Alternative:** per-task notifications for only one receiver; event groups for flag combinations and broadcast-style conditions.

---

## 9. Software timers

**Use case:** Button press ke 5 seconds baad idle-off; repeated press par reset. Timer service task callbacks execute karta hai, **callback ke andar blocking/long loops mat karo**.

```c
// Timer handle se 5s countdown restart ya cancel karenge.
static TimerHandle_t idleTimer;
// Callback timer service task mein run hota hai, ISR mein nahi.
static void idle_cb(TimerHandle_t timer) {
    (void)timer;
    // Callback chhota rakho, blocking nahi.
    // Callback short aur nonblocking rakho; heavy I/O worker task ko bhejo.
    turn_off_idle_indicator();
}
void timers_init(void) {
    // One-shot 5s timer create: name, period, reload, ID, callback.
    idleTimer = xTimerCreate("idle", pdMS_TO_TICKS(5000),
                             // pdFALSE = one-shot, auto-reload disabled.
                             pdFALSE, NULL, idle_cb);
    // NULL ka matlab timer allocate nahi ho paya.
    configASSERT(idleTimer != NULL);
}
void on_button_press_task_context(void) {
    // Stopped hai to start; active hai to countdown reset.
    // Button dubara press: countdown reset; 10ms command-queue wait hai, timer period nahi.
    if (xTimerReset(idleTimer, pdMS_TO_TICKS(10)) != pdPASS) {
        // Timer command reject/full ho to error note karo.
        record_timer_command_failure();
    }
}
```

- `xTimerCreate(name,period,autoReload,timerID,callback)`: name debug; period in ticks; `pdFALSE` one-shot / `pdTRUE` periodic; ID user data pointer; callback signature `void cb(TimerHandle_t)`.
- `xTimerStart(timer,ticksToWait)` enqueues start command; command-queue wait parameter **timer delay nahi**, timer period creation pe decided.
- `xTimerReset(timer,ticksToWait)` resets expiry relative to processing/start semantics; debounce-style.
- `xTimerStop(timer,ticksToWait)` stop command; `xTimerChangePeriod(timer,newPeriod,ticksToWait)` changes period.
- `xTimerDelete(timer,ticksToWait)` deletes timer; ensure no unsafe concurrent use.
- FromISR timer control APIs exist; use `...FromISR` with wake flag. Callback itself is **not ISR**, still shouldn't block shared daemon task.
- **Alternative:** dedicated task + `vTaskDelayUntil` for substantial periodic work; HW timer for sub-tick deadlines.

---

## 10. Stream/message buffers, mailbox & queue sets

### Stream buffer = continuous bytes
UART RX stream: bytes from one writer to one reader; no message boundaries. `xStreamBufferCreate(capacity,triggerLevel)`: capacity bytes; trigger level controls how many bytes typically unblock waiting receiver (timeout can return earlier); `xStreamBufferSend(handle,data,len,ticks)`, `xStreamBufferReceive(handle,out,maxLen,ticks)` return **byte count** (not pdPASS). One writer / one reader default assumption; multiple writers/readers require external synchronization. `...FromISR` available.

### Message buffer = variable-length framed packets
`xMessageBufferCreate(capacityBytes)`: variable-length messages with length header overhead. `xMessageBufferSend(handle,msg,len,ticks)` returns bytes written; `xMessageBufferReceive(handle,out,maxLen,ticks)` returns message size, 0 on no receive / insufficient buffer; if output too small, message remains. One writer / one reader default; multiple producers need mutex or prefer queue/pool design. Neither message nor stream buffers act as a universal multi-producer queue by default.

### Mailbox = only latest value
`QueueHandle_t latest = xQueueCreate(1,sizeof(Sample));` then `xQueueOverwrite(latest,&s)`. Receiver `xQueuePeek` for current value without removing. Multi-reader access and *freshness signaling* require design attention: peek alone doesn't block for new value. Length must be one for overwrite.

### Queue sets = one consumer waits on several objects
`xQueueCreateSet(maxMembersPending)`, `xQueueAddToSet(queueOrSemaphore,set)`, `xQueueSelectFromSet(set,ticks)` → returns ready member, then read **that member**; remove via `xQueueRemoveFromSet`. Set capacity must cover sum of event capacities of members. Don't read member directly while in set without following selection rules. Useful but extra complexity; often better architecture is single common queue containing tagged events.

---

## 11. ISR & context switches

### The exact ISR wake-up sequence
1. Task B (high priority) `xSemaphoreTake`/`ulTaskNotifyTake` par **Blocked** hai. Task A lower priority running.
2. Interrupt aaya. ISR `xSemaphoreGiveFromISR` or `vTaskNotifyGiveFromISR` B ko Ready bana sakta hai.
3. `pxHigherPriorityTaskWoken` initially `pdFALSE`. Kernel may set to `pdTRUE` if higher-priority task unblock hua.
4. `portYIELD_FROM_ISR(hpw)` (or port-specific equivalent) ISR exit par reschedule request karta hai → B **next tick ka wait kiye bina** run kar sakta hai.
5. Agar yield request nahi ki, B Ready hone ke bawajood later scheduling point tak wait kar sakta hai. ESP32/other ports and ISR nesting affect exact mechanism.

```c
void gpio_isr_example(void) {
    // ISR wake flag pdFALSE initialize karo.
    BaseType_t hpw = pdFALSE;
    // Task ko ISR-safe notification do; newly Ready high-priority task flag set kar sakti hai.
    vTaskNotifyGiveFromISR(handlerTaskHandle, &hpw);
    // ISR exit par prompt scheduling request; exact signature port-dependent hai.
    portYIELD_FROM_ISR(hpw); // Apne port ka correct macro/signature check karo!
}
```

**Never:** Task-context `xQueueSend` / `xSemaphoreTake` / `vTaskDelay` ISR mein. Also don't use mutex from ISR. ISR FreeRTOS API eligibility is affected by **interrupt priority/level**, especially Cortex-M `configMAX_SYSCALL_INTERRUPT_PRIORITY` and ESP-IDF ISR allocation flags/IRAM requirements.

### Yield API differences
- `taskYIELD()` = task-context voluntary yield; no event wait.
- `portYIELD_FROM_ISR(x)` = common port macro, request switch if `x` true; exact signature varies.
- `portEND_SWITCHING_ISR(x)` = alternative port convention.
- `taskYIELD_FROM_ISR()` = present on some ports; not universally portable. Look at portmacro.h. Don't assume all three interchangeable with same arguments.

### Critical section
`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` (single-core vanilla examples). **ESP-IDF SMP uses portMUX_TYPE spinlock arguments** to its critical-section macros; follow installed IDF headers. Critical section must be short, contain no blocking; interrupts are masked/spinlocking per port. For shared hardware, mutex usually right primitive.

### Scheduler suspension
`vTaskSuspendAll()` / `xTaskResumeAll()` temporarily suspend task scheduling on relevant kernel/port; **does NOT simply disable hardware interrupts**. Not replacement for ISR-safe atomic mutual exclusion, especially SMP. `xTaskGetSchedulerState()` returns `taskSCHEDULER_NOT_STARTED` / `taskSCHEDULER_RUNNING` / `taskSCHEDULER_SUSPENDED` when enabled.

---

## 12. Memory, stack, debugging & configuration

**Two separate things:** Heap allocates queues/TCBs/stacks in dynamic-creation paths; each task's stack stores local variables, return addresses, call frames. Huge local arrays / deep recursion stack overflow kar sakte hain even when heap free.

- `pvPortMalloc(size)`: kernel-managed heap allocate; returns pointer/NULL. `vPortFree(ptr)` releases where heap scheme supports freeing. Heap_1 no free, heap_2/4/5 have different allocation/fragmentation/coalescing; choose port/project scheme.
- `xPortGetFreeHeapSize()`: current free bytes; `xPortGetMinimumEverFreeHeapSize()`: runtime low-water minimum. Enough free bytes != guarantee contiguous block.
- `uxTaskGetStackHighWaterMark(taskHandle)`: minimum free task stack remaining observed so far; **vanilla FreeRTOS units are StackType_t words; ESP-IDF bytes**. `NULL` current task. Very low → enlarge stack/inspect.
- `vApplicationStackOverflowHook(task,name)`: optional callback if overflow checking enabled; hook is diagnostic, not guaranteed to catch every corruption.
- `vApplicationMallocFailedHook()`: optional allocation-failure hook when configured.
- `vApplicationIdleHook()`: idle work, never block. Idle task also involved in some deletion cleanup.
- `vApplicationTickHook()`: tick context, keep very short, ISR constraints.
- `vTaskGetInfo`, `uxTaskGetSystemState`, `vTaskList`, `vTaskGetRunTimeStats`: debugging/statistics depend on config and appropriate counter support; don't call expensive diagnostics in hard real-time path.

### Config flags worth remembering

| Flag | Meaning |
|---|---|
| `configTICK_RATE_HZ` | Number of kernel ticks each second |
| `configMAX_PRIORITIES` | Valid priorities 0 to N-1 |
| `configUSE_PREEMPTION` | Enable preemptive scheduling |
| `configUSE_TIME_SLICING` | Equal-priority time slicing where supported |
| `configUSE_MUTEXES` / `configUSE_RECURSIVE_MUTEXES` | Mutex feature |
| `configUSE_COUNTING_SEMAPHORES` | Counting semaphores |
| `configUSE_TASK_NOTIFICATIONS` | Direct task notifications |
| `configTASK_NOTIFICATION_ARRAY_ENTRIES` | Per-task notification slots |
| `configUSE_TIMERS` | Software timer service |
| `configTIMER_TASK_PRIORITY` | Timer service task priority |
| `configTIMER_QUEUE_LENGTH` | Timer command queue capacity |
| `configCHECK_FOR_STACK_OVERFLOW` | Stack protection checking |
| `configSUPPORT_STATIC_ALLOCATION` | Static create APIs |
| `configSUPPORT_DYNAMIC_ALLOCATION` | Dynamic create APIs |
| `configUSE_TICKLESS_IDLE` | Tickless idle feature (port-dependent) |
| `configMAX_SYSCALL_INTERRUPT_PRIORITY` | Cortex-M interrupt priority boundary for FreeRTOS API calls |

### First five checks when firmware hangs
1. High-priority task infinite busy-loop? Other tasks **starvation**.
2. Waiting on full/empty queue? **Timeout + return code** logged?
3. Mutex lock taken but not released? **Deadlock / priority inversion**.
4. Stack watermark low? **Stack overflow / large local memory**.
5. ISR priority invalid or wrong ISR API? `configASSERT` / crash.

---

## 13. Quick decision matrix

| Situation | Preferred tool | Why not alternative? |
|---|---|---|
| Task A sends struct to Task B | Queue | Semaphore payload store nahi karta |
| ISR wakes a single known task | Direct notification | Less overhead than semaphore |
| Need count of N events | Counting notification or counting semaphore | Binary semaphore coalesces repeated signals |
| Two tasks access same I2C/SPI | Mutex | Binary semaphore lacks owner/inheritance |
| Need short atomic shared update | Atomic/critical section | Long mutex scope avoid, ISR interactions controlled |
| Ready when Wi-Fi & sensor & storage | Event group AND | Counting semaphore identity lose karta |
| Multiple flags OR | Event group OR | Single notification bits alternative if one receiver |
| Continuous UART bytes | Stream buffer | Queue each byte more overhead, no framing |
| Variable length whole messages | Message buffer | Stream buffer preserves no frame boundaries |
| Latest sensor value only | Length-one overwrite queue | Regular queue stores stale backlog |
| 100ms stable periodic loop | DelayUntil | `vTaskDelay` processing time accumulate karta |
| Short delayed callback | Software timer | New task for every small timer wastes RAM |
| Several input queues one waiter | Queue set / tagged common queue | Polling wastes CPU |
| Very short ISR | FromISR notify/queue | No blocking or heavy processing in ISR |
| Multiple identical resource slots | Counting semaphore + actual resource tracker | Mutex only single owner lock |
| Never allocate dynamically at runtime | Static creation | Avoid runtime heap failure/fragmentation |

---

## 14. Rapid-fire interview & counter-questions

1. **Task create successful but task never prints?** Scheduler started? High-priority busy-loop? Task blocked/suspended? Stack? ESP-IDF `app_main` environment different from vanilla `vTaskStartScheduler`.
2. **Does `vTaskDelay(500)` mean 500ms?** No, 500 *ticks*; conversion depends on rate.
3. **Who makes a task Blocked?** Blocking API with resource unavailable + positive wait, or `vTaskDelay`. Queue-full error with wait 0 does NOT.
4. **Queue full and producer highest priority?** With zero-wait busy-spin and lower-priority consumer, **starvation** keeps queue full.
5. **What does `xQueueSend` actually copy?** Exactly `uxItemSize` bytes from pointed object at call; nested pointed-to data not deep-copied.
6. **Why mutex instead of binary semaphore?** Ownership and priority inheritance to protect shared resource.
7. **Can ISR use mutex?** No; ISR sends event to task using FromISR API.
8. **Binary semaphore vs counting?** Binary stores 0/1 token; counting stores bounded event/resource count.
9. **Notification vs queue?** Notifications target one task and limited 32-bit state; queues carry multiple independent payload items.
10. **`pdTRUE` vs `pdFALSE` in `ulTaskNotifyTake`?** Clear all on exit vs decrement by one.
11. **Task B unblocked in ISR, will it wait for tick?** Not if eligible for preemption and ISR issues appropriate yield on exit.
12. **DelayUntil is hard deadline guarantee?** No; scheduled wake and actual CPU execution differ under overruns/preemption.
13. **What's a port?** CPU/architecture + compiler + interrupt/context-switch implementation that adapts kernel to hardware.
14. **Task yield vs block?** Yield remains Ready; block becomes not runnable until condition/time.
15. **ESP-IDF stack 2048 means?** IDF stack bytes, vanilla standard words.
16. **Shared int ++ atomic?** Usually read-modify-write; need atomic/critical section or mutex; multi-core memory ordering matters.
17. **Can timer callback wait on queue indefinitely?** Avoid; timer service shared, stops other callbacks.
18. **`portMAX_DELAY` magic infinite always?** Configuration-dependent, plus task/notification conditions.
19. **Event group used for counting repeated interrupts?** No; repeated set bit collapses; notification counter / semaphore better.
20. **Deadlock fixes?** Consistent lock order, short scope, bounded timeout, avoid nested locks.
21. **Stack vs heap?** Stack per task call frames; heap for dynamic task/queue objects.
22. **Why idle task should run?** Deleted-task cleanup and idle processing; CPU-heavy tasks should block.

**Practice method:** Har API ke liye 5 cheeze bolo: "Why", "parameters", "Blocked kab", "error/timeout", "alternative kab better". Then [scenario workbook](./FREE_RTOS_SCENARIOS.md) solve karo.

---

## 15. Official references
- [FreeRTOS API reference](https://freertos.org/Documentation/02-Kernel/04-API-references/)
- [FreeRTOS task scheduling](https://freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/04-Task-scheduling)
- [ESP-IDF FreeRTOS overview](https://docs.espressif.com/projects/esp-idf/en/v5.3/esp32/api-reference/system/freertos_idf.html)

> Practical note: code snippets API behavior samjhane ke liye hain. Integration mein headers, task stack, GPIO/driver setup, error cleanup, config flags, and port-specific ISR declaration add karna hoga.

---

## Appendix A. Parameter-by-parameter API desk reference

Each entry answers: **har parameter kyun diya?**, **return kya?**, **common mistake kya?** Core working explanations above; yahan interview ke samay quick lookup.

### A1. `xQueueCreate(uxQueueLength, uxItemSize)`

**Parameters:**
1. uxQueueLength: max number of elements stored; full condition isi se aati hai.
2. uxItemSize: bytes copied per item; use sizeof(Type), pointer queue mein sizeof(Type*).

**Return:** QueueHandle_t or NULL. **Practical trap / why:** Burst buffered copies; don't assume external data deep-copied.

### A2. `xQueueSend(xQueue, pvItemToQueue, xTicksToWait)`

**Parameters:**
1. xQueue: valid queue handle.
2. pvItemToQueue: existing source item's address; content copied.
3. xTicksToWait: max ticks to block IF queue full; 0 no wait.

**Return:** pdPASS / errQUEUE_FULL (pdFAIL). **Practical trap / why:** ISR ke liye xQueueSendFromISR.

### A3. `xQueueSendToFront(xQueue, pvItemToQueue, xTicksToWait)`

**Parameters:**
1. xQueue: queue handle.
2. pvItemToQueue: source item address.
3. xTicksToWait: full hone par wait limit.

**Return:** pdPASS / failure. **Practical trap / why:** Urgent message ko front push; repeated use starvation of ordinary queued work.

### A4. `xQueueReceive(xQueue, pvBuffer, xTicksToWait)`

**Parameters:**
1. xQueue: queue handle.
2. pvBuffer: allocated writable output address (at least item size).
3. xTicksToWait: empty queue par max block time.

**Return:** pdPASS / errQUEUE_EMPTY (pdFAIL). **Practical trap / why:** Success par first item removes; producer queue space pa sakta hai.

### A5. `xQueuePeek(xQueue, pvBuffer, xTicksToWait)`

**Parameters:**
1. xQueue: queue handle.
2. pvBuffer: result copy destination.
3. xTicksToWait: empty par wait.

**Return:** pdPASS / failure. **Practical trap / why:** Reads but doesn't remove; count unchanged.

### A6. `xQueueSendFromISR(xQueue, pvItemToQueue, pxHigherPriorityTaskWoken)`

**Parameters:**
1. xQueue: queue handle.
2. pvItemToQueue: source item's address.
3. pxHigherPriorityTaskWoken: pointer to pdFALSE-initialized BaseType_t; API sets when needed.

**Return:** pdPASS / errQUEUE_FULL. **Practical trap / why:** No wait; after ISR API use port-specific yield if flag true.

### A7. `xQueueReceiveFromISR(xQueue, pvBuffer, pxHigherPriorityTaskWoken)`

**Parameters:**
1. xQueue: queue handle.
2. pvBuffer: writable output.
3. pxHigherPriorityTaskWoken: flag for newly unblocked higher priority task.

**Return:** pdPASS / pdFAIL. **Practical trap / why:** ISR must be short; only if port/API allowed.

### A8. `xQueueOverwrite(xQueue, pvItemToQueue)`

**Parameters:**
1. xQueue: queue length MUST be 1.
2. pvItemToQueue: latest value's address.

**Return:** pdPASS. **Practical trap / why:** Mailbox; old value intentionally overwritten.

### A9. `xSemaphoreTake(xSemaphore, xTicksToWait)`

**Parameters:**
1. xSemaphore: binary/counting/mutex handle.
2. xTicksToWait: unavailable token/lock par max ticks.

**Return:** pdTRUE on take / pdFALSE on timeout. **Practical trap / why:** Mutex take has owner; semaphore take has token semantics.

### A10. `xSemaphoreGive(xSemaphore)`

**Parameters:**
1. xSemaphore: valid semaphore/mutex handle; for mutex give by owner.

**Return:** pdTRUE / pdFALSE. **Practical trap / why:** Binary/counting full hone par failure possible; never give mutex from ISR.

### A11. `xSemaphoreGiveFromISR(xSemaphore, pxHigherPriorityTaskWoken)`

**Parameters:**
1. xSemaphore: binary/counting handle only.
2. pxHigherPriorityTaskWoken: pdFALSE-initialized wake flag pointer.

**Return:** pdTRUE / pdFALSE. **Practical trap / why:** ISR context, no mutex; portYIELD_FROM_ISR as needed.

### A12. `xSemaphoreCreateCounting(uxMaxCount, uxInitialCount)`

**Parameters:**
1. uxMaxCount: absolute bound.
2. uxInitialCount: initial tokens, not more than maximum.

**Return:** SemaphoreHandle_t or NULL. **Practical trap / why:** Events start at 0; N free resources start at N.

### A13. `xSemaphoreTakeRecursive(xMutex, xTicksToWait)`

**Parameters:**
1. xMutex: recursive mutex created by matching API.
2. xTicksToWait: wait if another task owns it.

**Return:** pdTRUE / pdFALSE. **Practical trap / why:** Same task nested takes allowed, matching gives required.

### A14. `xSemaphoreGiveRecursive(xMutex)`

**Parameters:**
1. xMutex: recursive mutex currently owned by calling task.

**Return:** pdTRUE / pdFALSE. **Practical trap / why:** Use only with recursive mutex; no ISR.

### A15. `xTaskNotify(xTaskToNotify, ulValue, eAction)`

**Parameters:**
1. xTaskToNotify: target task handle.
2. ulValue: 32-bit payload/mask (ignored for eIncrement/eNoAction).
3. eAction: overwrite, OR bits, increment, etc..

**Return:** pdPASS / pdFAIL for no-overwrite pending. **Practical trap / why:** No queue of structs; one target notification state/value.

### A16. `xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit, pulNotificationValue, xTicksToWait)`

**Parameters:**
1. ulBitsToClearOnEntry: mask cleared when entering wait if no pending notification.
2. ulBitsToClearOnExit: mask cleared before return after value copied.
3. pulNotificationValue: optional output pointer for observed 32-bit value.
4. xTicksToWait: max wait for pending notification.

**Return:** pdTRUE notified / pdFALSE timeout. **Practical trap / why:** For count-style notify prefer ulTaskNotifyTake.

### A17. `ulTaskNotifyTake(xClearCountOnExit, xTicksToWait)`

**Parameters:**
1. xClearCountOnExit: pdTRUE clears all count; pdFALSE decrements one.
2. xTicksToWait: max block if count zero.

**Return:** uint32_t pre-clear/pre-decrement value (0 on timeout). **Practical trap / why:** Not a generic message queue; no payload history.

### A18. `xEventGroupSetBits(xEventGroup, uxBitsToSet)`

**Parameters:**
1. xEventGroup: event handle.
2. uxBitsToSet: OR bit mask.

**Return:** EventBits_t resulting bit state. **Practical trap / why:** Repeated setting same bit doesn't count multiple events.

### A19. `xEventGroupWaitBits(xEventGroup, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait)`

**Parameters:**
1. xEventGroup: handle.
2. uxBitsToWaitFor: mask of relevant bits.
3. xClearOnExit: pdTRUE auto-clear matched flags on satisfaction.
4. xWaitForAllBits: pdTRUE AND, pdFALSE OR.
5. xTicksToWait: timeout ticks.

**Return:** EventBits_t snapshot, inspect required condition/timeout. **Practical trap / why:** For level readiness often xClearOnExit=pdFALSE.

### A20. `xEventGroupSync(xEventGroup, uxBitsToSet, uxBitsToWaitFor, xTicksToWait)`

**Parameters:**
1. xEventGroup: shared group.
2. uxBitsToSet: unique arrival bit for calling task.
3. uxBitsToWaitFor: collective ALL-mask.
4. xTicksToWait: barrier timeout.

**Return:** EventBits_t bits seen. **Practical trap / why:** Barrier implementation; don't confuse with one-time init bits.

### A21. `xTimerCreate(pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallbackFunction)`

**Parameters:**
1. pcTimerName: debug name.
2. xTimerPeriodInTicks: nonzero period ticks.
3. uxAutoReload: pdTRUE periodic, pdFALSE one-shot.
4. pvTimerID: per-timer application pointer.
5. pxCallbackFunction: void callback(TimerHandle_t).

**Return:** TimerHandle_t or NULL. **Practical trap / why:** Timer callback runs in timer daemon task.

### A22. `xTimerChangePeriod(xTimer, xNewPeriod, xTicksToWait)`

**Parameters:**
1. xTimer: existing timer handle.
2. xNewPeriod: new interval in ticks (not milliseconds).
3. xTicksToWait: how long caller can wait to post command.

**Return:** pdPASS / pdFAIL. **Practical trap / why:** Callback must be short; command can fail if timer command queue full.

### A23. `xTimerStart(xTimer, xTicksToWait)`

**Parameters:**
1. xTimer: timer handle.
2. xTicksToWait: timer command queue send timeout (NOT timer period).

**Return:** pdPASS / pdFAIL. **Practical trap / why:** Command acceptance and callback execution are different stages.

### A24. `xTimerReset(xTimer, xTicksToWait)`

**Parameters:**
1. xTimer: timer handle.
2. xTicksToWait: command queue wait duration.

**Return:** pdPASS / pdFAIL. **Practical trap / why:** Restarts countdown for debounce/inactivity.

### A25. `xStreamBufferCreate(xBufferSizeBytes, xTriggerLevelBytes)`

**Parameters:**
1. xBufferSizeBytes: byte storage capacity.
2. xTriggerLevelBytes: number of bytes desired to unblock receiver, where possible.

**Return:** StreamBufferHandle_t or NULL. **Practical trap / why:** Framing not preserved.

### A26. `xStreamBufferSend(xStreamBuffer, pvTxData, xDataLengthBytes, xTicksToWait)`

**Parameters:**
1. xStreamBuffer: buffer handle.
2. pvTxData: bytes input.
3. xDataLengthBytes: requested byte count.
4. xTicksToWait: maximum wait for buffer space.

**Return:** bytes actually sent. **Practical trap / why:** Partial writes possible; must handle return.

### A27. `xStreamBufferReceive(xStreamBuffer, pvRxData, xBufferLengthBytes, xTicksToWait)`

**Parameters:**
1. xStreamBuffer: buffer handle.
2. pvRxData: writable byte array.
3. xBufferLengthBytes: output capacity.
4. xTicksToWait: maximum wait.

**Return:** bytes actually received. **Practical trap / why:** Treat as stream, not packet.

### A28. `xMessageBufferCreate(xBufferSizeBytes)`

**Parameters:**
1. xBufferSizeBytes: total capacity including framing overhead.

**Return:** MessageBufferHandle_t or NULL. **Practical trap / why:** Message buffers preserve per-message boundaries.

### A29. `xMessageBufferSend(xMessageBuffer, pvTxData, xDataLengthBytes, xTicksToWait)`

**Parameters:**
1. xMessageBuffer: handle.
2. pvTxData: complete message data.
3. xDataLengthBytes: message length.
4. xTicksToWait: room wait limit.

**Return:** bytes sent or 0. **Practical trap / why:** Message needs full space including header.

### A30. `xMessageBufferReceive(xMessageBuffer, pvRxData, xBufferLengthBytes, xTicksToWait)`

**Parameters:**
1. xMessageBuffer: handle.
2. pvRxData: output writable buffer.
3. xBufferLengthBytes: max output bytes.
4. xTicksToWait: wait for message.

**Return:** message length or 0. **Practical trap / why:** Too small receive buffer cannot accept pending message.

### A31. `xQueueCreateSet(uxEventQueueLength)`

**Parameters:**
1. uxEventQueueLength: sum capacities / max pending events across members.

**Return:** QueueSetHandle_t or NULL. **Practical trap / why:** Not a substitute for a shared tagged queue when simpler.

### A32. `xQueueAddToSet(xQueueOrSemaphore, xQueueSet)`

**Parameters:**
1. xQueueOrSemaphore: member queue/semaphore not simultaneously used outside set rules.
2. xQueueSet: created set handle.

**Return:** pdPASS / pdFAIL. **Practical trap / why:** Subscribe member once, then select before receive.

### A33. `xQueueSelectFromSet(xQueueSet, xTicksToWait)`

**Parameters:**
1. xQueueSet: set of members.
2. xTicksToWait: wait for any member readiness.

**Return:** ready member handle or NULL. **Practical trap / why:** Then read that selected object.

### A34. `pvPortMalloc(xWantedSize)`

**Parameters:**
1. xWantedSize: heap bytes requested.

**Return:** void* or NULL. **Practical trap / why:** Heap scheme/config determines free/coalescing behavior.

### A35. `vPortFree(pv)`

**Parameters:**
1. pv: pointer allocated by matching FreeRTOS heap API.

**Return:** void. **Practical trap / why:** Not supported for freeing in heap_1.

### A36. `uxTaskGetStackHighWaterMark(xTask)`

**Parameters:**
1. xTask: task handle, NULL means self.

**Return:** minimum ever unused stack; units port-dependent. **Practical trap / why:** ESP-IDF returns bytes; vanilla words.

### A37. `vTaskPrioritySet(xTask, uxNewPriority)`

**Parameters:**
1. xTask: handle, NULL means self.
2. uxNewPriority: 0..configMAX_PRIORITIES-1.

**Return:** void. **Practical trap / why:** Scheduler may immediately pick a different task.

### A38. `xTaskCreatePinnedToCore(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID)`

**Parameters:**
1. pvTaskCode: task entry.
2. pcName: debug task name.
3. usStackDepth: ESP-IDF bytes.
4. pvParameters: stable task argument pointer.
5. uxPriority: relative priority.
6. pxCreatedTask: optional out handle.
7. xCoreID: CPU index or tskNO_AFFINITY.

**Return:** pdPASS / failure. **Practical trap / why:** ESP-IDF only; avoid unnecessary affinity.

> Reminder: exact declaration/version can vary by kernel and ESP-IDF port. Check installed headers before copying a signature verbatim.
