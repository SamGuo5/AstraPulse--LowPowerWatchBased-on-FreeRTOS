#ifndef WATCH_ASTRA_FREERTOS_CONFIG_H
#define WATCH_ASTRA_FREERTOS_CONFIG_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
extern uint32_t SystemCoreClock;
void app_panic(const char *reason);
#ifdef __cplusplus
}
#endif
#define configCPU_CLOCK_HZ SystemCoreClock
#define configTICK_RATE_HZ 1000
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configMAX_PRIORITIES 6
#define configMINIMAL_STACK_SIZE 256
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configQUEUE_REGISTRY_SIZE 0
#define configUSE_TIMERS 0
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define configSUPPORT_STATIC_ALLOCATION 1
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 0
#define configUSE_IDLE_HOOK 1
#define configUSE_TICK_HOOK 0
#define configUSE_TICKLESS_IDLE 1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP 50
#define configENABLE_FPU 1
#define configENABLE_MPU 0
#define configENABLE_TRUSTZONE 0
#define configRUN_FREERTOS_SECURE_ONLY 0
#define configENABLE_PAC 0
#define configENABLE_BTI 0
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (15 << 4)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (5 << 4)
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define configASSERT(x) do { if(!(x)) app_panic("FreeRTOS assert"); } while(0)
#endif
