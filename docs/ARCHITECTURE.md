# Greenfield architecture

## Dependency direction

```text
Application -> UI -> Services -> Drivers -> BSP -> STM32 HAL/CMSIS
                   \-> immutable model snapshots
```

- **BSP** owns clocks, pins, buses, DMA, interrupts, watchdog, and power rails.
- **Drivers** own one physical device each and expose typed, non-blocking APIs.
- **Services** own time, health, motion, settings, alarms, storage, and power
  policy. They never draw pixels.
- **UI** owns Astra pages, navigation, animation, and view models. It never
  talks to a sensor or flash directly.
- **Application** creates static RTOS objects, wires queues, and starts tasks.

The old project is not a dependency. It was used once to produce
`HARDWARE.md` and the product feature list.

## FreeRTOS execution model

| Task | Priority | Period / trigger | Responsibility |
|---|---:|---|---|
| input | 3 | 5 ms active / key EXTI screen-off | debounce four keys, publish input events |
| motion acquisition | 4 | data-ready / 10 ms | read MPU6050 into a fixed sample queue |
| PPG acquisition | 4 | data-ready / FIFO | drain MAX30102 without UI blocking |
| health processing | 2 | queued samples | heart-rate and SpO2 algorithms |
| environment | 3 | 1 s | SHT20 state machine |
| power | 1 | 1 s after boot | filtered PA0 ADC battery voltage and charge estimate |
| time | 2 | 200 ms | RTC ownership, calendar snapshot and setting commands |
| alarm | 2 | 100 ms / 5 ms while ringing | two persistent daily alarms and non-blocking PB8 sound pattern |
| sound | 2 | task notification / 2 ms while tone active | arbitrate PB8 alarm output and non-blocking key tones |
| UI | 1 | 33 ms active / input or 1 s screen-off | consume events/snapshots and render Astra |
| storage | 1 | command queue | journaled settings and statistics writes |
| monitor | 1 | 1 s | inspect task stack margins and feed the independent watchdog |

All queues, tasks, and synchronization objects are statically allocated.
Sensor tasks publish bounded messages; the UI reads immutable snapshots. No
task waits in a polling loop and no flash operation executes in the UI task.

The monitor tracks every application task with
`uxTaskGetStackHighWaterMark()`. The minimum permitted reserve is 32 Cortex-M
words (128 bytes). It refreshes the STM32 independent watchdog only while all
registered stacks remain above that reserve. The watchdog has a conservative
nominal timeout of about eight seconds, so normal sensor retries and flash
journal compaction cannot cause a false reset. A scheduler lockup, disabled
interrupts, fault-handler loop, or unsafe stack margin stops refresh and lets
hardware recover the system. The RCC watchdog-reset flag is captured at boot;
the system-settings footer reports `WDG OK`, `WDG RST`, or `WDG ERR` together
with the live minimum stack reserve in words.

## Screen-off power policy

When the home/menu display timeout expires, the UI publishes one shared
screen-sleep state and sends the SSD1306 power-save command. While that state
is active:

- the UI task blocks on its input queue for up to one second instead of
  rendering at 30 FPS; a key still wakes it immediately;
- active key polling stops; a falling-edge EXTI wakes and notifies the input
  task, which then performs normal debounce processing;
- MAX30102 enters its hardware shutdown bit and its acquisition/processing
  pipelines pause, then the FIFO is reconfigured on wake;
- SHT20 sampling pauses and MPU6050 bus polling pauses;
- battery sampling changes from one second to 30 seconds, RTC snapshot refresh
  changes from 200 ms to one second, and idle alarm polling changes from
  100 ms to 500 ms;
- the sound task is notification-driven and blocks indefinitely while silent
  instead of waking every 2 ms.

Feature pages remain active and are never automatically put into this state.
The independent watchdog and stack monitor continue to run once per second.
This reduces OLED, I2C and task execution activity and creates long idle
windows in which tickless STOP2 can be entered.

## Tickless STOP2

FreeRTOS tickless idle is enabled with a 50 ms minimum expected idle interval.
Only the screen-off policy may enter STOP2; active UI and feature pages retain
the normal 1 kHz tick. The application overrides the weak Cortex-M33
`vPortSuppressTicksAndSleep()` implementation because SysTick cannot run in
STOP2. The low-power path:

1. confirms the kernel can still sleep and masks interrupts;
2. stops SysTick and arms the RTC wake-up counter from the expected idle time;
3. refreshes the independent watchdog and enters STM32U575 STOP2;
4. wakes on RTC or one of the four falling-edge key EXTI lines;
5. rebuilds the original MSI/PLL clock tree before unmasking interrupts;
6. measures elapsed time from RTC subseconds, advances the FreeRTOS and HAL
   ticks, restarts SysTick, and refreshes the watchdog again.

Each STOP2 interval is capped at four seconds so the firmware is safe whether
the board option bytes freeze or continue IWDG in Stop mode; normal background
deadlines currently produce shorter intervals. Key EXTI interrupts run at
priority 6 and notify the input task with the FreeRTOS ISR API. The system
settings footer alternates its watchdog report with `STOP2 count seconds`,
providing an on-device proof that deep sleep and resume have executed.

## Persistent storage

The firmware image is restricted to the 1 MiB first flash bank.  The final two
8 KiB pages of bank 2 (`0x081fc000` and `0x081fe000`) are reserved for a
128-bit record journal.  Records contain a monotonic sequence number and CRC32.
The storage task appends updates at priority 1 and compacts between the two
pages with a complete checkpoint before erasing stale data, so a reset during
program or erase leaves a recoverable copy.  System settings, personal profile,
sound/display settings, alarm configuration and Dino high score use this service.

## Delivery slices

1. Boot, FreeRTOS, OLED, four-key navigation, fault capture.
2. Bus manager and real sensor drivers with device-ID/self-test screens.
3. RTC/time, alarms, stopwatch, settings journal, battery and power service.
4. Motion and PPG pipelines, then real step/heart-rate/SpO2 algorithms.
5. Remaining product pages, Dino game, sleep/wake and full endurance testing.

Each slice must build in Debug and Release and pass an on-device acceptance
check before the next slice is merged.
