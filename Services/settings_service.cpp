#include "settings_service.hpp"

#include "FreeRTOS.h"
#include "bsp.h"
#include "storage_service.hpp"
#include "task.h"

namespace {
constexpr uint32_t signature=0x53000000u;
SystemSettings settings{false,true,true,40,120};

bool valid(const SystemSettings &value){
  return value.heart_low>=35u&&value.heart_low<=80u&&
         value.heart_high>=90u&&value.heart_high<=200u&&
         (!value.heart_low_enabled||!value.heart_high_enabled||value.heart_low<value.heart_high);
}

uint32_t encode(const SystemSettings &value){
  return signature|(value.status_led?1u:0u)|(value.heart_low_enabled?2u:0u)|
         (value.heart_high_enabled?4u:0u)|((uint32_t)value.heart_low<<3u)|
         ((uint32_t)value.heart_high<<10u);
}

bool decode(uint32_t word,SystemSettings &value){
  if((word&0xff000000u)!=signature)return false;
  value.status_led=(word&1u)!=0u;
  value.heart_low_enabled=(word&2u)!=0u;
  value.heart_high_enabled=(word&4u)!=0u;
  value.heart_low=static_cast<uint8_t>((word>>3u)&0x7fu);
  value.heart_high=static_cast<uint8_t>((word>>10u)&0xffu);
  return valid(value);
}

void save(const SystemSettings &value){
  const uint32_t word=encode(value);
  HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR3,word);
  HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR4,~word);
}
}

void settings_service_start(){
  SystemSettings loaded{};uint32_t stored=0;
  if(storage_service_read(StorageKey::SystemSettings,stored)&&decode(stored,loaded))settings=loaded;
  else{
    const uint32_t backup=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR3);
    const uint32_t check=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR4);
    if((backup^check)==0xffffffffu&&decode(backup,loaded))settings=loaded;
    (void)storage_service_write(StorageKey::SystemSettings,encode(settings));
  }
  save(settings);
  bsp_led_set(settings.status_led?1u:0u);
}

SystemSettings settings_service_snapshot(){
  taskENTER_CRITICAL();const SystemSettings copy=settings;taskEXIT_CRITICAL();return copy;
}

bool settings_service_set(SystemSettings value){
  if(!valid(value))return false;
  taskENTER_CRITICAL();settings=value;taskEXIT_CRITICAL();
  bsp_led_set(value.status_led?1u:0u);save(value);
  return storage_service_write(StorageKey::SystemSettings,encode(value));
}
