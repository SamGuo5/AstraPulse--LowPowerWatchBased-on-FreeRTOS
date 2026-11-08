#include "climate_service.hpp"

#include "FreeRTOS.h"
#include "sht20.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"
#include <algorithm>

namespace {
ClimateSnapshot snapshot{};
StaticTask_t task_control;
StackType_t task_stack[512];

void publish(bool valid,int16_t temperature,uint16_t humidity){
  taskENTER_CRITICAL();
  if(valid){
    snapshot.valid=true;
    snapshot.temperature_tenths=temperature;
    snapshot.humidity_tenths=humidity;
    ++snapshot.sample_count;
  }else if(snapshot.error_count<0xffffu)++snapshot.error_count;
  taskEXIT_CRITICAL();
}

void climate_task(void *){
  if(sht20_soft_reset())vTaskDelay(pdMS_TO_TICKS(20));
  TickType_t wake=xTaskGetTickCount();
  for(;;){
    if(power_policy_is_screen_sleeping()){
      vTaskDelay(pdMS_TO_TICKS(1000));wake=xTaskGetTickCount();continue;
    }
    uint16_t raw_temperature=0,raw_humidity=0;
    bool ok=sht20_start_temperature();
    if(ok){vTaskDelay(pdMS_TO_TICKS(100));ok=sht20_read_raw(&raw_temperature);}
    if(ok)ok=sht20_start_humidity();
    if(ok){vTaskDelay(pdMS_TO_TICKS(40));ok=sht20_read_raw(&raw_humidity);}
    if(ok){
      const int32_t temperature=-4685+(17572*(int32_t)raw_temperature)/65536;
      const int32_t humidity=-600+(12500*(int32_t)raw_humidity)/65536;
      const int32_t humidity_clamped=std::max<int32_t>(0,std::min<int32_t>(10000,humidity));
      publish(true,static_cast<int16_t>((temperature+(temperature>=0?5:-5))/10),
              static_cast<uint16_t>((humidity_clamped+5)/10));
    }else publish(false,0,0);
    xTaskDelayUntil(&wake,pdMS_TO_TICKS(1000));
  }
}
}

void climate_service_start(){
  /* Sensor acquisition must not be starved by full-frame OLED transfers. */
  TaskHandle_t handle=xTaskCreateStatic(climate_task,"climate",512,nullptr,3,task_stack,&task_control);
  configASSERT(handle);system_monitor_register(handle);
}

ClimateSnapshot climate_service_snapshot(){
  taskENTER_CRITICAL();
  const ClimateSnapshot copy=snapshot;
  taskEXIT_CRITICAL();
  return copy;
}
