#include "sound_settings_page.hpp"

#include "hal/hal.h"
#include "sound_service.hpp"
#include <string>

namespace {
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
void switch_control(float x,float y,bool enabled){
  HAL::drawRFrame(x,y,28,12,5);
  if(enabled){
    HAL::drawRBox(x,y,28,12,5);HAL::setDrawType(0);HAL::drawRBox(x+18,y+2,8,8,4);HAL::setDrawType(1);
  }else HAL::drawRBox(x+2,y+2,8,8,4);
}
void row(uint8_t index,uint8_t selected,float y,const char *label,bool enabled){
  if(index==selected)HAL::drawRFrame(2,y,124,19,5);
  HAL::drawChinese(10,y+13,label);switch_control(94,y+3,enabled);
}
}

void SoundSettingsPage::enter(uint32_t now_ms){(void)now_ms;selected_=0;}
void SoundSettingsPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(event==BSP_KEY_UP||event==BSP_KEY_DOWN)selected_^=1u;
  else if(event==BSP_KEY_ENTER){
    SoundSettings value=sound_service_snapshot();
    if(selected_==0u)value.master_enabled=!value.master_enabled;
    else value.key_tone_enabled=!value.key_tone_enabled;
    (void)sound_service_set(value);
  }
}
void SoundSettingsPage::render(uint32_t now_ms){
  (void)now_ms;const SoundSettings value=sound_service_snapshot();
  HAL::canvasClear();HAL::setDrawType(1);centered(11,"声音设置");HAL::drawHDottedLine(0,14,128);
  row(0,selected_,18,"总声音",value.master_enabled);
  row(1,selected_,38,"按键音",value.key_tone_enabled);
  HAL::drawEnglish(2,63,"UP/DN SELECT");HAL::drawEnglish(104,63,"OK");HAL::canvasUpdate();
}
