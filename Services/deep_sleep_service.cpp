#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "power_policy_service.hpp"

#include <algorithm>

extern "C" void vPortSuppressTicksAndSleep(TickType_t expected_idle_ticks){
  if(!power_policy_is_screen_sleeping()){
    __DSB();__WFI();__ISB();return;
  }

  /* The scheduler is already suspended by the idle task. PRIMASK keeps a key
     or RTC interrupt pending while clocks and the kernel tick are restored. */
  __disable_irq();__DSB();__ISB();
  if(eTaskConfirmSleepModeStatus()==eAbortSleep){__enable_irq();return;}

  const uint32_t requested_ms=std::min<uint32_t>(pdTICKS_TO_MS(expected_idle_ticks),4000u);
  if(requested_ms<50u){__enable_irq();return;}

  SysTick->CTRL=0u;
  SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk;
  const uint32_t measured_ms=bsp_stop2_sleep(requested_ms);
  const TickType_t measured_ticks=pdMS_TO_TICKS(measured_ms);
  const TickType_t accounted_ticks=std::min<TickType_t>(measured_ticks,expected_idle_ticks);

  /* clock_init() inside bsp_stop2_sleep() also reconfigures SysTick through
     HAL_InitTick. Set it explicitly to the FreeRTOS 1 kHz core-clock source. */
  SysTick->LOAD=(SystemCoreClock/configTICK_RATE_HZ)-1u;
  SysTick->VAL=0u;
  SCB->ICSR=SCB_ICSR_PENDSTCLR_Msk;
  SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk|SysTick_CTRL_TICKINT_Msk|SysTick_CTRL_ENABLE_Msk;
  if(accounted_ticks>0u){
    vTaskStepTick(accounted_ticks);
    uwTick+=pdTICKS_TO_MS(accounted_ticks);
  }

  __enable_irq();__DSB();__ISB();
  power_policy_record_deep_sleep(pdTICKS_TO_MS(accounted_ticks));
}
