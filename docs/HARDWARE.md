# Hardware contract

This document is the only hardware contract inherited from the old project. No
old application, UI, scheduler, or algorithm source belongs to this firmware.

| Function | Device / pin | Interface |
|---|---|---|
| MCU | STM32U575RIT6 | Cortex-M33, 2 MiB internal flash |
| Display | SSD1306-compatible 128 x 64, address `0x3c` | I2C2: PB13 SCL, PB14 SDA |
| Pulse oximeter | MAX30102, address `0x57` | I2C3: PC0 SCL, PC1 SDA |
| Motion | MPU6050, address `0x68` (`0x69` also probed) | I2C4: PB10 SCL/AF3, PB7 SDA/AF5 |
| Temperature / humidity | SHT20, address `0x40` | I2C1: PB6 SCL, PB3 SDA |
| Calendar clock | STM32U575 internal RTC | LSI source, backup register guards initialization |
| Battery | PA0 / ADC1_IN5 | 12-bit ADC, 2:1 external divider |
| Up / down | PC12 / PA4 | active-low GPIO |
| Enter / back | PA5 / PC11 | active-low GPIO |
| Status LED | PA8 | GPIO output |
| Buzzer | PB8 | GPIO software tone output |
| Peripheral power | PB12 | GPIO output, high enables power |
| Debug | PA13 SWDIO, PA14 SWCLK | SWD |
| Console | PA9 TX, PA10 RX | USART1 |

Every driver must use this table through the BSP. UI and services must not
access STM32 HAL handles or GPIO registers directly.
