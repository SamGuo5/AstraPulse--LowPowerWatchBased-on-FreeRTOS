#include "system_settings_page.hpp"

#include "hal/hal.h"
#include "settings_service.hpp"
#include "system_monitor_service.hpp"
#include "power_policy_service.hpp"
#include <cstdio>
#include <string>

namespace {
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}

void row(uint8_t index,uint8_t selected,float y,const char *label,const char *value){
  if(index==selected)HAL::drawRFrame(2,y,124,13,4);
  HAL::drawChinese(9,y+10,label);
  std::string measured=value;
  HAL::drawEnglish(119.0f-HAL::getFontWidth(measured),y+10,value);
}
}

void SystemSettingsPage::enter(uint32_t now_ms){(void)now_ms;selected_=0;}

void SystemSettingsPage::change_selected(){
  SystemSettings value=settings_service_snapshot();
  if(selected_==0u)value.status_led=!value.status_led;
  else if(selected_==1u){
    if(!value.heart_low_enabled){value.heart_low_enabled=true;value.heart_low=40;}
    else if(value.heart_low<70u)value.heart_low=static_cast<uint8_t>(value.heart_low+5u);
    else value.heart_low_enabled=false;
  }else{
    if(!value.heart_high_enabled){value.heart_high_enabled=true;value.heart_high=120;}
    else if(value.heart_high>95u)value.heart_high=static_cast<uint8_t>(value.heart_high-5u);
    else value.heart_high_enabled=false;
  }
  (void)settings_service_set(value);
}

void SystemSettingsPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(event==BSP_KEY_UP)selected_=selected_==0u?2u:static_cast<uint8_t>(selected_-1u);
  else if(event==BSP_KEY_DOWN)selected_=selected_>=2u?0u:static_cast<uint8_t>(selected_+1u);
  else if(event==BSP_KEY_ENTER)change_selected();
}

void SystemSettingsPage::render(uint32_t now_ms){
  const SystemSettings value=settings_service_snapshot();
  const SystemMonitorSnapshot monitor=system_monitor_snapshot();
  const PowerPolicySnapshot power=power_policy_snapshot();
  char low[8]{},high[8]{},status[32]{};
  if(value.heart_low_enabled)std::snprintf(low,sizeof(low),"%u",value.heart_low);
  else std::snprintf(low,sizeof(low),"OFF");
  if(value.heart_high_enabled)std::snprintf(high,sizeof(high),"%u",value.heart_high);
  else std::snprintf(high,sizeof(high),"OFF");

  HAL::canvasClear();HAL::setDrawType(1);centered(11,"系统设置");
  HAL::drawHDottedLine(0,14,128);
  row(0,selected_,16,"指示灯",value.status_led?"ON":"OFF");
  row(1,selected_,29,"心率 LOW",low);
  row(2,selected_,42,"心率 HIGH",high);
  if(((now_ms/2000u)&1u)!=0u){
    if(power.deep_sleep_count==0u)std::snprintf(status,sizeof(status),"STOP2 WAIT");
    else std::snprintf(status,sizeof(status),"STOP2 %lu %lus",(unsigned long)power.deep_sleep_count,
                       (unsigned long)(power.deep_sleep_ms/1000u));
  }else if(!monitor.watchdog_active)std::snprintf(status,sizeof(status),"WDG WAIT");
  else if(!monitor.stack_ok)std::snprintf(status,sizeof(status),"WDG ERR S:%u",monitor.minimum_stack_words);
  else if(monitor.recovered_from_watchdog)std::snprintf(status,sizeof(status),"WDG RST S:%u",monitor.minimum_stack_words);
  else std::snprintf(status,sizeof(status),"WDG OK S:%u",monitor.minimum_stack_words);
  HAL::drawEnglish(2,63,status);
  HAL::canvasUpdate();
}
