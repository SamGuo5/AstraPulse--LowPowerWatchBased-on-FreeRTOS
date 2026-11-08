#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
extern void rtos_tick_handler(void);
void SysTick_Handler(void) {
    HAL_IncTick();
    if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)rtos_tick_handler();
}
void HAL_Delay(uint32_t ms) {
    if(ms==0)return;
    if(xTaskGetSchedulerState()==taskSCHEDULER_RUNNING && __get_IPSR()==0){vTaskDelay(pdMS_TO_TICKS(ms));return;}
    uint32_t start=HAL_GetTick();while((uint32_t)(HAL_GetTick()-start)<ms)__NOP();
}
