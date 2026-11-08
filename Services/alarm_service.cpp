#include "alarm_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"
#include "time_service.hpp"
#include "storage_service.hpp"
#include "sound_service.hpp"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"

namespace {
constexpr uint32_t backup_signature=0xa14a0000u;
constexpr uint32_t backup_signature_mask=0xffff0000u;
constexpr uint32_t enabled_mask=1u<<15;
constexpr uint32_t ring_timeout_ms=60000u;

AlarmSnapshot snapshot{{{7,30,false},{18,0,false}},false,0};
bool save_pending{};
bool dismiss_pending{};
StaticTask_t task_control;
StackType_t task_stack[512];
uint32_t last_fired[2]{};

bool valid(const AlarmConfig &config){return config.hour<24u&&config.minute<60u;}
uint32_t encode(const AlarmConfig &config){
  return backup_signature|(config.enabled?enabled_mask:0u)|((uint32_t)config.hour<<8)|config.minute;
}
bool decode(uint32_t value,AlarmConfig &config){
  if((value&backup_signature_mask)!=backup_signature)return false;
  config.hour=static_cast<uint8_t>((value>>8)&0x1fu);
  config.minute=static_cast<uint8_t>(value&0x3fu);
  config.enabled=(value&enabled_mask)!=0u;
  return valid(config);
}
uint32_t minute_key(const TimeSnapshot &time){
  uint32_t key=time.year-2000u;
  key=key*13u+time.month;key=key*32u+time.day;key=key*24u+time.hour;
  return key*60u+time.minute;
}
void load(){
  AlarmConfig first{},second{};uint32_t word=0;
  if(storage_service_read(StorageKey::AlarmOne,word)&&decode(word,first))snapshot.alarms[0]=first;
  else if(decode(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1),first))snapshot.alarms[0]=first;
  if(storage_service_read(StorageKey::AlarmTwo,word)&&decode(word,second))snapshot.alarms[1]=second;
  else if(decode(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR2),second))snapshot.alarms[1]=second;
}
void save(const AlarmSnapshot &copy){
  const uint32_t first=encode(copy.alarms[0]),second=encode(copy.alarms[1]);
  HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,first);HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR2,second);
  (void)storage_service_write(StorageKey::AlarmOne,first);
  (void)storage_service_write(StorageKey::AlarmTwo,second);
}
void stop_ringing(){
  sound_service_alarm_output(false);
  taskENTER_CRITICAL();snapshot.ringing=false;taskEXIT_CRITICAL();
}
void start_ringing(uint8_t index){
  taskENTER_CRITICAL();snapshot.ringing=true;snapshot.active_index=index;taskEXIT_CRITICAL();
}

void alarm_task(void *){
  bool buzzer_high=false;TickType_t ring_started=0;
  for(;;){
    taskENTER_CRITICAL();
    const bool dismiss=dismiss_pending;dismiss_pending=false;
    const bool save_now=save_pending;save_pending=false;
    AlarmSnapshot copy=snapshot;
    taskEXIT_CRITICAL();
    if(save_now)save(copy);
    if(dismiss&&copy.ringing){stop_ringing();copy.ringing=false;buzzer_high=false;}

    const TimeSnapshot time=time_service_snapshot();
    if(!copy.ringing&&time.valid){
      const uint32_t key=minute_key(time);
      uint8_t trigger=2u;
      for(uint8_t i=0;i<2u;++i){
        const AlarmConfig &alarm=copy.alarms[i];
        if(alarm.enabled&&alarm.hour==time.hour&&alarm.minute==time.minute&&last_fired[i]!=key){
          last_fired[i]=key;if(trigger>=2u)trigger=i;
        }
      }
      if(trigger<2u){
        start_ringing(trigger);copy.ringing=true;copy.active_index=trigger;
        ring_started=xTaskGetTickCount();
      }
    }

    if(copy.ringing){
      const uint32_t elapsed=pdTICKS_TO_MS(xTaskGetTickCount()-ring_started);
      if(elapsed>=ring_timeout_ms){stop_ringing();buzzer_high=false;}
      else{
        const uint32_t phase=elapsed%1000u;
        const bool sound=(phase<180u)||(phase>=260u&&phase<440u);
        if(sound){buzzer_high=!buzzer_high;sound_service_alarm_output(buzzer_high);}
        else{buzzer_high=false;sound_service_alarm_output(false);}
      }
      vTaskDelay(pdMS_TO_TICKS(5));
    }else{
      sound_service_alarm_output(false);buzzer_high=false;
      vTaskDelay(pdMS_TO_TICKS(power_policy_is_screen_sleeping()?500u:100u));
    }
  }
}
}

void alarm_service_start(){
  load();save(snapshot);sound_service_alarm_output(false);
  TaskHandle_t handle=xTaskCreateStatic(alarm_task,"alarm",512,nullptr,2,task_stack,&task_control);
  configASSERT(handle);system_monitor_register(handle);
}
AlarmSnapshot alarm_service_snapshot(){
  taskENTER_CRITICAL();const AlarmSnapshot copy=snapshot;taskEXIT_CRITICAL();return copy;
}
bool alarm_service_set(uint8_t index,AlarmConfig config){
  if(index>=2u||!valid(config))return false;
  taskENTER_CRITICAL();snapshot.alarms[index]=config;save_pending=true;taskEXIT_CRITICAL();return true;
}
void alarm_service_dismiss(){taskENTER_CRITICAL();dismiss_pending=true;taskEXIT_CRITICAL();}
