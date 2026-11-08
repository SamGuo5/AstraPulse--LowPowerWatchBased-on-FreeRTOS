#include "alarm_page.hpp"

#include "hal/hal.h"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
void centered(float baseline,const std::string &text){
  std::string measured=text;HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
void hline(float x,float y){HAL::drawBox(x+2,y,8,2);}
void vline(float x,float y){HAL::drawBox(x,y,2,8);}
void digit(float x,float y,uint8_t value){
  const uint8_t mask=segments[value%10u];
  if(mask&0x01u)hline(x,y);
  if(mask&0x02u)vline(x+10,y+2);
  if(mask&0x04u)vline(x+10,y+12);
  if(mask&0x08u)hline(x,y+20);
  if(mask&0x10u)vline(x,y+12);
  if(mask&0x20u)vline(x,y+2);
  if(mask&0x40u)hline(x,y+10);
}
void bell(float x,float y){
  HAL::drawRFrame(x+2,y+2,10,9,4);HAL::drawBox(x+1,y+10,12,2);
  HAL::drawBox(x+6,y,2,2);HAL::drawBox(x+6,y+12,2,2);
}
void switch_control(float x,float y,bool enabled){
  HAL::drawRFrame(x,y,24,10,4);
  if(enabled){HAL::drawRBox(x,y,24,10,4);HAL::setDrawType(0);HAL::drawRBox(x+15,y+2,7,6,3);HAL::setDrawType(1);}
  else HAL::drawRBox(x+2,y+2,7,6,3);
}
uint8_t wrap(uint8_t value,uint8_t maximum,int direction){
  if(direction>0)return value>=maximum?0u:static_cast<uint8_t>(value+1u);
  return value==0u?maximum:static_cast<uint8_t>(value-1u);
}
void large_time(uint8_t hour,uint8_t minute){
  constexpr float x=32,y=19;
  digit(x,y,hour/10u);digit(x+14,y,hour%10u);
  HAL::drawBox(x+29,y+6,2,2);HAL::drawBox(x+29,y+14,2,2);
  digit(x+34,y,minute/10u);digit(x+48,y,minute%10u);
}
}

void AlarmPage::enter(uint32_t now_ms){(void)now_ms;editing_=false;}
void AlarmPage::exit(uint32_t now_ms){(void)now_ms;editing_=false;}

void AlarmPage::adjust(int direction){
  if(field_==Field::Hour)edit_.hour=wrap(edit_.hour,23,direction);
  else if(field_==Field::Minute)edit_.minute=wrap(edit_.minute,59,direction);
  else edit_.enabled=!edit_.enabled;
}

void AlarmPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(!editing_){
    if(event==BSP_KEY_UP)selected_=0;
    else if(event==BSP_KEY_DOWN)selected_=1;
    else if(event==BSP_KEY_ENTER){edit_=alarm_service_snapshot().alarms[selected_];field_=Field::Hour;editing_=true;}
    return;
  }
  if(event==BSP_KEY_UP)adjust(1);
  else if(event==BSP_KEY_DOWN)adjust(-1);
  else if(event==BSP_KEY_ENTER){
    if(field_!=Field::Enabled)field_=static_cast<Field>(static_cast<uint8_t>(field_)+1u);
    else{(void)alarm_service_set(selected_,edit_);editing_=false;}
  }
}

void AlarmPage::render(uint32_t now_ms){
  (void)now_ms;const AlarmSnapshot snapshot=alarm_service_snapshot();
  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,editing_?(selected_==0?"闹钟一":"闹钟二"):"闹钟设置");
  if(editing_)HAL::drawEnglish(103,11,"EDIT");
  HAL::drawHDottedLine(0,14,128);
  if(editing_){
    large_time(edit_.hour,edit_.minute);switch_control(99,26,edit_.enabled);
    if(field_==Field::Hour)HAL::drawHLine(32,43,26);
    else if(field_==Field::Minute)HAL::drawHLine(66,43,26);
    else HAL::drawHLine(99,38,24);
    HAL::drawEnglish(1,62,"UP/DN");
    HAL::drawEnglish(field_==Field::Hour?49:(field_==Field::Minute?43:46),62,
                     field_==Field::Hour?"HOUR":(field_==Field::Minute?"MINUTE":"STATE"));
    HAL::drawEnglish(102,62,"OK>");HAL::canvasUpdate();return;
  }
  for(uint8_t i=0;i<2u;++i){
    const float y=17.0f+i*19.0f;
    if(selected_==i)HAL::drawRFrame(2,y,124,17,4);
    bell(7,y+2);HAL::drawEnglish(24,y+12,i==0?"A1":"A2");
    char time[8]{};std::snprintf(time,sizeof(time),"%02u:%02u",snapshot.alarms[i].hour,snapshot.alarms[i].minute);
    HAL::drawEnglish(43,y+12,time);switch_control(99,y+3,snapshot.alarms[i].enabled);
  }
  HAL::drawEnglish(2,62,"UP/DN SELECT");HAL::drawEnglish(101,62,"OK");HAL::canvasUpdate();
}

void AlarmPage::render_ringing(uint32_t now_ms,const AlarmSnapshot &snapshot) const{
  const uint8_t index=snapshot.active_index%2u;const AlarmConfig &alarm=snapshot.alarms[index];
  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,index==0?"闹钟一":"闹钟二");HAL::drawHDottedLine(0,14,128);
  const float shake=((now_ms/100u)&1u)?-2.0f:2.0f;bell(56+shake,18);
  char time[8]{};std::snprintf(time,sizeof(time),"%02u:%02u",alarm.hour,alarm.minute);
  HAL::drawEnglish(49,46,time);centered(62,"按键停止");HAL::canvasUpdate();
}
