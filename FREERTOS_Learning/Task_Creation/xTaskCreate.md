## Task Creation syntax

```c
BaseType_t xTaskCreate( TaskFunction_t pvTaskCode,
                         const char * const pcName,
                         const configSTACK_DEPTH_TYPE uxStackDepth,
                         void *pvParameters,
                         UBaseType_t uxPriority,
                         TaskHandle_t *pxCreatedTask
                       );
```

pvTaskCode → Task function (entry point), must not return

pcName → Task name (for debugging / handle lookup)

uxStackDepth → Stack size in words (not bytes)

pvParameters → Parameter passed to task (must stay valid)

uxPriority → Task priority (optionally privileged)

pxCreatedTask → Output handle of created task (can be NULL)

Return value

pdPASS → Task created successfully

errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY → Not enough memory

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DELAY_PERIOD 1000
TaskHandle_t xHandle = NULL;

void task(void * param)
{
  int *n=(int *)(param);
  TickType_t tickValue = xTaskGetTickCount();
  // printf("Size of TickYpe is :  %zu \n", sizeof(tickValue));
  for(int i=0 ;i<*n;i++)
  {
    TickType_t tickValue = xTaskGetTickCount();
    printf("Tick Value : %ld :: Hello Task \n",tickValue);
    vTaskDelay(DELAY_PERIOD/portTICK_PERIOD_MS);
  }
  vTaskDelete(xHandle);
}

void app_main() {
  int n=10;
  BaseType_t xReturned;
  xReturned=xTaskCreate(task , "task", 1024 ,(void*)&n, 0, xHandle);
  if(xReturned == pdPASS)
  {
    printf("Task was created successfully \n");
  }
  else 
  {
    printf("Task was not created successfully \n");
  }
  while (true) {
    TickType_t tickValue = xTaskGetTickCount();
    printf("Tick Value : %ld  :: Inside main\n",tickValue);
    // printf("Inside main \n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
```

## Output

```c
Task was created successfully 
Tick Value : 0  :: Inside main
Tick Value : 1 :: Hello Task 
Tick Value : 100  :: Inside main
Tick Value : 102 :: Hello Task 
Tick Value : 200  :: Inside main
Tick Value : 203 :: Hello Task 
Tick Value : 300  :: Inside main
Tick Value : 304 :: Hello Task 
Tick Value : 400  :: Inside main
Tick Value : 405 :: Hello Task 
Tick Value : 500  :: Inside main
Tick Value : 506 :: Hello Task 
Tick Value : 600  :: Inside main
Tick Value : 607 :: Hello Task 
Tick Value : 700  :: Inside main
Tick Value : 708 :: Hello Task 
Tick Value : 800  :: Inside main
Tick Value : 809 :: Hello Task 
Tick Value : 900  :: Inside main
Tick Value : 910 :: Hello Task 
Tick Value : 1000  :: Inside main
Tick Value : 1100  :: Inside main
```


# Preemption (Higher priority and Lower Priority)

```c

Higher priority task when wakes and gets in to ready state, scheduler will check if the current running task has low priority, if yes , there it will preempt the current running task move it to the end of the running list.
```
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RTOS_PRINT(fmt, ...) \
    printf("[Tick:%lu] " fmt "\r\n", (unsigned long)xTaskGetTickCount(), ##__VA_ARGS__)

void MyTask(void *param)
{
    RTOS_PRINT("MyTask started");
    while(1)
    {
      RTOS_PRINT("Inside My Task ");
      vTaskDelay(500/portTICK_PERIOD_MS); //Blocked state
    }
}

void app_main(void)
{
    printf("app_main start\n");
    printf("TICK FREQ :%ld \n",portTICK_PERIOD_MS);
    xTaskCreate(
        MyTask,          // Task function
        "MyTask",        // Name (debug only)
        4096,            // Stack size (bytes)
        NULL,            // Parameter
        3,               // Priority
        NULL             // Task handle
    );

  while(1)
  {
    for(int i=0 ;i<1000;i++)
    {
      RTOS_PRINT("Inside Main : %d ",i); 
    }
    vTaskDelay(700/portTICK_PERIOD_MS); //Blocked state
  }
}

```

```c
Here Main task is printing from 1 to 1000 times, but when in between , after 50 ticks , MyTask becomes ready it will prempt the MainTask and will complete its execution untill it is blocked, only after it gets blocked Maintask will get the CPU cycles for its execution 
```
### Logs
```c
[Tick:0] MyTask started
[Tick:0] Inside My Task 
[Tick:0] Inside Main : 0 
[Tick:0] Inside Main : 1 
[Tick:0] Inside Main : 2 
[Tick:0] Inside Main : 3 
[Tick:0] Inside Main : 4 
[Tick:0] Inside Main : 5 
[Tick:1] Inside Main : 6 
[Tick:1] Inside Main : 7 
[Tick:1] Inside Main : 8 
[Tick:1] Inside Main : 9 
[Tick:2] Inside Main : 10 
[Tick:2] Inside Main : 11 
[Tick:2] Inside Main : 12 
[Tick:2] Inside Main : 13 
[Tick:3] Inside Main : 14 
[Tick:3] Inside Main : 15 
[Tick:3] Inside Main : 16 
[Tick:3] Inside Main : 17 
[Tick:4] Inside Main : 18 
[Tick:4] Inside Main : 19 
[Tick:4] Inside Main : 20 
[Tick:4] Inside Main : 21 
[Tick:5] Inside Main : 22 
[Tick:5] Inside Main : 23 
[Tick:5] Inside Main : 24 
[Tick:5] Inside Main : 25 
[Tick:6] Inside Main : 26 
[Tick:6] Inside Main : 27 
[Tick:6] Inside Main : 28 
[Tick:6] Inside Main : 29 
[Tick:7] Inside Main : 30 
[Tick:7] Inside Main : 31 
[Tick:7] Inside Main : 32 
[Tick:7] Inside Main : 33 
[Tick:8] Inside Main : 34 
[Tick:8] Inside Main : 35 
[Tick:8] Inside Main : 36 
[Tick:8] Inside Main : 37 
[Tick:9] Inside Main : 38 
[Tick:9] Inside Main : 39 
[Tick:9] Inside Main : 40 
[Tick:9] Inside Main : 41 
[Tick:10] Inside Main : 42 
[Tick:10] Inside Main : 43 
[Tick:10] Inside Main : 44 
[Tick:10] Inside Main : 45 
[Tick:11] Inside Main : 46 
[Tick:11] Inside Main : 47 
[Tick:11] Inside Main : 48 
[Tick:12] Inside Main : 49 
[Tick:12] Inside Main : 50 
[Tick:12] Inside Main : 51 
[Tick:12] Inside Main : 52 
[Tick:13] Inside Main : 53 
[Tick:13] Inside Main : 54 
[Tick:13] Inside Main : 55 
[Tick:13] Inside Main : 56 
[Tick:14] Inside Main : 57 
[Tick:14] Inside Main : 58 
[Tick:14] Inside Main : 59 
[Tick:14] Inside Main : 60 
[Tick:15] Inside Main : 61 
[Tick:15] Inside Main : 62 
[Tick:15] Inside Main : 63 
[Tick:15] Inside Main : 64 
[Tick:16] Inside Main : 65 
[Tick:16] Inside Main : 66 
[Tick:16] Inside Main : 67 
[Tick:16] Inside Main : 68 
[Tick:17] Inside Main : 69 
[Tick:17] Inside Main : 70 
[Tick:17] Inside Main : 71 
[Tick:18] Inside Main : 72 
[Tick:18] Inside Main : 73 
[Tick:18] Inside Main : 74 
[Tick:18] Inside Main : 75 
[Tick:19] Inside Main : 76 
[Tick:19] Inside Main : 77 
[Tick:19] Inside Main : 78 
[Tick:19] Inside Main : 79 
[Tick:20] Inside Main : 80 
[Tick:20] Inside Main : 81 
[Tick:20] Inside Main : 82 
[Tick:20] Inside Main : 83 
[Tick:21] Inside Main : 84 
[Tick:21] Inside Main : 85 
[Tick:21] Inside Main : 86 
[Tick:21] Inside Main : 87 
[Tick:22] Inside Main : 88 
[Tick:22] Inside Main : 89 
[Tick:22] Inside Main : 90 
[Tick:22] Inside Main : 91 
[Tick:23] Inside Main : 92 
[Tick:23] Inside Main : 93 
[Tick:23] Inside Main : 94 
[Tick:24] Inside Main : 95 
[Tick:24] Inside Main : 96 
[Tick:24] Inside Main : 97 
[Tick:24] Inside Main : 98 
[Tick:25] Inside Main : 99 
[Tick:25] Inside Main : 100 
[Tick:25] Inside Main : 101 
[Tick:25] Inside Main : 102 
[Tick:26] Inside Main : 103 
[Tick:26] Inside Main : 104 
[Tick:26] Inside Main : 105 
[Tick:26] Inside Main : 106 
[Tick:27] Inside Main : 107 
[Tick:27] Inside Main : 108
[Tick:27] Inside Main : 109 
[Tick:28] Inside Main : 110 
[Tick:28] Inside Main : 111 
[Tick:28] Inside Main : 112 
[Tick:28] Inside Main : 113 
[Tick:29] Inside Main : 114 
[Tick:29] Inside Main : 115 
[Tick:29] Inside Main : 116 
[Tick:29] Inside Main : 117 
[Tick:30] Inside Main : 118 
[Tick:30] Inside Main : 119 
[Tick:30] Inside Main : 120 
[Tick:30] Inside Main : 121 
[Tick:31] Inside Main : 122 
[Tick:31] Inside Main : 123 
[Tick:31] Inside Main : 124 
[Tick:32] Inside Main : 125 
[Tick:32] Inside Main : 126 
[Tick:32] Inside Main : 127 
[Tick:32] Inside Main : 128 
[Tick:33] Inside Main : 129 
[Tick:33] Inside Main : 130 
[Tick:33] Inside Main : 131 
[Tick:33] Inside Main : 132 
[Tick:34] Inside Main : 133 
[Tick:34] Inside Main : 134 
[Tick:34] Inside Main : 135 
[Tick:35] Inside Main : 136 
[Tick:35] Inside Main : 137 
[Tick:35] Inside Main : 138 
[Tick:35] Inside Main : 139 
[Tick:36] Inside Main : 140 
[Tick:36] Inside Main : 141 
[Tick:36] Inside Main : 142 
[Tick:36] Inside Main : 143 
[Tick:37] Inside Main : 144 
[Tick:37] Inside Main : 145 
[Tick:37] Inside Main : 146 
[Tick:37] Inside Main : 147 
[Tick:38] Inside Main : 148 
[Tick:38] Inside Main : 149 
[Tick:38] Inside Main : 150 
[Tick:39] Inside Main : 151 
[Tick:39] Inside Main : 152 
[Tick:39] Inside Main : 153 
[Tick:39] Inside Main : 154 
[Tick:40] Inside Main : 155
[Tick:40] Inside Main : 156 
[Tick:40] Inside Main : 157 
[Tick:40] Inside Main : 158 
[Tick:41] Inside Main : 159 
[Tick:41] Inside Main : 160 
[Tick:41] Inside Main : 161 
[Tick:42] Inside Main : 162 
[Tick:42] Inside Main : 163 
[Tick:42] Inside Main : 164 
[Tick:42] Inside Main : 165 
[Tick:43] Inside Main : 166 
[Tick:43] Inside Main : 167 
[Tick:43] Inside Main : 168 
[Tick:43] Inside Main : 169 
[Tick:44] Inside Main : 170 
[Tick:44] Inside Main : 171 
[Tick:44] Inside Main : 172 
[Tick:44] Inside Main : 173 
[Tick:45] Inside Main : 174 
[Tick:45] Inside Main : 175 
[Tick:45] Inside Main : 176 
[Tick:46] Inside Main : 177 
[Tick:46] Inside Main : 178 
[Tick:46] Inside Main : 179 
[Tick:46] Inside Main : 180 
[Tick:47] Inside Main : 181 
[Tick:47] Inside Main : 182 
[Tick:47] Inside Main : 183 
[Tick:47] Inside Main : 184 
[Tick:48] Inside Main : 185 
[Tick:48] Inside Main : 186 
[Tick:48] Inside Main : 187 
[Tick:48] Inside Main : 188 
[Tick:49] Inside Main : 189 
[Tick:49] Inside Main : 190 
[Tick:49] Inside Main : 191 

[Tick:50] Inside My Task 

[Tick:50] Inside Main : 192 
[Tick:50] Inside Main : 193 
[Tick:50] Inside Main : 194 
[Tick:51] Inside Main : 195 
[Tick:51] Inside Main : 196 
[Tick:51] Inside Main : 197 
[Tick:51] Inside Main : 198 
[Tick:52] Inside Main : 199 
[Tick:52] Inside Main : 200 
[Tick:52] Inside Main : 201 
[Tick:53] Inside Main : 202 
[Tick:53] Inside Main : 203 
[Tick:53] Inside Main : 204 
[Tick:53] Inside Main : 205 
[Tick:54] Inside Main : 206 
[Tick:54] Inside Main : 207 
[Tick:54] Inside Main : 208 
[Tick:54] Inside Main : 209 
[Tick:55] Inside Main : 210 
[Tick:55] Inside Main : 211 
[Tick:55] Inside Main : 212 
[Tick:55] Inside Main : 213 
[Tick:56] Inside Main : 214 
[Tick:56] Inside Main : 215 
[Tick:56] Inside Main : 216 
[Tick:57] Inside Main : 217 
[Tick:57] Inside Main : 218 
[Tick:57] Inside Main : 219 
[Tick:57] Inside Main : 220 
[Tick:58] Inside Main : 221 
[Tick:58] Inside Main : 222 
[Tick:58] Inside Main : 223 
[Tick:58] Inside Main : 224 
[Tick:59] Inside Main : 225 
[Tick:59] Inside Main : 226 
[Tick:59] Inside Main : 227 
[Tick:60] Inside Main : 228 
[Tick:60] Inside Main : 229 
[Tick:60] Inside Main : 230 
[Tick:60] Inside Main : 231 
[Tick:61] Inside Main : 232 
[Tick:61] Inside Main : 233 
[Tick:61] Inside Main : 234 
[Tick:61] Inside Main : 235 
[Tick:62] Inside Main : 236 
[Tick:62] Inside Main : 237 
[Tick:62] Inside Main : 238 
[Tick:62] Inside Main : 239 
[Tick:63] Inside Main : 240 
[Tick:63] Inside Main : 241 
[Tick:63] Inside Main : 242 
[Tick:64] Inside Main : 243 
[Tick:64] Inside Main : 244 
[Tick:64] Inside Main : 245 
[Tick:64] Inside Main : 246 
[Tick:65] Inside Main : 247 
[Tick:65] Inside Main : 248 
[Tick:65] Inside Main : 249 
[Tick:65] Inside Main : 250 
[Tick:66] Inside Main : 251 
[Tick:66] Inside Main : 252 
[Tick:66] Inside Main : 253 
[Tick:67] Inside Main : 254 
[Tick:67] Inside Main : 255 
[Tick:67] Inside Main : 256 
[Tick:67] Inside Main : 257 
[Tick:68] Inside Main : 258 
[Tick:68] Inside Main : 259 
[Tick:68] Inside Main : 260 
[Tick:68] Inside Main : 261 
[Tick:69] Inside Main : 262 
[Tick:69] Inside Main : 263 
[Tick:69] Inside Main : 264 
[Tick:69] Inside Main : 265 
[Tick:70] Inside Main : 266 
[Tick:70] Inside Main : 267 
[Tick:70] Inside Main : 268 
[Tick:71] Inside Main : 269
[Tick:71] Inside Main : 270 
[Tick:71] Inside Main : 271 
[Tick:71] Inside Main : 272 
[Tick:72] Inside Main : 273 
[Tick:72] Inside Main : 274 
[Tick:72] Inside Main : 275 
[Tick:72] Inside Main : 276 
[Tick:73] Inside Main : 277 
[Tick:73] Inside Main : 278 
[Tick:73] Inside Main : 279 
[Tick:73] Inside Main : 280 
[Tick:74] Inside Main : 281 
[Tick:74] Inside Main : 282 
[Tick:74] Inside Main : 283 
[Tick:75] Inside Main : 284 
[Tick:75] Inside Main : 285 
[Tick:75] Inside Main : 286 
[Tick:75] Inside Main : 287 
[Tick:76] Inside Main : 288 
[Tick:76] Inside Main : 289 
[Tick:76] Inside Main : 290 
[Tick:76] Inside Main : 291 
[Tick:77] Inside Main : 292 
[Tick:77] Inside Main : 293 
[Tick:77] Inside Main : 294 
[Tick:78] Inside Main : 295 
[Tick:78] Inside Main : 296 
[Tick:78] Inside Main : 297 
[Tick:78] Inside Main : 298 
[Tick:79] Inside Main : 299 
[Tick:79] Inside Main : 300 
[Tick:79] Inside Main : 301 
[Tick:79] Inside Main : 302 
[Tick:80] Inside Main : 303 
[Tick:80] Inside Main : 304 
[Tick:80] Inside Main : 305 
[Tick:80] Inside Main : 306 
[Tick:81] Inside Main : 307 
[Tick:81] Inside Main : 308 
[Tick:81] Inside Main : 309 
[Tick:82] Inside Main : 310 
[Tick:82] Inside Main : 311 
[Tick:82] Inside Main : 312 
[Tick:82] Inside Main : 313 
[Tick:83] Inside Main : 314 
[Tick:83] Inside Main : 315 
[Tick:83] Inside Main : 316 
[Tick:83] Inside Main : 317 
[Tick:84] Inside Main : 318 
[Tick:84] Inside Main : 319 
[Tick:84] Inside Main : 320 
[Tick:85] Inside Main : 321 
[Tick:85] Inside Main : 322 
[Tick:85] Inside Main : 323 
[Tick:85] Inside Main : 324 
[Tick:86] Inside Main : 325 
[Tick:86] Inside Main : 326 
[Tick:86] Inside Main : 327 
[Tick:86] Inside Main : 328 
[Tick:87] Inside Main : 329 
[Tick:87] Inside Main : 330
[Tick:87] Inside Main : 331 
[Tick:87] Inside Main : 332 
[Tick:88] Inside Main : 333 
[Tick:88] Inside Main : 334 
[Tick:88] Inside Main : 335 
[Tick:89] Inside Main : 336 
[Tick:89] Inside Main : 337 
[Tick:89] Inside Main : 338 
[Tick:89] Inside Main : 339 
[Tick:90] Inside Main : 340 
[Tick:90] Inside Main : 341
[Tick:90] Inside Main : 342 
[Tick:90] Inside Main : 343 
[Tick:91] Inside Main : 344 
[Tick:91] Inside Main : 345 
[Tick:91] Inside Main : 346 
[Tick:92] Inside Main : 347 
[Tick:92] Inside Main : 348 
[Tick:92] Inside Main : 349 
[Tick:92] Inside Main : 350 
[Tick:93] Inside Main : 351 
[Tick:93] Inside Main : 352 
[Tick:93] Inside Main : 353
[Tick:93] Inside Main : 354 
[Tick:94] Inside Main : 355 
[Tick:94] Inside Main : 356 
[Tick:94] Inside Main : 357 
[Tick:94] Inside Main : 358 
[Tick:95] Inside Main : 359 
[Tick:95] Inside Main : 360 
[Tick:95] Inside Main : 361 
[Tick:96] Inside Main : 362 
[Tick:96] Inside Main : 363 
[Tick:96] Inside Main : 364 
[Tick:96] Inside Main : 365 
[Tick:97] Inside Main : 366 
[Tick:97] Inside Main : 367 
[Tick:97] Inside Main : 368 
[Tick:97] Inside Main : 369 
[Tick:98] Inside Main : 370 
[Tick:98] Inside Main : 371 
[Tick:98] Inside Main : 372
[Tick:99] Inside Main : 373 
[Tick:99] Inside Main : 374 
[Tick:99] Inside Main : 375 
[Tick:99] Inside Main : 376 

[Tick:100] Inside My Task 

[Tick:100] Inside Main : 377 
[Tick:100] Inside Main : 378 
[Tick:100] Inside Main : 379 
[Tick:101] Inside Main : 380 
[Tick:101] Inside Main : 381 
[Tick:101] Inside Main : 382 
```
