#include "system_monitor_service.hpp"

#include "bsp.h"
#include <limits>

namespace {
constexpr uint8_t task_capacity=16u;
/* FreeRTOS reports words, so this is 128 bytes on Cortex-M33. */
constexpr UBaseType_t safe_stack_words=32u;
TaskHandle_t tasks[task_capacity]{};
uint8_t task_count{};
SystemMonitorSnapshot snapshot{0u,0u,true,false,false};
StaticTask_t task_control;
StackType_t task_stack[384];

void monitor_task(void *){
  taskENTER_CRITICAL();
  snapshot.recovered_from_watchdog=bsp_was_watchdog_reset();
  taskEXIT_CRITICAL();

  const bool watchdog_active=bsp_watchdog_start();
  TickType_t wake=xTaskGetTickCount();
  for(;;){
    TaskHandle_t local[task_capacity]{};
    uint8_t count=0u;
    taskENTER_CRITICAL();
    count=task_count;
    for(uint8_t i=0u;i<count;++i)local[i]=tasks[i];
    taskEXIT_CRITICAL();

    UBaseType_t minimum=std::numeric_limits<UBaseType_t>::max();
    bool stack_ok=count>0u;
    for(uint8_t i=0u;i<count;++i){
      const UBaseType_t remaining=uxTaskGetStackHighWaterMark(local[i]);
      if(remaining<minimum)minimum=remaining;
      if(remaining<safe_stack_words)stack_ok=false;
    }
    if(minimum==std::numeric_limits<UBaseType_t>::max())minimum=0u;
    const uint16_t minimum_words=minimum>0xffffu?0xffffu:static_cast<uint16_t>(minimum);

    taskENTER_CRITICAL();
    snapshot.minimum_stack_words=minimum_words;
    snapshot.registered_tasks=count;
    snapshot.stack_ok=stack_ok;
    snapshot.watchdog_active=watchdog_active;
    taskEXIT_CRITICAL();

    /* Feeding is deliberately conditional. A scheduler/interrupt deadlock
       cannot run this lowest-priority task, while a dangerous stack margin
       intentionally turns into a controlled reset instead of corruption. */
    if(watchdog_active&&stack_ok)bsp_watchdog_refresh();
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(1000));
  }
}
}

void system_monitor_register(TaskHandle_t handle){
  if(handle==nullptr)return;
  taskENTER_CRITICAL();
  bool exists=false;
  for(uint8_t i=0u;i<task_count;++i)if(tasks[i]==handle)exists=true;
  if(!exists&&task_count<task_capacity)tasks[task_count++]=handle;
  taskEXIT_CRITICAL();
}

void system_monitor_start(){
  TaskHandle_t handle=xTaskCreateStatic(monitor_task,"monitor",384,nullptr,1,task_stack,&task_control);
  configASSERT(handle);
  system_monitor_register(handle);
}

SystemMonitorSnapshot system_monitor_snapshot(){
  taskENTER_CRITICAL();
  const SystemMonitorSnapshot copy=snapshot;
  taskEXIT_CRITICAL();
  return copy;
}
