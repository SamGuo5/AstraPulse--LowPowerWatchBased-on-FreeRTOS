#include "display_settings_page.hpp"

#include "display_settings_service.hpp"
#include "hal/hal.h"
#include <cstdio>
#include <string>

namespace {
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
void sun(float x,float y,uint8_t level){
  HAL::drawRFrame(x+5,y+5,10,10,5);HAL::drawBox(x+8,y+8,4,4);
  HAL::drawHLine(x,y+9,4);HAL::drawHLine(x+16,y+9,4);
  HAL::drawVLine(x+9,y,4);HAL::drawVLine(x+9,y+16,4);
  const float width=20.0f*level/8.0f;HAL::drawFrame(x,y+23,20,5);
  if(width>=2.0f)HAL::drawBox(x+2,y+25,width-2.0f,1);
}
void row(uint8_t index,uint8_t selected,float y,const char *label,const char *value){
  if(index==selected)HAL::drawRFrame(30,y,96,18,5);
  HAL::drawChinese(38,y+12,label);
  std::string measured=value;HAL::drawEnglish(118.0f-HAL::getFontWidth(measured),y+12,value);
}
}

void DisplaySettingsPage::enter(uint32_t now_ms){(void)now_ms;selected_=0;}
void DisplaySettingsPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(event==BSP_KEY_UP||event==BSP_KEY_DOWN)selected_^=1u;
  else if(event==BSP_KEY_ENTER){
    DisplaySettings value=display_settings_service_snapshot();
    if(selected_==0u){
      value.brightness_level=value.brightness_level>=8u?1u:static_cast<uint8_t>(value.brightness_level+1u);
      HAL::setScreenBrightness(display_settings_contrast(value));
    }else value.timeout_index=value.timeout_index>=4u?0u:static_cast<uint8_t>(value.timeout_index+1u);
    (void)display_settings_service_set(value);
  }
}
void DisplaySettingsPage::render(uint32_t now_ms){
  (void)now_ms;const DisplaySettings value=display_settings_service_snapshot();
  char level[8]{},timeout[8]{};std::snprintf(level,sizeof(level),"L%u",value.brightness_level);
  const uint16_t seconds=display_settings_timeout_seconds(value);
  if(seconds==0u)std::snprintf(timeout,sizeof(timeout),"OFF");
  else std::snprintf(timeout,sizeof(timeout),"%uS",seconds);
  HAL::canvasClear();HAL::setDrawType(1);centered(11,"屏幕亮度");HAL::drawHDottedLine(0,14,128);
  sun(5,21,value.brightness_level);row(0,selected_,18,"亮度",level);row(1,selected_,38,"休眠",timeout);
  HAL::drawEnglish(2,63,"UP/DN SELECT");HAL::drawEnglish(104,63,"OK");HAL::canvasUpdate();
}
