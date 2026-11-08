#include "watch_face.hpp"
#include "hal/hal.h"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
constexpr uint8_t bold_digits[10][5]={
  {0x07,0x05,0x05,0x05,0x07},
  {0x02,0x06,0x02,0x02,0x07},
  {0x07,0x01,0x07,0x04,0x07},
  {0x07,0x01,0x07,0x01,0x07},
  {0x05,0x05,0x07,0x01,0x01},
  {0x07,0x04,0x07,0x01,0x07},
  {0x07,0x04,0x07,0x05,0x07},
  {0x07,0x01,0x02,0x02,0x02},
  {0x07,0x05,0x07,0x05,0x07},
  {0x07,0x05,0x07,0x01,0x07}
};
constexpr uint8_t heart_icon[8]={0x00,0x66,0xff,0xff,0x7e,0x3c,0x18,0x00};
constexpr uint8_t oxygen_icon[8]={0x70,0x88,0x88,0x88,0x76,0x02,0x04,0x06};
constexpr uint8_t temperature_icon[8]={0x18,0x24,0x24,0x24,0x24,0x5a,0x7e,0x3c};
constexpr uint8_t humidity_icon[8]={0x10,0x18,0x3c,0x3c,0x7e,0x7e,0x3c,0x00};

void hline(float x,float y){HAL::drawBox(x+2,y,8,2);}
void vline(float x,float y){HAL::drawBox(x,y,2,8);}
void digit(float x,float y,uint8_t value){
  const uint8_t mask=segments[value%10];
  if(mask&0x01)hline(x,y);
  if(mask&0x02)vline(x+10,y+2);
  if(mask&0x04)vline(x+10,y+12);
  if(mask&0x08)hline(x,y+20);
  if(mask&0x10)vline(x,y+12);
  if(mask&0x20)vline(x,y+2);
  if(mask&0x40)hline(x,y+10);
}

void draw_time(uint8_t hour,uint8_t minute,uint8_t second){
  constexpr float x=33,y=18;
  digit(x,y,hour/10);digit(x+14,y,hour%10);
  HAL::drawBox(x+29,y+6,2,2);HAL::drawBox(x+29,y+14,2,2);
  digit(x+34,y,minute/10);digit(x+48,y,minute%10);
  if((second&1u)==0){
    HAL::drawPixel(x+29,y+5);HAL::drawPixel(x+29,y+13);
  }
}

void draw_icon(float x,float y,const uint8_t (&rows)[8]){
  for(uint8_t row=0;row<8;++row){
    for(uint8_t column=0;column<8;++column){
      if((rows[row]&(0x80u>>column))!=0u)HAL::drawPixel(x+column,y+row);
    }
  }
}

void draw_bold_digit(float x,float y,uint8_t value){
  const auto &rows=bold_digits[value%10u];
  for(uint8_t row=0;row<5;++row){
    for(uint8_t column=0;column<3;++column){
      if((rows[row]&(0x04u>>column))!=0u)HAL::drawBox(x+column*2,y+row*2,2,2);
    }
  }
}

void draw_value(float cell_x,float y,int16_t value){
  const bool negative=value<0;
  uint16_t magnitude=static_cast<uint16_t>(negative?-value:value);
  magnitude=std::min<uint16_t>(magnitude,999u);
  const uint8_t digits=magnitude>=100u?3u:(magnitude>=10u?2u:1u);
  const uint8_t glyphs=digits+(negative?1u:0u);
  const float width=glyphs*6.0f+(glyphs-1u);
  float x=cell_x+10.0f+(22.0f-width)/2.0f;
  if(negative){HAL::drawBox(x,y+4,6,2);x+=7;}
  if(digits==3u){draw_bold_digit(x,y,magnitude/100u);x+=7;}
  if(digits>=2u){draw_bold_digit(x,y,(magnitude/10u)%10u);x+=7;}
  draw_bold_digit(x,y,magnitude%10u);
}
void draw_missing(float cell_x,float y){
  HAL::drawBox(cell_x+14,y+4,6,2);HAL::drawBox(cell_x+22,y+4,6,2);
}

void draw_sensor_strip(const HomeSnapshot &snapshot){
  constexpr float icon_top=50;
  constexpr float value_top=49;
  draw_icon(1,icon_top,heart_icon);
  if(snapshot.heart_valid)draw_value(0,value_top,snapshot.heart_rate);
  else draw_missing(0,value_top);

  draw_icon(33,icon_top,oxygen_icon);
  if(snapshot.spo2_valid)draw_value(32,value_top,snapshot.spo2_percent);
  else draw_missing(32,value_top);

  draw_icon(65,icon_top,temperature_icon);
  if(snapshot.climate_valid)draw_value(64,value_top,snapshot.temperature_c);
  else draw_missing(64,value_top);

  draw_icon(97,icon_top,humidity_icon);
  if(snapshot.climate_valid)draw_value(96,value_top,snapshot.humidity_percent);
  else draw_missing(96,value_top);

  for(float x: {31.0f,63.0f,95.0f}){
    HAL::drawPixel(x,50);HAL::drawPixel(x,54);HAL::drawPixel(x,58);
  }
}
void draw_alarm_status(){
  HAL::drawRFrame(68,3,8,6,3);HAL::drawHLine(67,9,10);
  HAL::drawPixel(71,2);HAL::drawPixel(72,2);HAL::drawPixel(71,10);
}
}

void WatchFace::render(const HomeSnapshot &snapshot) const {
  HAL::canvasClear();
  HAL::setDrawType(1);

  /* Top status strip is supplied entirely by immutable service snapshots. */
  char date_text[16]{};
  char battery_text[8]{};
  std::snprintf(date_text,sizeof(date_text),"%04u/%02u/%02u",snapshot.year,snapshot.month,snapshot.day);
  if(snapshot.battery_valid)std::snprintf(battery_text,sizeof(battery_text),"%u%%",snapshot.battery_percent);
  else std::snprintf(battery_text,sizeof(battery_text),"--%%");
  HAL::drawEnglish(0,11,date_text);
  if(snapshot.alarm_enabled)draw_alarm_status();
  /* Status text is ASCII.  Keep it on u8g2's ASCII path so an absent battery
     cannot enter the custom Chinese glyph renderer during the boot handoff. */
  HAL::drawEnglish(80,11,battery_text);
  HAL::drawFrame(104,2,19,9);HAL::drawBox(123,4,2,5);
  const float fill=snapshot.battery_valid?15.0f*snapshot.battery_percent/100.0f:0.0f;
  if(fill>=1)HAL::drawBox(106,4,fill,5);
  if(snapshot.battery_low){HAL::drawPixel(104,12);HAL::drawPixel(108,12);HAL::drawPixel(112,12);}
  HAL::drawHDottedLine(0,14,128);

  draw_time(snapshot.hour,snapshot.minute,snapshot.second);
  HAL::drawHDottedLine(0,44,128);
  draw_sensor_strip(snapshot);
  HAL::canvasUpdate();
}
