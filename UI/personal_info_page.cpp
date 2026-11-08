#include "personal_info_page.hpp"

#include "hal/hal.h"
#include "storage_service.hpp"
#include <cstdio>
#include <string>

namespace {
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
uint8_t wrap(uint8_t value,uint8_t low,uint8_t high,int direction){
  if(direction>0)return value>=high?low:static_cast<uint8_t>(value+1u);
  return value<=low?high:static_cast<uint8_t>(value-1u);
}
void value_card(float x,float y,const char *label,uint8_t value,const char *unit,bool selected,bool editing){
  if(selected)HAL::drawRFrame(x,y,58,34,5);
  HAL::drawChinese(x+6,y+12,label);
  char text[8]{};std::snprintf(text,sizeof(text),"%u",value);
  HAL::drawEnglish(x+8,y+28,text);HAL::drawEnglish(x+38,y+28,unit);
  if(selected&&editing)HAL::drawHLine(x+7,y+31,44);
}
}

void PersonalInfoPage::enter(uint32_t now_ms){
  (void)now_ms;edit_=profile_service_snapshot();selected_=0;editing_=false;
}
void PersonalInfoPage::exit(uint32_t now_ms){(void)now_ms;editing_=false;}

void PersonalInfoPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(!editing_){
    if(event==BSP_KEY_UP||event==BSP_KEY_DOWN)selected_^=1u;
    else if(event==BSP_KEY_ENTER)editing_=true;
    return;
  }
  if(event==BSP_KEY_UP){
    if(selected_==0u)edit_.height_cm=wrap(edit_.height_cm,145u,219u,1);
    else edit_.weight_kg=wrap(edit_.weight_kg,30u,149u,1);
  }else if(event==BSP_KEY_DOWN){
    if(selected_==0u)edit_.height_cm=wrap(edit_.height_cm,145u,219u,-1);
    else edit_.weight_kg=wrap(edit_.weight_kg,30u,149u,-1);
  }else if(event==BSP_KEY_ENTER){
    (void)profile_service_set(edit_);editing_=false;
  }
}

void PersonalInfoPage::render(uint32_t now_ms){
  (void)now_ms;const StorageStatus storage=storage_service_status();
  HAL::canvasClear();HAL::setDrawType(1);centered(11,"个人信息");
  HAL::drawEnglish(105,11,storage.ready&&storage.error_count==0u?"MEM":"ERR");HAL::drawHDottedLine(0,14,128);
  value_card(3,18,"身高",edit_.height_cm,"cm",selected_==0u,editing_);
  value_card(67,18,"体重",edit_.weight_kg,"kg",selected_==1u,editing_);
  HAL::drawEnglish(2,63,editing_?"UP/DN VALUE":"UP/DN SELECT");
  HAL::drawEnglish(104,63,editing_?"SAVE":"OK");HAL::canvasUpdate();
}
