#include "date_time_page.hpp"

#include "hal/hal.h"
#include "home_view_model.hpp"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
constexpr const char *weekdays[7]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
constexpr const char *field_names[5]={"YEAR","MONTH","DAY","HOUR","MINUTE"};

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

void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
bool leap(uint16_t year){return year%4u==0u&&(year%100u!=0u||year%400u==0u);}
uint8_t days_in_month(uint16_t year,uint8_t month){
  static constexpr uint8_t days[]={31,28,31,30,31,30,31,31,30,31,30,31};
  return month==2u&&leap(year)?29u:days[month-1u];
}
uint16_t wrap(uint16_t value,uint16_t low,uint16_t high,int direction){
  if(direction>0)return value>=high?low:static_cast<uint16_t>(value+1u);
  return value<=low?high:static_cast<uint16_t>(value-1u);
}
void underline(float baseline,const std::string &text,size_t start,size_t length){
  std::string measured=text,before=text.substr(0,start),selected=text.substr(start,length);
  const float left=(128.0f-HAL::getFontWidth(measured))/2.0f;
  HAL::drawHLine(left+HAL::getFontWidth(before),baseline+2,HAL::getFontWidth(selected));
}
}

void DateTimePage::enter(uint32_t now_ms){(void)now_ms;editing_=false;}
void DateTimePage::exit(uint32_t now_ms){(void)now_ms;editing_=false;}

void DateTimePage::begin_edit(){
  if(snapshot_==nullptr)return;
  edit_={snapshot_->year,snapshot_->month,snapshot_->day,snapshot_->weekday,
         snapshot_->hour,snapshot_->minute,snapshot_->second,true};
  field_=Field::Year;editing_=true;
}

void DateTimePage::adjust(int direction){
  switch(field_){
    case Field::Year:edit_.year=wrap(edit_.year,2000,2099,direction);break;
    case Field::Month:edit_.month=static_cast<uint8_t>(wrap(edit_.month,1,12,direction));break;
    case Field::Day:edit_.day=static_cast<uint8_t>(wrap(edit_.day,1,days_in_month(edit_.year,edit_.month),direction));break;
    case Field::Hour:edit_.hour=static_cast<uint8_t>(wrap(edit_.hour,0,23,direction));break;
    case Field::Minute:edit_.minute=static_cast<uint8_t>(wrap(edit_.minute,0,59,direction));break;
  }
  const uint8_t maximum=days_in_month(edit_.year,edit_.month);
  if(edit_.day>maximum)edit_.day=maximum;
}

void DateTimePage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(!editing_){if(event==BSP_KEY_ENTER)begin_edit();return;}
  if(event==BSP_KEY_UP)adjust(1);
  else if(event==BSP_KEY_DOWN)adjust(-1);
  else if(event==BSP_KEY_ENTER){
    if(field_!=Field::Minute)field_=static_cast<Field>(static_cast<uint8_t>(field_)+1u);
    else{edit_.second=0;(void)time_service_set(edit_);editing_=false;}
  }
}

void DateTimePage::render(uint32_t now_ms){
  (void)now_ms;
  if(snapshot_==nullptr)return;
  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,"日期时间");
  if(editing_)HAL::drawEnglish(103,11,"EDIT");
  HAL::drawHDottedLine(0,14,128);

  if(editing_){
    char date[16]{},time[10]{};
    std::snprintf(date,sizeof(date),"%04u-%02u-%02u",edit_.year,edit_.month,edit_.day);
    std::snprintf(time,sizeof(time),"%02u:%02u",edit_.hour,edit_.minute);
    const std::string date_text=date,time_text=time;
    centered(30,date_text);centered(47,time_text);
    switch(field_){
      case Field::Year:underline(30,date_text,0,4);break;
      case Field::Month:underline(30,date_text,5,2);break;
      case Field::Day:underline(30,date_text,8,2);break;
      case Field::Hour:underline(47,time_text,0,2);break;
      case Field::Minute:underline(47,time_text,3,2);break;
    }
    HAL::drawEnglish(1,62,"UP/DN");
    centered(62,field_names[static_cast<uint8_t>(field_)]);
    HAL::drawEnglish(102,62,"OK>");HAL::canvasUpdate();return;
  }

  uint8_t hour=snapshot_->hour;
  constexpr float x=25,y=18;
  digit(x,y,hour/10u);digit(x+14,y,hour%10u);
  HAL::drawBox(x+29,y+6,2,2);HAL::drawBox(x+29,y+14,2,2);
  digit(x+34,y,snapshot_->minute/10u);digit(x+48,y,snapshot_->minute%10u);

  char side[8]{};
  std::snprintf(side,sizeof(side),"%02uS",snapshot_->second);
  HAL::drawEnglish(101,29,side);
  HAL::drawEnglish(105,40,"RTC");

  HAL::drawHDottedLine(0,44,128);
  char date[24]{};
  std::snprintf(date,sizeof(date),"%04u-%02u-%02u %s",snapshot_->year,
                snapshot_->month,snapshot_->day,weekdays[snapshot_->weekday%7u]);
  centered(59,date);
  HAL::canvasUpdate();
}
