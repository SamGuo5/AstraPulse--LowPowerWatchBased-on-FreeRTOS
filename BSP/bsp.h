#ifndef WATCH_ASTRA_BSP_H
#define WATCH_ASTRA_BSP_H
#include "stm32u5xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {BSP_KEY_NONE=0,BSP_KEY_UP,BSP_KEY_DOWN,BSP_KEY_ENTER,BSP_KEY_BACK} BspKey;
typedef struct {
    uint8_t initialized;
    uint8_t scl_high;
    uint8_t sda_high;
    uint8_t enable_high;
    uint32_t hal_error;
} BspI2c4Diagnostics;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;
extern I2C_HandleTypeDef hi2c4;
extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc1;
void bsp_init(void);
BspKey bsp_key_raw(void);
void bsp_led_set(uint8_t on);
void bsp_buzzer_set(uint8_t on);
bool bsp_battery_adc_read(uint16_t *raw);
bool bsp_i2c4_ready(void);
bool bsp_i2c4_recover(void);
BspI2c4Diagnostics bsp_i2c4_diagnostics(void);
bool bsp_watchdog_start(void);
void bsp_watchdog_refresh(void);
bool bsp_was_watchdog_reset(void);
uint32_t bsp_stop2_sleep(uint32_t requested_ms);
void bsp_key_wakeup_callback(void);
void Error_Handler(void);
#ifdef __cplusplus
}
#endif
#endif
