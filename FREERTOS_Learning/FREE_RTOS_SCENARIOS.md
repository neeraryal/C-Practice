# FreeRTOS Real-World Scenarios — Hinglish Interview Workbook

> **No bookish definitions.** Har question: actual situation → **best choice + kyun** → alternative tradeoff → interviewer counter-question. Pehle answer khud bolna, phir explanation read karna.
>
> APIs ke **exact parameters and working code**: [FREE_RTOS_API.md](./FREE_RTOS_API.md). Same project: ESP-IDF/ESP32 ke deviations explicitly mention kiye hain.

## How to answer any RTOS design question (interview template)
1. **Is information sent?** Agar actual data/struct/bytes bhejne hain to queue/message/stream; sirf event signal ho to notification/semaphore.
2. **Who waits?** Exactly one known task = direct notification. Multiple tasks = queue/semaphore/event group based on semantics.
3. **Will producer outrun consumer?** Worst-case burst, queue depth, timeout, drop/backpressure, priority starvation discuss.
4. **Is it shared ownership?** Mutex + inheritance, not binary semaphore.
5. **What is context?** ISR = FromISR only; zero blocking; optional yield on exit.
6. **What's failure behavior?** Zero wait vs nonzero wait, overflow, timeout, stack, deadlines.
7. **What is better alternative?** Choose only after explaining why simplest is sufficient.

## First: 6 timing and task traps

**Tick vs milliseconds:** 100Hz = 10ms/tick; 1000Hz = 1ms/tick. `pdMS_TO_TICKS(500)` converts; `vTaskDelay(500)` means 500 ticks. `xTaskGetTickCount()` returns ticks, not milliseconds.

**Ready ≠ Running:** Event makes blocked high-priority task Ready; scheduler picks it if rules allow. ISR port yield improves switch latency on exit.

**Blocked ≠ Suspended:** Queue/sem/notify wait with positive timeout = Blocked; `vTaskSuspend()` = Suspended until resume. Zero-timeout send fail = remains running/ready.

**taskYIELD ≠ delay:** Yield doesn't wait for a timeout; lower priorities don't get CPU while higher-priority task constantly Ready.

**ESP-IDF stack units:** ESP-IDF xTaskCreate stack depth bytes; standard FreeRTOS uses stack words. Double check version/port.

**No magic precise time:** `vTaskDelayUntil` periodic schedule reduces drift, but hard 1kHz precision needs WCET analysis/hardware.

## The 40 scenarios (in interview order)

---

## A. Signaling & synchronization

### Q1. GPIO interrupt se ek task wake karna?

**Situation samjho:** Interviewer pooch raha hai ki 'gpio interrupt se ek task wake karna' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Direct task notification: ISR mein vTaskNotifyGiveFromISR(handle,&hpw), task mein ulTaskNotifyTake(pdTRUE,portMAX_DELAY).

**Alternative / kab better:** Binary semaphore bhi valid; notification dedicated receiver ke liye lightweight.

**Interviewer counter-question:** ISR gives before wait? Pending notification retained. ISR end par portYIELD_FROM_ISR(hpw) kyun?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q2. ISR burst: 5 interrupts before task runs?

**Situation samjho:** Interviewer pooch raha hai ki 'isr burst: 5 interrupts before task runs' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Counting notification: ISR notification increment kare; task ulTaskNotifyTake(pdFALSE,...) se one-by-one counts consume kare.

**Alternative / kab better:** Counting semaphore max count bound ke saath; queue if each interrupt has payload.

**Interviewer counter-question:** pdTRUE use karoge to repeated interrupts ka count clear hokar events coalesce ho sakte hain; overflow policy?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q3. Sensor + GPS + network teeno ready?

**Situation samjho:** Interviewer pooch raha hai ki 'sensor + gps + network teeno ready' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** EventGroupSetBits on each subsystem; xEventGroupWaitBits(flags,ALL,pdFALSE,pdTRUE,timeout).

**Alternative / kab better:** Three notifications managed by coordinator possible but more bookkeeping.

**Interviewer counter-question:** WaitForAllBits pdFALSE hai to kya hoga? Auto-clear se sticky readiness disappear?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q4. 3 workers ek barrier par wait?

**Situation samjho:** Interviewer pooch raha hai ki '3 workers ek barrier par wait' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** xEventGroupSync(group,myBit,ALL_BITS,timeout) all arrive together.

**Alternative / kab better:** Coordinator + per-worker notifications if custom release logic.

**Interviewer counter-question:** SetBits+WaitBits separately race kyun la sakta? Barrier timeout mein recovery kaise?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q5. Ek event se 3 tasks wake?

**Situation samjho:** Interviewer pooch raha hai ki 'ek event se 3 tasks wake' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Event group bit on common condition; multiple tasks can wait with no/appropriate clearing; task notifications individually to exact 3 handles also possible.

**Alternative / kab better:** Binary semaphore gives one waiter token, broadcast nahi.

**Interviewer counter-question:** xClearOnExit pdTRUE karke kitne waiters release honge? Waiters ke arrival timing matter?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q6. Task A and B ping pong?

**Situation samjho:** Interviewer pooch raha hai ki 'task a and b ping pong' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Each task gets its own notification, exchange notify after producing; shared state protected or payload via queues.

**Alternative / kab better:** Two queues for actual bidirectional payload.

**Interviewer counter-question:** Notification count 1/0 and lost turn ka deadlock kaise avoid karoge?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## B. Passing data & queues

### Q7. Fast producer slow consumer?

**Situation samjho:** Interviewer pooch raha hai ki 'fast producer slow consumer' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Queue length based on worst-case burst, xQueueSend timeout/drop/backpressure policy, consumer xQueueReceive blocking.

**Alternative / kab better:** Latest only: overwrite mailbox; no loss: capacity + throughput requirements re-design.

**Interviewer counter-question:** Producer 10ms, consumer 100ms, length 5, wait=0: producer blocked hota? If highest-priority busy-spin then who runs?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q8. 512-byte struct queue copy expensive?

**Situation samjho:** Interviewer pooch raha hai ki '512-byte struct queue copy expensive' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Fixed pool of buffers; queue pointer/address to owned buffer, consumer returns pointer to free pool.

**Alternative / kab better:** DMA descriptor/ring buffer for hardware streams; direct queue copy if low data rate.

**Interviewer counter-question:** Pointer to stack local queued and function returns, kya crash? Who frees ownership?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q9. Only latest temperature matters?

**Situation samjho:** Interviewer pooch raha hai ki 'only latest temperature matters' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Queue length 1 + xQueueOverwrite(latest,&temp); xQueuePeek for snapshot.

**Alternative / kab better:** Shared latest value guarded by mutex; event notification for freshness.

**Interviewer counter-question:** Peek result data stale hai? Multiple readers and update consistency?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q10. UART ISR bytes continuous receive?

**Situation samjho:** Interviewer pooch raha hai ki 'uart isr bytes continuous receive' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** DMA/ring/stream buffer from RX ISR, parser task stream receive and construct frames.

**Alternative / kab better:** Queue chunks or DMA descriptors; avoid per-byte queue overhead.

**Interviewer counter-question:** StreamBuffer one writer/one reader; overflow/partial send? ISR-safe call?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q11. Variable-sized framed JSON packets?

**Situation samjho:** Interviewer pooch raha hai ki 'variable-sized framed json packets' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** MessageBufferSend complete frame and MessageBufferReceive whole message; dimension max frame + framing overhead.

**Alternative / kab better:** Queue of buffer pointers for multi-producer or large frames.

**Interviewer counter-question:** Receive buffer smaller than next message: data lost or retained?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q12. Two producers one consumer frame transfer?

**Situation samjho:** Interviewer pooch raha hai ki 'two producers one consumer frame transfer' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Serialize writers with mutex around message buffer send, or cleaner tagged queue of message pointers.

**Alternative / kab better:** Dedicated aggregator task merges producers into message buffer.

**Interviewer counter-question:** Message buffer SPSC default: why multi-writer without lock unsafe?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## C. Mutex, sharing & deadlock

### Q13. Two tasks send same UART/SPI?

**Situation samjho:** Interviewer pooch raha hai ki 'two tasks send same uart/spi' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Mutex surrounds transaction; xSemaphoreTake(lock,timeout), send full transaction, xSemaphoreGive(lock).

**Alternative / kab better:** Single owner peripheral task receiving TX requests through queue.

**Interviewer counter-question:** Why binary semaphore cannot solve priority inversion? ISR mutex release possible?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q14. Nested function takes same lock twice?

**Situation samjho:** Interviewer pooch raha hai ki 'nested function takes same lock twice' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Recursive mutex with xSemaphoreTakeRecursive / xSemaphoreGiveRecursive same count.

**Alternative / kab better:** Refactor to internal function that assumes caller owns lock; usually simpler.

**Interviewer counter-question:** Regular mutex re-take by same task causes what? Recursive mutex can still deadlock across tasks?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q15. Priority inversion L/H/M?

**Situation samjho:** Interviewer pooch raha hai ki 'priority inversion l/h/m' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Priority-inheritance mutex: L holds; H waits; L boosted above M until release.

**Alternative / kab better:** Reduce lock time, priority design, server task; binary semaphore lacks inheritance.

**Interviewer counter-question:** Inheritance does NOT solve circular deadlock; two mutex case complexities?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q16. Two-line shared update atomic?

**Situation samjho:** Interviewer pooch raha hai ki 'two-line shared update atomic' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Atomic operation if appropriate, or tiny port-correct critical section.

**Alternative / kab better:** Mutex for larger data structure/any blocking work; avoid critical for I/O.

**Interviewer counter-question:** ESP32 multicore: disabling local interrupts alone protects other core?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q17. Task A takes mutex X then Y; B takes Y then X?

**Situation samjho:** Interviewer pooch raha hai ki 'task a takes mutex x then y; b takes y then x' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Circular wait deadlock; enforce total order X then Y for all tasks, bounded timeouts and cleanup.

**Alternative / kab better:** Dedicated resource server eliminates nested locks.

**Interviewer counter-question:** Priority inheritance helps deadlock? No. Who releases lock on timeout path?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q18. 3 interchangeable DMA channels?

**Situation samjho:** Interviewer pooch raha hai ki '3 interchangeable dma channels' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Counting semaphore initialized 3 for number free, plus protected allocation of actual channel ID; or queue of three handles.

**Alternative / kab better:** Queue of available handles elegantly counts + identifies exact resource.

**Interviewer counter-question:** If semaphore decremented but no actual channel free due to bug, how diagnose?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## D. Timing & periodic work

### Q19. 1kHz hard real-time control loop?

**Situation samjho:** Interviewer pooch raha hai ki '1khz hard real-time control loop' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Hardware timer/peripheral trigger with bounded ISR and/or notify high priority task; measure execution/worst-case jitter.

**Alternative / kab better:** vTaskDelayUntil(1 tick) only soft periodic if tick rate 1kHz and workload fits.

**Interviewer counter-question:** 1ms body exceeds deadline: can any API force exact 1kHz? No. ISR nesting jitter?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q20. Cancelable 5-second debounce/off callback?

**Situation samjho:** Interviewer pooch raha hai ki 'cancelable 5-second debounce/off callback' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** One-shot software timer xTimerReset on press, xTimerStop when cancel; timer callback short.

**Alternative / kab better:** Task waiting notifications with timeout when timer dependency more complex.

**Interviewer counter-question:** xTimerReset second arg 5000 means timer 5s? No, that is timer command queue wait ticks.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q21. 15 periodic housekeeping jobs?

**Situation samjho:** Interviewer pooch raha hai ki '15 periodic housekeeping jobs' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Several software timers for small nonblocking callbacks; worker task for substantial operations.

**Alternative / kab better:** One periodic scheduler task with sorted deadlines/timers if specific requirements.

**Interviewer counter-question:** One callback blocks I2C 2s; impact on other 14 timers?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q22. Timer callback must do blocking I2C?

**Situation samjho:** Interviewer pooch raha hai ki 'timer callback must do blocking i2c' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Timer callback notify/send work command to worker task; worker performs I2C with timeout.

**Alternative / kab better:** Single long-lived task and delayUntil instead of timer.

**Interviewer counter-question:** Timer daemon task blocked: whose timers slip?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## E. Interrupts & deferred work

### Q23. Long ISR parse packet?

**Situation samjho:** Interviewer pooch raha hai ki 'long isr parse packet' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** ISR copies tiny descriptor or gives notification; dedicated task parses outside ISR.

**Alternative / kab better:** DMA ring + task stream parsing; xTimerPendFunctionCall for short deferred function where appropriate.

**Interviewer counter-question:** Calling xQueueReceive with timeout from ISR why illegal?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q24. xQueueSendFromISR vs xQueueSend?

**Situation samjho:** Interviewer pooch raha hai ki 'xqueuesendfromisr vs xqueuesend' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** ISR-specific path cannot block or manipulate normal task wait state same way; use FromISR signature + wake flag.

**Alternative / kab better:** If ISR level can't call kernel, use hardware flag/low-priority deferred ISR route.

**Interviewer counter-question:** What if xQueueSend(...,0) from ISR? Still wrong API.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q25. Unblocked H doesn't run until tick?

**Situation samjho:** Interviewer pooch raha hai ki 'unblocked h doesn't run until tick' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** ISR FreeRTOS API sets hpw then port-specific YIELD_FROM_ISR requests context switch at interrupt exit.

**Alternative / kab better:** Even without explicit switch request scheduling happens later; latency unacceptable often.

**Interviewer counter-question:** Does yielding switch inside ISR? Typically switch requested for ISR return; port rules matter.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q26. Fast interrupt counting without payload?

**Situation samjho:** Interviewer pooch raha hai ki 'fast interrupt counting without payload' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Task notification increment on ISR, ulTaskNotifyTake(pdFALSE) drains count.

**Alternative / kab better:** Counting semaphore bounded counts if multiple waiters.

**Interviewer counter-question:** Overflow possible? If each interrupt uniquely timestamped, count insufficient.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## F. Power, memory & reliability

### Q27. Battery drained by 1ms tick?

**Situation samjho:** Interviewer pooch raha hai ki 'battery drained by 1ms tick' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Enable supported tickless idle; block tasks for long waits; hardware deep/light sleep + wake sources.

**Alternative / kab better:** Lower tick rate when latency allows.

**Interviewer counter-question:** Tickless idle only when task Ready? Never idle if busy-loop; peripheral wake constraints?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q28. No dynamic allocations during runtime?

**Situation samjho:** Interviewer pooch raha hai ki 'no dynamic allocations during runtime' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Static task/queue/semaphore/timer creation where supported, allocate before scheduler start; size memory upfront.

**Alternative / kab better:** Fixed memory pools with bounded behavior.

**Interviewer counter-question:** Static storage lifetime valid? sizeof stack in words vs ESP bytes nuance?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q29. Random corruption after minutes?

**Situation samjho:** Interviewer pooch raha hai ki 'random corruption after minutes' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Check stack watermark, stack overflow hook, heap integrity, buffer bounds, dangling pointers, race.

**Alternative / kab better:** Use guard patterns/trace/MPU where supported.

**Interviewer counter-question:** StackHighWaterMark measures unused min stack, does it prove no out-of-bounds write? No.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q30. Task delete appears heap leak?

**Situation samjho:** Interviewer pooch raha hai ki 'task delete appears heap leak' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Idle task may reclaim task memory when scheduled; application-owned buffers not auto-freed.

**Alternative / kab better:** Prefer tasks persist and block rather than create/delete frequently.

**Interviewer counter-question:** Does vTaskDelete release mutex ownership and malloced app buffers? No.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q31. pvPortMalloc eventually NULL?

**Situation samjho:** Interviewer pooch raha hai ki 'pvportmalloc eventually null' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Fragmentation / leak / out-of-memory; heap_4 coalesces, heap_5 multi-region; inspect largest free block where available.

**Alternative / kab better:** Static/pool allocation for predictable usage.

**Interviewer counter-question:** Free heap bytes large but no contiguous block enough—possible?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q32. Watchdog only fed if all tasks healthy?

**Situation samjho:** Interviewer pooch raha hai ki 'watchdog only fed if all tasks healthy' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Supervisor task checks heartbeat counters/timeouts for all critical tasks, then feeds watchdog only if each fresh.

**Alternative / kab better:** Task-level watchdog registrations if supported.

**Interviewer counter-question:** A task updating heartbeat but stuck making no progress? Need meaningful milestone heartbeat.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.


---

## G. System design & debugging

### Q33. One task waits data queue, command queue, shutdown sem?

**Situation samjho:** Interviewer pooch raha hai ki 'one task waits data queue, command queue, shutdown sem' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Queue set allows select from multiple queue/semaphore members; capacity rules and select-before-receive.

**Alternative / kab better:** Unified tagged event queue often cleaner.

**Interviewer counter-question:** Queue set max pending count sizing? Direct reads race?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q34. Picking priorities for deadlines?

**Situation samjho:** Interviewer pooch raha hai ki 'picking priorities for deadlines' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Measure response times and WCET; assign urgency/deadline, block when idle, avoid prolonged critical sections.

**Alternative / kab better:** Rate-monotonic fixed priority for suitable periodic assumptions; deadline analysis essential.

**Interviewer counter-question:** High priority ≠ important. Can CPU 120% busy meet deadlines? No.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q35. Equal priority CPU bound, unfair scheduling?

**Situation samjho:** Interviewer pooch raha hai ki 'equal priority cpu bound, unfair scheduling' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Check configUSE_TIME_SLICING, ticks, affinity, critical sections, and actual readiness; prefer blocking/event driven.

**Alternative / kab better:** Explicit taskYIELD can help equal-priority peers but doesn't fix high priority starvation.

**Interviewer counter-question:** Is time slicing guaranteed on ESP-IDF SMP as ideal round robin? No.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q36. Which task stack overflowing??

**Situation samjho:** Interviewer pooch raha hai ki 'which task stack overflowing?' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Check uxTaskGetStackHighWaterMark for each, enable configCHECK_FOR_STACK_OVERFLOW, inspect crash/backtrace.

**Alternative / kab better:** Static stack instrumentation, debug tracing.

**Interviewer counter-question:** ESP-IDF watermark byte units vs vanilla words?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q37. Release build HardFault when ISR fires?

**Situation samjho:** Interviewer pooch raha hai ki 'release build hardfault when isr fires' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Check interrupt priority constraints, API FromISR usage, IRAM safety, stack and optimization-sensitive undefined behavior.

**Alternative / kab better:** Validate assert hooks, backtrace, Cortex-M configMAX_SYSCALL_INTERRUPT_PRIORITY or ESP-specific interrupt level.

**Interviewer counter-question:** Why does code work in debug? Timing/UB differences; do not assert only one root cause.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q38. ESP32 Wi-Fi and control interference?

**Situation samjho:** Interviewer pooch raha hai ki 'esp32 wi-fi and control interference' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Pin appropriately to cores if supported; respect IDF Wi-Fi/system task constraints; measure latency and shared contention.

**Alternative / kab better:** Task priority + offload via DMA / peripheral hardware + lock discipline.

**Interviewer counter-question:** Pinned task still uses shared memory/bus; core isolation enough? No.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q39. Ultra-fast motor commutation ISR?

**Situation samjho:** Interviewer pooch raha hai ki 'ultra-fast motor commutation isr' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Hardware peripheral/timer where possible; high-priority ISR that cannot call FreeRTOS, writes minimal hardware state; deferred worker for RTOS.

**Alternative / kab better:** Hardware PWM/MCPWM to remove latency sensitive work from software.

**Interviewer counter-question:** Can high-level ISR call xQueueSendFromISR? Not necessarily; priority constraints apply.

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

### Q40. Who hogs CPU??

**Situation samjho:** Interviewer pooch raha hai ki 'who hogs cpu?' ko real firmware mein kaise handle karoge. Pehle data vs signal, task vs ISR, aur bounded timeout decide karo.

**Best approach — aur kyun:** Enable runtime stats with suitable monotonic counter and config; uxTaskGetSystemState / vTaskGetRunTimeStats; trace task load.

**Alternative / kab better:** GPIO timing, tracing, ESP-IDF task stats/heap tracing.

**Interviewer counter-question:** Stats counter frequency and idle percentage interpretation?

**Failure-mode answer:** Return value verify karo, maximum blocking interval choose karo, aur CPU starvation, missed event, wrong ISR-context, overflow ya latency ko explicitly handle karo. Har design mein bounded memory + producer/consumer rate check karo.

---

## Full code walkthrough 1 — Producer fast, consumer slow, QUEUE FULL

**Question:** Producer every 10ms sensor value generate karta hai, consumer har 100ms ek value process karta, queue capacity 5 hai. Full queue pe exact behavior? Task kab Blocked?

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
static QueueHandle_t samples;

static void produce(void *arg) {
    (void)arg;
    int n = 0;
    for (;;) {
        int value = n++;
        BaseType_t ok = xQueueSend(samples, &value, 0);
        if (ok != pdPASS) {
            // Full, zero wait => NOT blocked. Item not accepted.
            printf("queue full at tick %lu\n",
                   (unsigned long)xTaskGetTickCount());
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // THIS does block periodically
    }
}
static void consume(void *arg) {
    (void)arg;
    int v;
    for (;;) {
        if (xQueueReceive(samples, &v, portMAX_DELAY) == pdPASS) {
            printf("consumed %d\n", v);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
void start_demo(void) {
    samples = xQueueCreate(5, sizeof(int));
    configASSERT(samples);
    configASSERT(xTaskCreate(produce, "P", 2048, NULL, 3, NULL) == pdPASS);
    configASSERT(xTaskCreate(consume, "C", 2048, NULL, 2, NULL) == pdPASS);
}
// Stack depth numbers are BYTES in ESP-IDF, STACK WORDS in vanilla FreeRTOS.
// Standard FreeRTOS app starts scheduler in platform setup; ESP-IDF already runs it.
```

**Step-by-step timeline, conceptually:** producer ~10 items per 100ms, consumer ~1 per 100ms. Queue fills after short burst. `xQueueSend(...,0)` returns **pdFAIL** when full—no queue-induced Blocked transition. The producer does block during its separate `vTaskDelay`. Consumer may wait on empty queue initially, but once processing a message it enters 100ms delay. **If producer delay is removed and it busy-spins at strictly higher priority, consumer may starve permanently.**

**What if third parameter = 50 ticks?** On full queue, producer goes Blocked waiting for space **up to 50 ticks**; wakes when consumer receives, or timeout. It is not a 50-millisecond value. Actual scheduler tick rate decides time.

**Interviewer sequence:** Who decides blocked? → queue full and wait 0? → queue full and wait 50? → higher priority busy loop? → how to avoid data loss? Answer: explicit error policy/backpressure/buffer sizing/throughput requirements, not blindly raising queue depth.

## Full code walkthrough 2 — GPIO ISR → task notification

```c
#include "FreeRTOS.h"
#include "task.h"
static TaskHandle_t workerHandle = NULL;

static void irq_worker(void *arg) {
    (void)arg;
    for (;;) {
        uint32_t oldCount = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        (void)oldCount;
        process_one_irq();
    }
}
void setup_worker(void) {
    xTaskCreate(irq_worker, "IRQ-work", 2048, NULL, 4, &workerHandle);
}
void gpio_irq_handler_example(void) {
    BaseType_t hpw = pdFALSE;
    vTaskNotifyGiveFromISR(workerHandle, &hpw);
    portYIELD_FROM_ISR(hpw); // Verify macro for your specific port.
}
```

**Why best?** Single receiver, no payload, ISR must be short. Count accumulates; `pdFALSE` consumes one count at a time. If all ISR occurrences independently carry data, counter alone insufficient → queue of event records.

**Counter:** `pdTRUE` and `pdFALSE` difference? With pdTRUE notification value reset to 0; pdFALSE subtracts 1. `portYIELD_FROM_ISR` task context function nahi, ISR exit scheduling request hai.

## Full code walkthrough 3 — Task A and Task B share UART: mutex

```c
#include "FreeRTOS.h"
#include "semphr.h"
static SemaphoreHandle_t txMutex;

void init_bus(void) {
    txMutex = xSemaphoreCreateMutex();
    configASSERT(txMutex);
}
BaseType_t send_frame(const void *frame, size_t len) {
    if (xSemaphoreTake(txMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return pdFALSE;
    // UART driver transaction must be completed safely before unlock;
    // if async DMA, lock lifecycle needs DMA completion ownership rules.
    uart_write_frame(frame, len);
    xSemaphoreGive(txMutex);
    return pdTRUE;
}
```

**Why mutex:** Same owner locks/unlocks, priority inheritance protects H waiting for L where supported. **Binary semaphore** can signal from ISR, but has no owner/priority inheritance, so not equivalent for resource access.

## Full code walkthrough 4 — AND readiness + 100ms periodic task

```c
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#define SENSOR_READY (1U << 0)
#define NET_READY    (1U << 1)
static EventGroupHandle_t flags;

static void app(void *arg) {
    (void)arg;
    xEventGroupWaitBits(flags, SENSOR_READY | NET_READY,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    TickType_t last = xTaskGetTickCount();
    TickType_t period = pdMS_TO_TICKS(100);
    configASSERT(period > 0);
    for (;;) {
        do_short_periodic_work();
        vTaskDelayUntil(&last, period);
    }
}
// Setup flags = xEventGroupCreate(); init tasks SetBits as ready.
```

**Why:** Readiness is a set of persistent conditions, not count of occurrences. `pdTRUE` 4th param means AND. DelayUntil reduces period drift, **no hard exact execution guarantee** if work overruns.

---

## Fast primitive decision chart

| Problem asked | Start with | Avoid because |
|---|---|---|
| One known task wake | task notification | semaphore overhead unless multiple waiters |
| Count repeated signal events | counted notify / counting semaphore | binary loses repeated gives |
| Actual structs between tasks | queue | semaphore can't carry payload |
| Only latest value | overwrite queue of length 1 | backlog adds stale readings |
| Task waits for multiple readiness flags | event group AND/OR | bare binary semaphore has no bit identity |
| Many tasks listen to state | event group / explicit notify fan-out | single semaphore give doesn't broadcast |
| Shared UART/SPI/critical resource | mutex | binary semaphore lacks ownership/inheritance |
| Few CPU instructions shared | short critical / atomics | holding mutex more than needed |
| Bulk UART bytes | stream buffer / DMA ring | queue of individual bytes unnecessary |
| Variable length framed messages | message buffer | stream buffer doesn't preserve frames |
| Need bounded fixed periodic scheduling | vTaskDelayUntil | vTaskDelay accumulates work-time drift |
| Absolute 1kHz tight jitter | hardware timer + measured handler | scheduler timing not exact by itself |
| Brief delayed nonblocking operation | software timer | task for every timer wastes stack |
| Multiple queue sources | queue set / common tagged queue | polling burns CPU |
| ISR long work | FromISR notify + worker | no blocking / heavy processing inside ISR |
| Fixed N pooled resources | counting sem + IDs / queue of handles | single mutex serializes everything |

## 30-second answers interviewers expect

**Blocked kab hoga?** Delay API se; queue full on SEND with xTicksToWait > 0; queue empty on RECEIVE with wait > 0; semaphore not available and waiting; notification/event wait. Scheduler READY hone tak nahi run karata.

**Starvation kab?** Higher-priority task constantly Ready rahe aur lower never gets CPU; full queue + tight producer retry classic trap. Delay/block/yield scheduling semantics distinguish karo.

**Mutex vs semaphore?** Mutex protect shared ownership + inheritance; semaphore signal or count resources/events, no mutex-style ownership.

**Why FromISR?** ISR block nahi ho sakta. Wake flag + port yield lower latency possible karta hai. Some high-priority interrupts cannot call ANY FreeRTOS API.

**Tick rate kya?** 100Hz means 10ms between regular ticks; xTaskGetTickCount counts ticks; convert with pdMS_TO_TICKS; scheduler tick != guaranteed CPU execution instant.

**What is port?** Processor/compiler/architecture-specific FreeRTOS implementation—context switching, interrupt handling, stack layout, tick mechanics.

**Who should choose task priority?** Deadline and measured execution budget, not feature importance; short critical sections, bounded wait and system load analysis.

---

## Revision plan

**Pass 1:** API companion's `xTaskCreate`, ticks, `vTaskDelay`, `xQueueSend`, `xQueueReceive`, `xSemaphoreTake` and `ulTaskNotifyTake` parameters explain out loud.

**Pass 2:** Q1–Q40 without seeing answers: identify context, payload, receivers, blocking, best primitive and alternatives.

**Pass 3:** Four walkthroughs trace task states after *every line*. Counter-questions modify wait=0 to 50 ticks, change task priorities and remove delays.

**Pass 4:** ESP32 target specifics: stack depth bytes, pinned tasks, SMP critical sections, ISR priority and IDF config. Cross-check installed IDF version before compiling.

*Not all snippets compile standalone: hardware handlers (`process_one_irq`, `uart_write_frame`, etc.), system initialization, GPIO ISR registration and driver hooks are project-specific placeholders. All key RTOS calls have their parameters described in the [API guide](./FREE_RTOS_API.md).*

---

## Advanced scenario practice — Q41 to Q70

Yeh **additional interview counter-questions** hain. Pehle khud solve karo: payload vs signal, wait/timeout, task priority, ISR restrictions aur failure policy. API arguments aur commented C code: [API Hinglish notes](./FREE_RTOS_API.md).

### Q41. Queue full aur high-priority producer busy retry kare?

**Situation aur best approach:** xQueueSend(...,0) immediately fail; busy producer consumer ko CPU nahi de sakta. Bounded timeout/drop/backpressure use karo.

**Counter-question:** Producer xQueueSend(...,50 ticks) kare aur consumer 10 ticks mein read kare to state transition kya?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q42. Queue mein local struct ka pointer send ho?

**Situation aur best approach:** Queue pointer ki COPY karti hai, struct ki nahi; function return ke baad dangling. Queue struct by value ya owned static memory-pool pointer use karo.

**Counter-question:** xQueueSend pdPASS hone ke baad bhi consumer corrupted data kyun dekh sakta hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q43. Two tasks shared counter++ use karein?

**Situation aur best approach:** Counter++ read-modify-write race; suitable atomics ya short SMP-safe critical section use karo; longer work ke liye mutex.

**Counter-question:** volatile int se race solve ho jayegi?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q44. Suspend / resume se event signal karein?

**Situation aur best approach:** xTaskResume before vTaskSuspend ho jaaye to wake lost; notification/semaphore pending event ke liye safer.

**Counter-question:** Event pehle aa gaya, waiting task baad mein enter kare—kaun si primitive pending event retain karti?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q45. xQueueReceive timeout ke baad old value process ho?

**Situation aur best approach:** Only successful pdPASS par output buffer ka naya data valid maano; pdFAIL on timeout par stale variable reh sakta.

**Counter-question:** Timeout pe last valid value use karni ho to freshness kaise record karoge?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q46. GPIO ISR queue full paaye?

**Situation aur best approach:** xQueueSendFromISR nonblocking fail karega. Count drops and choose ring buffer/DMA, depth or throughput improvement.

**Counter-question:** Can ISR wait 20ms for queue space?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q47. UART DMA transaction mutex release ke baad bhi chal rahi?

**Situation aur best approach:** Mutex only start tak rakhna unsafe ho sakta hai; complete transaction, buffer lifetime aur DMA-done ownership protocol design karo.

**Counter-question:** What if TX input pointer stack pe tha?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q48. 1ms periodic loop with 100Hz tick?

**Situation aur best approach:** 100Hz tick = 10ms; 1ms pdMS_TO_TICKS often zero. Hardware timer/peripheral or redesigned tick configuration use karo.

**Counter-question:** vTaskDelay(1) kitne milliseconds hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q49. 16-bit tick wraps around?

**Situation aur best approach:** TickType_t unsigned delta approach carefully choose, long absolute comparison naive na rakho; overflow-aware kernel helpers.

**Counter-question:** 100Hz pe 16-bit counter kab wrap hota? ~655.36 seconds.

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q50. Multiple producer message buffer share karein?

**Situation aur best approach:** Message buffer normally one writer/one reader assumption; serialize writers with mutex or use shared typed queue/pool.

**Counter-question:** Multiple writes without synchronization kab corrupt protocol?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q51. Event group bit repeated interrupts count kare?

**Situation aur best approach:** Event group sets only bits; repeated same event coalesces. Counting notification/semaphore or event queue use karo.

**Counter-question:** Three times set bit = count 3?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q52. Software timer callback blocks I2C for 2 seconds?

**Situation aur best approach:** Shared timer service task block hoti hai, baaki timer callbacks slip. Worker task ko queue/notify bhejo.

**Counter-question:** Timer priority increase karne se blocking callback ka issue fix hoga?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q53. ESP32 two cores both increment shared variable?

**Situation aur best approach:** Local interrupt disable on one core enough nahi. SMP-safe spinlock/atomic/mutex based on context use karo.

**Counter-question:** Pinned-to-core task hone se shared data automatically safe hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q54. Resource deadlock with two mutexes?

**Situation aur best approach:** Task A takes X then Y, B takes Y then X -> circular wait. Always consistent lock order, bounded waits.

**Counter-question:** Priority inheritance deadlock solve karta hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q55. Binary semaphore multiple tasks ko broadcast kare?

**Situation aur best approach:** One Give one token/waiter unblock karta hai, broadcast nahi. Event group persistent flag / multiple notifications use karo.

**Counter-question:** One Give pe three waiters simultaneously wake honge?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q56. Counting semaphore maximum pe aur give?

**Situation aur best approach:** Maximum count pe xSemaphoreGive fails; unlimited lossless event storage nahi. Overflow counter or rate/capacity control.

**Counter-question:** Does binary vs counting change payload support?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q57. Queue length grow karke overload solve kare?

**Situation aur best approach:** Long-term producer rate > consumer rate to any finite queue eventually full. Throughput, batching, backpressure, drops/parallel consumer address karo.

**Counter-question:** 10ms produce, 100ms consume: queue 100 se indefinite fix?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q58. Heap enough free but allocation fails?

**Situation aur best approach:** Fragmentation: enough total bytes but no sufficiently large contiguous region. Heap scheme, largest block, leak and fixed pools inspect karo.

**Counter-question:** xPortGetFreeHeapSize se requested large alloc guaranteed?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q59. Deleted task ka handle dusra task use kare?

**Situation aur best approach:** Stale handle can be invalid. Graceful stop protocol and ownership synchronization; application buffers/mutex not auto-cleaned.

**Counter-question:** vTaskDelete(NULL) se caller-owned heap free hoti?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q60. Watchdog heartbeat increment while task is stuck?

**Situation aur best approach:** Liveness heartbeat ko meaningful progress/deadlines se tie karo. Supervisor only when all workers completed useful work.

**Counter-question:** Busy-loop task heartbeat badhaye, kya healthy hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q61. High-priority ISR cannot call FreeRTOS?

**Situation aur best approach:** Port-specific interrupt priority/level rules; forbidden ISR mein FromISR bhi invalid. Do minimal hardware work, defer to allowed ISR/task.

**Counter-question:** portYIELD_FROM_ISR invalid priority fix karta?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q62. Task blocked on event and scheduler suspended?

**Situation aur best approach:** vTaskSuspendAll scheduler ko suspend karta, interrupts necessarily disabled nahi; suspended period mein blocking calls mat karo.

**Counter-question:** Is it safe to xQueueReceive(...,portMAX_DELAY) while scheduler suspended?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q63. Notification overwrite loses older data?

**Situation aur best approach:** eSetValueWithOverwrite latest 32-bit value keeps; previous data replace. For every payload queue better; eSetValueWithoutOverwrite can fail pending.

**Counter-question:** Counter notification se payload history milegi?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q64. Sensor has 3 ready conditions, one goes offline?

**Situation aur best approach:** Event group status bits use karo; disconnect pe clear; xEventGroupWaitBits AND as needed. Avoid stale sticky ready flag.

**Counter-question:** xClearOnExit pdTRUE hona chahiye if readiness is ongoing level?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q65. Two equal-priority CPU-bound tasks unevenly run?

**Situation aur best approach:** Check time slicing, affinity, kernel config and task Ready state; yield equal-priority peers, best block on events.

**Counter-question:** taskYIELD se lower-priority task ko chance milta?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q66. Logging adds latency to control task?

**Situation aur best approach:** printf can lock/block/UART slow; bounded deferred logging queue or trace/hardware timestamps.

**Counter-question:** ISR ke andar printf real-time safe assume kar sakte?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q67. Output buffer too small for next message?

**Situation aur best approach:** MessageBufferReceive may return 0 and leave too-large message pending; pre-size buffer according to protocol maximum.

**Counter-question:** Stream buffer and message buffer framing ka difference?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q68. Task priority inversion with medium task?

**Situation aur best approach:** Mutex priority inheritance boosts low owner when high waiter blocks; keep critical section short. Binary semaphore lacks inheritance.

**Counter-question:** Does inheritance solve nested mutex/deadlock?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q69. Queue holds pointer to freed heap buffer?

**Situation aur best approach:** Sender and receiver must agree ownership transfer; consumer uses pointer only while buffer valid. Pool/free queue or value copies.

**Counter-question:** Queue pointer ko deep copy karti hai?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.

### Q70. Multiple readiness signals with AND vs OR?

**Situation aur best approach:** xEventGroupWaitBits fourth arg pdTRUE AND; pdFALSE OR; third arg clear-on-exit controls flag lifetime.

**Counter-question:** First flag set, second set after 5s; waiter kab READY?

**Alternative choose karte waqt:** Actual payload ho to queue/buffer, sirf single-task event ho to notification; shared resource owner ho to mutex; ISR ho to FromISR + correct interrupt priority. Timeout/overflow bhi answer mein discuss karo.



## Two new code-tracing exercises

### Queue full with zero timeout

```c
QueueHandle_t q = xQueueCreate(2, sizeof(int)); // Do int ki capacity
int a=10, b=20, c=30, out=0;
xQueueSend(q, &a, 0);                           // 1 item; no blocking
xQueueSend(q, &b, 0);                           // 2 items; queue full
BaseType_t result = xQueueSend(q, &c, 0);       // pdFAIL immediately; NOT Blocked
if (result != pdPASS) {
    // 'c' drop/retry ka explicit decision lo, silently ignore mat karo
}
xQueueReceive(q, &out, 0);                      // 'a' dequeue; ab ek slot free
```

**Interviewer:** Agar third parameter 50 ticks ho aur consumer 10 ticks baad dequeue kare? Sender full queue pe Blocked jaata, space available hote hi Ready hota, scheduling permit kare to send success hota. 50 *ticks* hai, milliseconds nahi.

### ISR notification counting

```c
// Worker task context:
for (;;) {
    uint32_t previousCount = ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    // Count zero tha to wait mein Blocked; pdFALSE one count consume karta hai
    // Return value purani pending count hai; yahan one event process karo
    process_one_event();
}

// ISR context (sirf FreeRTOS-compatible interrupt level par):
BaseType_t hpw = pdFALSE;                   // Pehle flag false
vTaskNotifyGiveFromISR(workerHandle, &hpw); // Event pending count increment
portYIELD_FROM_ISR(hpw);                    // ISR ke end par scheduling request
```

**Interviewer:** pdTRUE use karte to? Pending event count zero ho jaata, burst event occurrences coalesce ho sakti hain. Bounded counters bhi overflow kar sakte hain.

---

*Q41–Q70 interview-focused extension; examples illustrative hain, hardware hooks and ISR registration target-specific honge.*
