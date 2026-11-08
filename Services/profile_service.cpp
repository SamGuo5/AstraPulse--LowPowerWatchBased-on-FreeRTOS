#include "profile_service.hpp"

#include "FreeRTOS.h"
#include "storage_service.hpp"
#include "task.h"

namespace {
constexpr uint32_t signature=0x50000000u;
PersonalProfile profile{175u,70u};

bool valid(const PersonalProfile &value){
  return value.height_cm>=145u&&value.height_cm<=219u&&
         value.weight_kg>=30u&&value.weight_kg<=149u;
}
uint32_t encode(const PersonalProfile &value){
  return signature|value.height_cm|((uint32_t)value.weight_kg<<8u);
}
bool decode(uint32_t word,PersonalProfile &value){
  if((word&0xff000000u)!=signature)return false;
  value.height_cm=static_cast<uint8_t>(word&0xffu);
  value.weight_kg=static_cast<uint8_t>((word>>8u)&0xffu);
  return valid(value);
}
}

void profile_service_start(){
  uint32_t word=0;PersonalProfile loaded{};
  if(storage_service_read(StorageKey::PersonalProfile,word)&&decode(word,loaded))profile=loaded;
}
PersonalProfile profile_service_snapshot(){
  taskENTER_CRITICAL();const PersonalProfile copy=profile;taskEXIT_CRITICAL();return copy;
}
bool profile_service_set(PersonalProfile value){
  if(!valid(value))return false;
  taskENTER_CRITICAL();profile=value;taskEXIT_CRITICAL();
  return storage_service_write(StorageKey::PersonalProfile,encode(value));
}
