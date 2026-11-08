#include "power_policy_service.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace {
PowerPolicySnapshot state{};
}

void power_policy_set_screen_sleeping(bool sleeping){
  taskENTER_CRITICAL();
  if(sleeping&&!state.screen_sleeping)++state.sleep_count;
  state.screen_sleeping=sleeping;
  taskEXIT_CRITICAL();
}

bool power_policy_is_screen_sleeping(){
  taskENTER_CRITICAL();const bool sleeping=state.screen_sleeping;taskEXIT_CRITICAL();return sleeping;
}

PowerPolicySnapshot power_policy_snapshot(){
  taskENTER_CRITICAL();const PowerPolicySnapshot copy=state;taskEXIT_CRITICAL();return copy;
}

void power_policy_record_deep_sleep(uint32_t elapsed_ms){
  if(elapsed_ms==0u)return;
  taskENTER_CRITICAL();
  ++state.deep_sleep_count;
  if(UINT32_MAX-state.deep_sleep_ms<elapsed_ms)state.deep_sleep_ms=UINT32_MAX;
  else state.deep_sleep_ms+=elapsed_ms;
  taskEXIT_CRITICAL();
}
