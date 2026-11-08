#include "display_settings_service.hpp"

#include "FreeRTOS.h"
#include "storage_service.hpp"
#include "task.h"

namespace {
constexpr uint32_t signature=0x44000000u;
constexpr uint16_t timeouts[]={0u,15u,30u,60u,120u};
DisplaySettings settings{6u,2u};

bool valid(const DisplaySettings &value){
  return value.brightness_level>=1u&&value.brightness_level<=8u&&value.timeout_index<5u;
}
uint32_t encode(const DisplaySettings &value){
  return signature|value.brightness_level|((uint32_t)value.timeout_index<<8u);
}
bool decode(uint32_t word,DisplaySettings &value){
  if((word&0xffff0000u)!=signature)return false;
  value.brightness_level=static_cast<uint8_t>(word&0xffu);
  value.timeout_index=static_cast<uint8_t>((word>>8u)&0xffu);return valid(value);
}
}

void display_settings_service_start(){
  uint32_t word=0;DisplaySettings loaded{};
  if(storage_service_read(StorageKey::DisplaySettings,word)&&decode(word,loaded))settings=loaded;
  else (void)storage_service_write(StorageKey::DisplaySettings,encode(settings));
}
DisplaySettings display_settings_service_snapshot(){
  taskENTER_CRITICAL();const DisplaySettings copy=settings;taskEXIT_CRITICAL();return copy;
}
bool display_settings_service_set(DisplaySettings value){
  if(!valid(value))return false;
  taskENTER_CRITICAL();settings=value;taskEXIT_CRITICAL();
  return storage_service_write(StorageKey::DisplaySettings,encode(value));
}
uint8_t display_settings_contrast(DisplaySettings value){
  if(!valid(value))value=settings;
  return static_cast<uint8_t>(32u+(value.brightness_level-1u)*31u);
}
uint16_t display_settings_timeout_seconds(DisplaySettings value){
  if(!valid(value))value=settings;
  return timeouts[value.timeout_index];
}
