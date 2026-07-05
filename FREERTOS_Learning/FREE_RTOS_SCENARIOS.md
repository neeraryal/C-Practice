# FreeRTOS Scenario-Based Interview Questions

> 40 real-world design scenarios. Each has an **Optimal Approach** (what a strong candidate
> answers) and an **Alternative Solution** (a valid trade-off or fallback), plus the key
> reasoning interviewers are listening for.
>
> Companion to [`FREE_RTOS_API.md`](./FREE_RTOS_API.md) — API details are cross-referenced there.

---

## Table of Contents

- [A. Signaling & Synchronization](#a-signaling--synchronization)
- [B. Data Passing](#b-data-passing)
- [C. Shared Resources & Mutual Exclusion](#c-shared-resources--mutual-exclusion)
- [D. Timing & Periodic Work](#d-timing--periodic-work)
- [E. Interrupts & Deferred Processing](#e-interrupts--deferred-processing)
- [F. Power, Memory & Reliability](#f-power-memory--reliability)
- [G. Architecture & Debugging](#g-architecture--debugging)
- [Quick Decision Cheat Sheet](#quick-decision-cheat-sheet)

---

## A. Signaling & Synchronization

### Q1. A GPIO interrupt must wake exactly one known task as fast as possible. How?

**Optimal Approach:** Use a **direct-to-task notification**. In the ISR call
`vTaskNotifyGiveFromISR(handlerTask, &woken)` then `portYIELD_FROM_ISR(woken)`; the task blocks
on `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)`. Zero extra RAM (it lives in the TCB), ~45% faster
than a binary semaphore, and it targets one specific task.

**Alternative:** A **binary semaphore** (`xSemaphoreGiveFromISR` / `xSemaphoreTake`). Slightly
slower and needs a semaphore object, but is the right call if **multiple** tasks might need to
wait on the event, or if the signaling source isn't always the same task.

**Why:** Interviewers want you to reach for notifications when it's 1-sender→1-known-receiver.

---

### Q2. An ISR can fire several times before its handler task gets to run. You must not lose any events. What primitive?

**Optimal Approach:** A **counting semaphore** (`xSemaphoreCreateCounting(max, 0)`). Each
`xSemaphoreGiveFromISR` increments the count; the task loops `xSemaphoreTake` once per event, so
every burst is processed. Equivalent: a task notification used as a counter
(`ulTaskNotifyTake(pdFALSE, ...)` — `pdFALSE` = don't clear, decrement one at a time).

**Alternative:** A **queue** carrying an event descriptor — same "no loss" guarantee *and* you
get the event's data, at higher RAM/time cost. Use this if each event carries a payload.

**Trap to avoid:** A **binary** semaphore latches at 1 and silently drops the extra events.

---

### Q3. A task must wait until three independent subsystems (sensor, GPS, network) have all initialized. How?

**Optimal Approach:** An **event group**. Each subsystem calls `xEventGroupSetBits` with its own
bit; the waiter calls `xEventGroupWaitBits(group, SENSOR|GPS|NET, pdTRUE, pdTRUE, portMAX_DELAY)`
— `xWaitForAllBits = pdTRUE` blocks until **all** are set (AND), then clears them.

**Alternative:** A **counting semaphore** initialized to 0 where each subsystem gives once and
the waiter takes three times. Works, but loses the identity of *which* subsystem is late — worse
for diagnostics.

---

### Q4. Multiple worker tasks must all reach a common point before any continues (a barrier). How?

**Optimal Approach:** `xEventGroupSync()`. Each worker atomically sets its own arrival bit and
waits for the full set — no race between "announce" and "wait." All are released together.

**Alternative:** A counting semaphore plus a "release" semaphore/notification managed by a
coordinator task. More moving parts and easy to get wrong; only worth it if you also need to run
coordinator logic at the barrier.

---

### Q5. One event must wake several waiting tasks simultaneously (broadcast). How?

**Optimal Approach:** An **event group** — set a bit, and *every* task blocked on that bit
unblocks. This is the natural one-to-many broadcast in FreeRTOS.

**Alternative:** Give a **counting semaphore** N times (once per waiter), or notify each task by
handle in a loop. Both require knowing the waiter count; the event group doesn't.

**Trap:** A binary semaphore only releases **one** waiter — not a broadcast.

---

### Q6. Two tasks each need to know when the other has produced a result, ping-pong style. How?

**Optimal Approach:** Two **binary semaphores** (or two task notifications, one per direction).
Task A does work, gives semB, waits on semA; Task B waits on semB, works, gives semA. Clean
hand-off, no shared flag polling.

**Alternative:** A single **queue** each way, if the "result" is actual data rather than a bare
signal — combines the sync with the data transfer.

---

## B. Data Passing

### Q7. Producer task generates sensor structs; consumer task processes them, occasionally slower than they arrive. Design the link.

**Optimal Approach:** A **queue** sized to absorb bursts: `xQueueCreate(depth, sizeof(Reading_t))`.
Producer `xQueueSend` with a short timeout; consumer `xQueueReceive(portMAX_DELAY)`. Decouples
the two rates and preserves order. Choose `depth` from worst-case burst × processing latency.

**Alternative:** A **stream/message buffer** if the payload is raw bytes / variable length. If
only the *latest* reading matters, switch to a mailbox (see Q9).

**Follow-up they'll ask:** What if the queue fills? Decide policy: block (back-pressure), drop
newest, or overwrite oldest — and justify it.

---

### Q8. The struct being passed is large (e.g. 512 bytes). Queue copies are expensive. What do you do?

**Optimal Approach:** Queue a **pointer** to the buffer (`xQueueCreate(depth, sizeof(void*))`),
not the buffer itself. Allocate from a **pre-allocated pool / memory block**, hand ownership to
the consumer, which returns the block to the pool when done. Copy cost drops to 4 bytes.

**Alternative:** A **message buffer** for variable-length blobs, or double-buffering with a
mailbox if only the newest matters. Pointer-passing needs a clear **ownership contract** to
avoid use-after-free — state that explicitly.

---

### Q9. Many display/control tasks all need the *latest* temperature; older readings are useless. Design it.

**Optimal Approach:** A **mailbox** = queue of length 1 written with `xQueueOverwrite` (never
blocks, always holds the newest) and read with `xQueuePeek` (non-destructive, so every reader
sees the same latest value). Writer overwrites at its own cadence; readers sample independently.

**Alternative:** A shared global protected by a mutex (or, for a single word, a plain atomic
read). Simpler but you must manage locking; the mailbox gives lock-free multi-reader semantics.

---

### Q10. A UART RX ISR receives a continuous byte stream of unknown length; a task parses it. Design.

**Optimal Approach:** A **stream buffer**. ISR calls `xStreamBufferSendFromISR` per byte/chunk;
the parser task calls `xStreamBufferReceive` with a trigger level and timeout. Purpose-built for
single-producer→single-consumer byte streams, lower overhead than a queue of bytes.

**Alternative:** A DMA ring buffer filled by hardware with the task notified on half/full/idle
interrupts — highest throughput, but more platform-specific setup. A byte queue also works but
wastes RAM/time (per-item overhead on single bytes).

---

### Q11. You must pass discrete, variable-length protocol frames (e.g. JSON packets) between two tasks. Which primitive?

**Optimal Approach:** A **message buffer** — it preserves message boundaries (each receive
returns exactly one frame via an internal length header). Single writer, single reader.

**Alternative:** A **queue of {length, pointer}** descriptors pointing at pooled buffers — needed
if you require **multiple** senders (message buffers are strictly one-to-one) or random access.

---

### Q12. Two producers must feed one consumer. A message buffer is one-to-one. How do you handle it?

**Optimal Approach:** Use a **queue** (queues support multiple senders natively). Each producer
`xQueueSend`s its item; the consumer drains in FIFO order.

**Alternative:** Give each producer its **own** message/stream buffer and have the consumer wait
on a **queue set** covering both. Keeps per-stream framing while still serving one consumer.

---

## C. Shared Resources & Mutual Exclusion

### Q13. Two tasks both transmit on the same UART/SPI peripheral. Prevent interleaved/garbled output.

**Optimal Approach:** A **mutex** (`xSemaphoreCreateMutex`). Take before the transaction, give
after. Ownership means only the locking task can release it, and **priority inheritance**
prevents a low-priority holder from starving a high-priority waiter.

**Alternative:** A dedicated **"server" task** that owns the peripheral; other tasks send
requests via a **queue**. This serializes access with zero locking in the callers and centralizes
error handling — preferred in larger systems.

**Trap:** Don't use a binary semaphore here — it lacks ownership and priority inheritance.

---

### Q14. A recursive function needs to lock the same mutex it already holds and would deadlock itself. Fix?

**Optimal Approach:** A **recursive mutex** (`xSemaphoreCreateRecursiveMutex` +
`xSemaphoreTakeRecursive`/`GiveRecursive`). The owning task can re-take it; each take must be
matched by a give, and it's released only when the count hits zero.

**Alternative:** **Refactor** so the lock is acquired once at the top-level entry and the inner
functions assume it's held (lock-free internals). Cleaner and cheaper — recursion + shared locks
is often a design smell.

---

### Q15. A low-priority task holds a mutex a high-priority task needs, and a medium task keeps preempting the low one. The high task starves. Name and fix it.

**Optimal Approach:** This is **priority inversion**. Fix: use a FreeRTOS **mutex**, which applies
**priority inheritance** automatically — the low-priority holder is temporarily boosted to the
waiter's priority so it finishes and releases quickly, then drops back.

**Alternative:** A **priority ceiling** protocol (manual in FreeRTOS): raise the task to the
resource's ceiling priority *before* taking the lock and restore after. Bounds inversion to one
critical section and prevents deadlock among tasks sharing that ceiling.

---

### Q16. You only need to protect a two-line update to a shared 32-bit variable. Mutex feels heavy. What's better?

**Optimal Approach:** A **critical section** (`taskENTER_CRITICAL` / `taskEXIT_CRITICAL`) around
the few instructions. It's microseconds long, no context switch, no separate object. Ideal for
touching a flag or hardware register.

**Alternative:** `vTaskSuspendAll` / `xTaskResumeAll` if you must block **task** switches but keep
**interrupts** running (e.g. the update is a bit longer but doesn't touch ISR-shared data). For a
single aligned word on a 32-bit MCU, a plain read/write may already be atomic — but say so only
with the ISR-safety caveat.

**Trap:** Never call a blocking API inside a critical section or while the scheduler is suspended.

---

### Q17. Two tasks acquire mutex A and mutex B in opposite orders and occasionally hang. Diagnose and prevent.

**Optimal Approach:** Classic **deadlock** (circular wait). Prevent with **global lock ordering**:
every task acquires A **before** B, always. Eliminates the circular-wait condition entirely.

**Alternative:** **Timeout + back-off** — take the second mutex with a finite `xTicksToWait`; on
failure, release the first and retry after a short delay. Guarantees progress but wastes cycles
and can livelock under heavy contention, so ordering is preferred.

---

### Q18. A shared resource has N identical instances (e.g. 3 DMA channels). Tasks should grab any free one and block when all are busy. How?

**Optimal Approach:** A **counting semaphore** initialized to N. Take before using a channel
(count→count-1), give when done. When count hits 0, further takers block until one is released.
Pair it with a small mutex-protected free-list to pick *which* channel.

**Alternative:** A **queue pre-loaded with N channel handles**: receive a handle to acquire, send
it back to release. The queue *is* the pool and hands you the specific instance in one step — often
cleaner than semaphore + free-list.

---

## D. Timing & Periodic Work

### Q19. A control loop must run at exactly 1 kHz regardless of how long its body takes. How?

**Optimal Approach:** `vTaskDelayUntil()` (a.k.a. `xTaskDelayUntil`) with a
`xTaskGetTickCount()` seed and a fixed 1 ms increment. It schedules against an **absolute** wake
time, so execution time doesn't accumulate into the period — no drift.

**Alternative:** A **hardware timer interrupt** at 1 kHz that gives a semaphore / notifies the
loop task — best when you need tighter jitter than the tick allows, or the tick rate is lower
than 1 kHz. `vTaskDelay` is **wrong** here (period = delay + run time → drift).

---

### Q20. You need a callback 5 seconds after a button press, cancelable if pressed again. How?

**Optimal Approach:** A **one-shot software timer** (`xTimerCreate(..., pdFALSE, ...)`). On press,
`xTimerReset()` (starts, or restarts the 5 s if already running); the callback fires only if no
new press arrives. No dedicated task, no stack cost.

**Alternative:** A dedicated task that blocks on `ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000))`;
a fresh press notifies it (restarting the wait), timeout runs the action. Use this if the action
must **block** — timer callbacks must not.

---

### Q21. You have 15 periodic housekeeping jobs at various intervals. Spawning 15 tasks wastes RAM. Better design?

**Optimal Approach:** **Software timers**. All 15 auto-reload timers share one **timer daemon
task** (one stack), each with its own period and callback. Massive RAM saving vs 15 task stacks.

**Alternative:** One "scheduler" task with a **sorted timer wheel / delta list** it services in a
loop with `vTaskDelayUntil`. More code, but gives full control and lets callbacks block (they run
in a normal task, not the daemon).

**Constraint to state:** Timer callbacks run on the shared daemon — they must be short and
non-blocking.

---

### Q22. A timer callback needs to do real work that occasionally blocks (I2C read). But callbacks must not block. Resolve.

**Optimal Approach:** Keep the callback trivial — have it **signal a worker task** (notification /
semaphore) that performs the blocking I2C read in normal task context. The timer just paces; the
task does the work.

**Alternative:** Replace the timer with a **dedicated periodic task** using `vTaskDelayUntil`,
which *can* block directly. Fewer moving parts if this is the only such job.

---

## E. Interrupts & Deferred Processing

### Q23. An ISR needs to do lengthy processing (parse a packet, call FreeRTOS APIs). You can't do that in the ISR. Pattern?

**Optimal Approach:** **Deferred interrupt processing.** Keep the ISR minimal — capture what's
needed and **signal a handler task** (task notification or binary semaphore), which does the heavy
work in task context where all APIs are legal. Minimizes interrupt latency for the rest of the
system.

**Alternative:** `xTimerPendFunctionCallFromISR()` — queue a function pointer + args to the timer
daemon, which runs your handler in task context. No dedicated task or semaphore needed; great for
one-off deferred work.

---

### Q24. Why must you call `xQueueSendFromISR` instead of `xQueueSend` inside an ISR? What actually breaks otherwise?

**Optimal Approach:** Non-ISR APIs may **block** and manipulate the ready/blocked lists assuming
task context. From an ISR there is no task to block, and re-entering the scheduler's lists from
interrupt context corrupts kernel state → asserts or HardFault. `FromISR` variants **never block**
and use interrupt-safe critical sections. Always follow with
`portYIELD_FROM_ISR(pxHigherPriorityTaskWoken)` so a woken higher-priority task runs immediately
on ISR exit.

**Alternative / nuance:** Only ISRs at or below `configMAX_SYSCALL_INTERRUPT_PRIORITY` may call
*any* FreeRTOS API. Higher-priority ISRs must call **none** — they run with zero kernel latency
and communicate via plain memory + a lower-priority ISR or flag.

---

### Q25. After an ISR gives a semaphore, the high-priority task that wants it doesn't run until the next tick. Why, and how do you fix it?

**Optimal Approach:** You forgot `portYIELD_FROM_ISR(woken)`. The `FromISR` call sets
`pxHigherPriorityTaskWoken = pdTRUE` when it unblocks a higher-priority task, but the actual
context switch only happens if you request it at ISR exit. Add the yield and the switch is
immediate.

**Alternative:** If sub-tick latency isn't required, letting the scheduler switch at the next tick
is acceptable — but for responsive designs, always yield.

---

### Q26. A sensor raises interrupts faster than your task can process, and you must not miss counts, but each interrupt has no payload. Cheapest correct mechanism?

**Optimal Approach:** A **task notification used as a counter**:
`vTaskNotifyGiveFromISR` in the ISR (increments the notification value), and
`ulTaskNotifyTake(pdFALSE, ...)` in the task (decrements one per call). Zero extra objects,
fastest path, no lost counts.

**Alternative:** A **counting semaphore** — same guarantee, works when **multiple** tasks may
consume the events (notifications are single-target).

---

## F. Power, Memory & Reliability

### Q27. A coin-cell IoT device wakes every 1 ms for the tick and drains the battery. How do you cut idle power?

**Optimal Approach:** Enable **tickless idle** (`configUSE_TICKLESS_IDLE`). When all tasks will be
blocked for many ticks, the kernel stops the periodic tick, deep-sleeps the core for that span,
and corrects the tick count on wake. The MCU sleeps for long stretches instead of waking 1000×/s.

**Alternative:** An **idle hook** that issues `__WFI()` to sleep until the next interrupt — simpler
but still wakes every tick. Combine both: tickless for long sleeps, `WFI` for short ones. On ESP32
this maps to **light sleep** (RAM retained).

---

### Q28. Your firmware must be certifiable / must never fragment the heap or fail an allocation at runtime. How do you allocate objects?

**Optimal Approach:** **Static allocation** everywhere — `xTaskCreateStatic`, `xQueueCreateStatic`,
`xSemaphoreCreateBinaryStatic`, etc. You provide the buffers; there's **no heap**, so no
fragmentation and no runtime allocation failure. Provide `vApplicationGetIdleTaskMemory` /
`GetTimerTaskMemory`.

**Alternative:** Dynamic allocation but **only at startup** with `heap_1` (allocate-only, no free)
— deterministic and simple, but you can never delete objects. `heap_4` (coalescing first-fit) is
the general-purpose choice when you must free/reallocate.

---

### Q29. A task intermittently corrupts memory and crashes after minutes of running. Prime suspect and how to confirm?

**Optimal Approach:** **Stack overflow.** Confirm with `configCHECK_FOR_STACK_OVERFLOW = 2` (paint
`0xA5` pattern check — catches transient overflows Method 1 misses) and a
`vApplicationStackOverflowHook`. Also read `uxTaskGetStackHighWaterMark` for each task and grow
the stack of any near zero.

**Alternative:** If the MCU has an **MPU**, put a guard region below each stack to fault on
overflow immediately (deterministic, catches the exact offending access). Then right-size stacks
from the water-mark data.

---

### Q30. Tasks you delete with `vTaskDelete` seem to leak RAM — free heap keeps dropping. Why?

**Optimal Approach:** A deleted task's **TCB and stack are freed by the idle task**, not
immediately. If a higher-priority task never blocks, the **idle task never runs**, so cleanup
never happens. Fix: ensure every task yields/blocks (`vTaskDelay`, queue waits) so idle gets CPU.

**Alternative:** Use **static** tasks (nothing to free) or have tasks **suspend** instead of
delete when you'll reuse them — no cleanup dependency on idle at all.

---

### Q31. `pvPortMalloc` starts returning NULL after hours of uptime, though total allocations seem fine. Cause and prevention?

**Optimal Approach:** **Heap fragmentation** from repeated malloc/free of varying sizes at
runtime. Prevent by allocating all objects **at startup** and never in hot loops; use `heap_4`
(coalesces adjacent free blocks) or fixed-size **memory pools**. Instrument with
`xPortGetMinimumEverFreeHeapSize()` to size the heap and detect leaks.

**Alternative:** Switch to **static allocation** entirely, or `heap_5` if you need to span
multiple RAM banks. Add `vApplicationMallocFailedHook` to catch and log any failure instead of
silently continuing.

---

### Q32. You must guarantee a watchdog is fed only if all critical tasks are alive and healthy. Design.

**Optimal Approach:** A **supervisor task** (or timer) that requires each critical task to
periodically set its bit in an **event group** (or increment a per-task heartbeat counter). The
supervisor only kicks the hardware watchdog when **all** expected bits are set within the window;
otherwise it lets the watchdog reset the system.

**Alternative:** Each task refreshes a timestamp in a shared, mutex-protected struct; the
supervisor checks all timestamps are recent. Equivalent guarantee, slightly more code than the
event-group AND-wait.

---

## G. Architecture & Debugging

### Q33. One task must service several sources — a data queue, a command queue, and a "shutdown" semaphore — blocking efficiently on all. How?

**Optimal Approach:** A **queue set** (`xQueueCreateSet` + `xQueueAddToSet`). The task blocks on
`xQueueSelectFromSet`, which returns whichever member is ready; the task then receives/takes from
that specific member. One blocking point, no polling.

**Alternative:** If the sources are just flags/events (no payload), an **event group** is lighter.
If everything can be reduced to messages, funnel all sources into **one queue** carrying a tagged
union `{type, data}` — often the simplest robust design.

---

### Q34. How do you decide task priorities so the system meets deadlines?

**Optimal Approach:** **Rate-monotonic** reasoning — shorter period / tighter deadline ⇒ higher
priority. Keep the number of priority levels small, ensure every task has a blocking point so
lower-priority tasks get CPU, and reserve the highest levels for latency-critical ISRs' handler
tasks. Validate with run-time stats (`vTaskGetRunTimeStats`).

**Alternative:** **Deadline-driven** grouping when periods don't map cleanly — assign by criticality
tiers and verify empirically with a trace tool (Tracealyzer/SystemView). State the assumption that
FreeRTOS is fixed-priority preemptive, not EDF.

---

### Q35. Two equal-priority tasks are both CPU-bound; one appears to starve the other. What's happening and the options?

**Optimal Approach:** With `configUSE_TIME_SLICING = 1`, equal-priority Ready tasks **round-robin**
each tick — but a task that never blocks and one that occasionally blocks won't share evenly, and
if slicing is off, the first one runs until it yields. Give each a blocking/yield point, or split
their priorities so the more urgent one preempts.

**Alternative:** Insert an explicit `taskYIELD()` in the long-running loop, or redesign the
CPU-bound work into chunks that block on a queue/notification — cooperative fairness instead of
relying on slicing.

---

### Q36. You suspect a task overflows its stack but don't know which. How do you find it without a debugger attached?

**Optimal Approach:** Enable `configCHECK_FOR_STACK_OVERFLOW = 2` and implement
`vApplicationStackOverflowHook(xTask, pcTaskName)` to log the offending task's **name** (over UART
/ to flash) before halting. Deterministic identification.

**Alternative:** Periodically walk all tasks with `uxTaskGetSystemState` / `vTaskList` and print
each **high-water mark**; the task trending toward 0 is your culprit. Good for proactive
monitoring, not just post-mortem.

---

### Q37. The system runs fine in debug but HardFaults in release the moment an interrupt fires. Most likely cause?

**Optimal Approach:** An **interrupt-priority misconfiguration** — an ISR at a priority **above**
`configMAX_SYSCALL_INTERRUPT_PRIORITY` is calling a `...FromISR` API, or the priority-bit shift is
wrong for the MCU's `configPRIO_BITS`. In debug, `configASSERT` in `port.c` would have caught it;
in release it's compiled out, so the corruption surfaces as a fault. Fix the priorities and keep
`configASSERT` enabled during development.

**Alternative:** The fault is an uninitialized/optimized-away variable exposed by release
optimization — but with the "on interrupt" timing, priority misconfig is the textbook answer.

---

### Q38. On a dual-core ESP32, a task doing Wi-Fi work and your real-time control task interfere. How do you isolate them?

**Optimal Approach:** **Pin tasks to cores** with `xTaskCreatePinnedToCore`. Espressif pins the
Wi-Fi/BT stack to **Core 0**; pin your deterministic control task to **Core 1** to avoid
contention and cache/scheduler interference.

**Alternative:** Leave tasks unpinned (`tskNO_AFFINITY`) and rely on the SMP scheduler, but raise
the control task's priority and minimize shared locks. Pinning is the stronger guarantee for
real-time isolation.

**SMP nuance:** Cross-core shared data still needs proper locking — pinning reduces contention, it
doesn't remove the need for mutexes/critical sections.

---

### Q39. A "fast" ISR (e.g. motor commutation) must have the lowest possible latency and must never be delayed by the kernel. How do you configure it?

**Optimal Approach:** Give that ISR a hardware priority **above**
`configMAX_SYSCALL_INTERRUPT_PRIORITY`. The kernel's critical sections (which mask interrupts only
*up to* that level) can never delay it → zero kernel-induced jitter. In exchange, that ISR must
call **no** FreeRTOS API.

**Alternative:** Keep it at a syscall-safe priority and let it signal a task, accepting a few
microseconds of possible masking latency — only if it actually needs kernel services.

---

### Q40. You need per-task CPU utilization to find which task hogs the processor. How?

**Optimal Approach:** Enable `configGENERATE_RUN_TIME_STATS` with a **high-resolution free-running
timer** (10–20× the tick rate) as the run-time counter, then call `vTaskGetRunTimeStats` to print
absolute and percentage CPU per task. Purpose-built for exactly this.

**Alternative:** A **trace recorder** (SEGGER SystemView / Percepio Tracealyzer) via
`configUSE_TRACE_FACILITY` for a visual timeline — richer insight (scheduling, blocking, ISR
timing) at the cost of tooling setup.

---

## Quick Decision Cheat Sheet

| If the scenario is… | Reach for… |
|---|---|
| ISR → one known task, fastest | Task notification (`vTaskNotifyGiveFromISR`) |
| ISR → possibly many tasks | Binary semaphore |
| Don't lose repeated events (no payload) | Counting semaphore / notification counter |
| Pass data, absorb bursts | Queue |
| Pass a big struct | Queue a pointer + memory pool |
| Only the latest value matters, many readers | Mailbox (`xQueueOverwrite` + `xQueuePeek`) |
| Continuous byte stream, 1→1 | Stream buffer |
| Variable-length framed messages, 1→1 | Message buffer |
| Protect a shared peripheral | Mutex (or a server task + queue) |
| Same task re-locks | Recursive mutex |
| Protect a 2-line update | Critical section |
| Pool of N identical resources | Counting semaphore / queue-of-handles |
| Wait for several events (AND/OR) | Event group |
| Rendezvous / barrier | `xEventGroupSync` |
| Fixed-frequency periodic loop | `vTaskDelayUntil` (or HW timer + notify) |
| Cancelable delayed callback | One-shot software timer |
| Many small periodic jobs | Software timers (shared daemon) |
| Heavy work from an ISR | Defer to a task / `xTimerPendFunctionCall` |
| Block on multiple sources | Queue set |
| Cut idle power | Tickless idle + idle-hook `WFI` |
| No heap / certifiable | Static allocation |
| Isolate real-time work on ESP32 | Pin to a core |

---

*FreeRTOS Scenario Q&A — pairs with `FREE_RTOS_API.md`. Answers assume the classic fixed-priority
preemptive kernel; confirm port/config specifics against your `FreeRTOSConfig.h`.*
