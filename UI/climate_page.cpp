#include "climate_page.hpp"

#include "climate_service.hpp"
#include "hal/hal.h"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
void hline(float x,float y){HAL::drawBox(x+1,y,6,2);}
void vline(float x,float y){HAL::drawBox(x,y,2,5);}
void digit(float x,float y,uint8_t value){
  const uint8_t mask=segments[value%10u];
  if(mask&0x01u)hline(x,y);
  if(mask&0x02u)vline(x+8,y+2);
  if(mask&0x04u)vline(x+8,y+9);
  if(mask&0x08u)hline(x,y+14);
  if(mask&0x10u)vline(x,y+9);
  if(mask&0x20u)vline(x,y+2);
  if(mask&0x40u)hline(x,y+7);
}
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
void value(float x,float y,int16_t tenths){
  const bool negative=tenths<0;
  const uint16_t magnitude=static_cast<uint16_t>(negative?-tenths:tenths);
  const uint16_t whole=magnitude/10u;
  if(negative)HAL::drawBox(x-6,y+7,5,2);
  float fraction_x=x+23;
  if(whole>=100u){
    x-=4;digit(x,y,static_cast<uint8_t>((whole/100u)%10u));
    digit(x+11,y,static_cast<uint8_t>((whole/10u)%10u));
    digit(x+22,y,static_cast<uint8_t>(whole%10u));fraction_x=x+34;
  }else{
    digit(x,y,static_cast<uint8_t>(whole/10u));digit(x+11,y,static_cast<uint8_t>(whole%10u));
  }
  char fraction[5]{};std::snprintf(fraction,sizeof(fraction),".%u",magnitude%10u);
  HAL::drawEnglish(fraction_x,y+15,fraction);
}
}

void ClimatePage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;
  if(event==BSP_KEY_ENTER)fahrenheit_=!fahrenheit_;
}

void ClimatePage::render(uint32_t now_ms){
  (void)now_ms;
  const ClimateSnapshot snapshot=climate_service_snapshot();
  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,"温湿度");
  HAL::drawEnglish(105,11,fahrenheit_?"F":"C");
  HAL::drawHDottedLine(0,14,128);
  if(!snapshot.valid){
    centered(37,"SHT20 OFFLINE");
    centered(55,"CHECK PB3 / PB6");
    HAL::canvasUpdate();return;
  }

  HAL::drawRFrame(1,17,62,33,5);HAL::drawRFrame(65,17,62,33,5);
  HAL::drawEnglish(6,27,"TEMP");HAL::drawEnglish(70,27,"RH");
  const int16_t temperature=fahrenheit_
      ?static_cast<int16_t>((snapshot.temperature_tenths*9)/5+320)
      :snapshot.temperature_tenths;
  value(9,30,temperature);HAL::drawEnglish(51,45,fahrenheit_?"F":"C");
  value(73,30,static_cast<int16_t>(snapshot.humidity_tenths));HAL::drawEnglish(116,45,"%");

  const bool comfortable=snapshot.temperature_tenths>=180 && snapshot.temperature_tenths<=280 &&
                         snapshot.humidity_tenths>=400 && snapshot.humidity_tenths<=700;
  const char *status=comfortable?"COMFORT":
      (snapshot.temperature_tenths<180?"COOL":snapshot.temperature_tenths>280?"WARM":
       snapshot.humidity_tenths<400?"DRY":"HUMID");
  centered(62,status);
  HAL::canvasUpdate();
}
