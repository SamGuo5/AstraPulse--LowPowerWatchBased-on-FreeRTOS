#include "sound_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "storage_service.hpp"
#include "task.h"
#include "system_monitor_service.hpp"

namespace {
constexpr uint32_t signature=0x41000000u;
constexpr uint32_t click_length_ms=40u;
SoundSettings settings{true,true};
bool click_requested{};
bool alarm_output{};
StaticTask_t task_control;
StackType_t task_stack[256];
TaskHandle_t task_handle{};

uint32_t encode(const SoundSettings &value){
  return signature|(value.master_enabled?1u:0u)|(value.key_tone_enabled?2u:0u);
}
bool decode(uint32_t word,SoundSettings &value){
  if((word&0xffffff00u)!=signature)return false;
  value.master_enabled=(word&1u)!=0u;value.key_tone_enabled=(word&2u)!=0u;return true;
}

void sound_task(void *){
  TickType_t click_until=0;bool click_phase=false;
  for(;;){
    taskENTER_CRITICAL();
    const SoundSettings current=settings;const bool click=click_requested;
    click_requested=false;const bool alarm=alarm_output;
    taskEXIT_CRITICAL();
    const TickType_t now=xTaskGetTickCount();
    if(click)click_until=now+pdMS_TO_TICKS(click_length_ms);
    const bool clicking=current.key_tone_enabled&&(int32_t)(click_until-now)>0;
    if(clicking)click_phase=!click_phase;else click_phase=false;
    bsp_buzzer_set(current.master_enabled&&(alarm||(clicking&&click_phase))?1u:0u);
    /* Sleep indefinitely when silent. Notifications make settings, alarm
       transitions and key clicks immediate without a permanent 500 Hz task. */
    (void)ulTaskNotifyTake(pdTRUE,clicking?pdMS_TO_TICKS(2):portMAX_DELAY);
  }
}
}

void sound_service_start(){
  uint32_t word=0;SoundSettings loaded{};
  if(storage_service_read(StorageKey::SoundSettings,word)&&decode(word,loaded))settings=loaded;
  else (void)storage_service_write(StorageKey::SoundSettings,encode(settings));
  bsp_buzzer_set(0);
  task_handle=xTaskCreateStatic(sound_task,"sound",256,nullptr,2,task_stack,&task_control);
  configASSERT(task_handle);system_monitor_register(task_handle);
}
SoundSettings sound_service_snapshot(){
  taskENTER_CRITICAL();const SoundSettings copy=settings;taskEXIT_CRITICAL();return copy;
}
bool sound_service_set(SoundSettings value){
  taskENTER_CRITICAL();settings=value;taskEXIT_CRITICAL();
  if(task_handle!=nullptr)xTaskNotifyGive(task_handle);
  return storage_service_write(StorageKey::SoundSettings,encode(value));
}
void sound_service_key_click(){
  taskENTER_CRITICAL();click_requested=true;taskEXIT_CRITICAL();
  if(task_handle!=nullptr)xTaskNotifyGive(task_handle);
}
void sound_service_alarm_output(bool enabled){
  taskENTER_CRITICAL();const bool changed=alarm_output!=enabled;alarm_output=enabled;taskEXIT_CRITICAL();
  if(changed&&task_handle!=nullptr)xTaskNotifyGive(task_handle);
}
