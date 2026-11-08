#include "power_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"
#include <algorithm>

namespace {
constexpr uint16_t adc_full_scale=4095;
constexpr uint16_t analog_supply_mv=3300;
constexpr uint8_t divider_gain=2;
constexpr uint8_t sample_total=16;

struct CurvePoint {uint16_t mv;uint8_t percent;};
constexpr CurvePoint discharge_curve[]={
  {3300,0},{3500,5},{3600,10},{3700,20},{3750,35},{3800,50},
  {3850,60},{3900,70},{4000,82},{4100,92},{4200,100}
};

PowerSnapshot snapshot{};
StaticTask_t task_control;
StackType_t task_stack[384];
uint16_t filtered_mv=0;

uint8_t voltage_to_percent(uint16_t mv){
  if(mv<=discharge_curve[0].mv)return discharge_curve[0].percent;
  for(size_t i=1;i<sizeof(discharge_curve)/sizeof(discharge_curve[0]);++i){
    if(mv<=discharge_curve[i].mv){
      const auto &low=discharge_curve[i-1];const auto &high=discharge_curve[i];
      const uint32_t scaled=(uint32_t)(mv-low.mv)*(high.percent-low.percent);
      return static_cast<uint8_t>(low.percent+scaled/(high.mv-low.mv));
    }
  }
  return 100;
}

bool read_trimmed_mv(uint16_t &mv){
  uint16_t samples[sample_total]{};uint8_t count=0;
  for(uint8_t i=0;i<sample_total;++i){
    uint16_t raw=0;
    if(bsp_battery_adc_read(&raw))samples[count++]=raw;
  }
  if(count<8)return false;
  std::sort(samples,samples+count);
  const uint8_t trim=count>=12?2:1;uint32_t sum=0;
  for(uint8_t i=trim;i<count-trim;++i)sum+=samples[i];
  const uint16_t raw=static_cast<uint16_t>(sum/(count-2u*trim));
  mv=static_cast<uint16_t>(((uint32_t)raw*analog_supply_mv*divider_gain+adc_full_scale/2u)/adc_full_scale);
  return mv>=2500u && mv<=5000u;
}

void publish_error(){
  taskENTER_CRITICAL();
  if(snapshot.error_count<0xffffu)++snapshot.error_count;
  taskEXIT_CRITICAL();
}

void publish(uint16_t measured_mv){
  if(filtered_mv==0)filtered_mv=measured_mv;
  else filtered_mv=static_cast<uint16_t>((7u*filtered_mv+measured_mv+4u)/8u);
  const uint8_t calculated=voltage_to_percent(filtered_mv);
  taskENTER_CRITICAL();
  if(!snapshot.valid || calculated+1u<snapshot.battery_percent || calculated>snapshot.battery_percent+1u)
    snapshot.battery_percent=calculated;
  snapshot.battery_mv=filtered_mv;snapshot.valid=true;snapshot.low=snapshot.battery_percent<=15u;
  ++snapshot.sample_count;
  taskEXIT_CRITICAL();
}

void power_task(void *){
  /* The battery is optional.  Establish the display and home screen first,
     then probe PA0 entirely in the background. */
  vTaskDelay(pdMS_TO_TICKS(2000));
  for(;;){
    uint16_t measured_mv=0;
    if(read_trimmed_mv(measured_mv))publish(measured_mv);else publish_error();
    vTaskDelay(pdMS_TO_TICKS(power_policy_is_screen_sleeping()?30000u:1000u));
  }
}
}

void power_service_start(){
  TaskHandle_t handle=xTaskCreateStatic(power_task,"power",384,nullptr,1,task_stack,&task_control);
  configASSERT(handle);system_monitor_register(handle);
}

PowerSnapshot power_service_snapshot(){
  taskENTER_CRITICAL();const PowerSnapshot copy=snapshot;taskEXIT_CRITICAL();return copy;
}
