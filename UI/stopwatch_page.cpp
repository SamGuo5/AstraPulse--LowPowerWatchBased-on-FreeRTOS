#include "stopwatch_page.hpp"

#include "hal/hal.h"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
void hline(float x,float y){HAL::drawBox(x+1,y,6,2);}
void vline(float x,float y){HAL::drawBox(x,y,2,6);}
void digit(float x,float y,uint8_t value){
  const uint8_t mask=segments[value%10u];
  if(mask&0x01u)hline(x,y);
  if(mask&0x02u)vline(x+8,y+2);
  if(mask&0x04u)vline(x+8,y+10);
  if(mask&0x08u)hline(x,y+16);
  if(mask&0x10u)vline(x,y+10);
  if(mask&0x20u)vline(x,y+2);
  if(mask&0x40u)hline(x,y+8);
}
void centered(float baseline,const std::string &text){
  std::string measured=text;
  HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
}

uint32_t StopwatchPage::elapsed(uint32_t now_ms) const {
  return accumulated_ms_+(running_?now_ms-running_since_ms_:0u);
}

void StopwatchPage::handle(BspKey event,uint32_t now_ms){
  if(event==BSP_KEY_ENTER){
    if(running_){accumulated_ms_+=now_ms-running_since_ms_;running_=false;}
    else{running_since_ms_=now_ms;running_=true;}
  }else if(event==BSP_KEY_DOWN){
    running_=false;accumulated_ms_=0;running_since_ms_=now_ms;
  }
}

void StopwatchPage::render(uint32_t now_ms){
  const uint32_t total_ms=elapsed(now_ms);
  const uint32_t total_centiseconds=total_ms/10u;
  const uint8_t minutes=static_cast<uint8_t>((total_ms/60000u)%100u);
  const uint8_t seconds=static_cast<uint8_t>((total_ms/1000u)%60u);
  const uint8_t centiseconds=static_cast<uint8_t>(total_centiseconds%100u);

  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,"秒表");
  HAL::drawEnglish(101,11,running_?"RUN":"STOP");
  HAL::drawHDottedLine(0,14,128);

  constexpr float x=20,y=20;
  digit(x,y,minutes/10u);digit(x+12,y,minutes%10u);
  HAL::drawBox(x+25,y+5,2,2);HAL::drawBox(x+25,y+12,2,2);
  digit(x+30,y,seconds/10u);digit(x+42,y,seconds%10u);
  char fraction[8]{};std::snprintf(fraction,sizeof(fraction),".%02u",centiseconds);
  HAL::drawEnglish(87,38,fraction);

  HAL::drawFrame(3,42,122,5);
  const float progress=118.0f*static_cast<float>(total_centiseconds%100u)/100.0f;
  if(progress>=1.0f)HAL::drawBox(5,44,progress,1);

  HAL::drawEnglish(3,61,"OK");
  HAL::drawChinese(18,61,running_?"暂停":"开始");
  HAL::drawVLine(75,52,8);HAL::drawHLine(72,57,7);
  HAL::drawChinese(86,61,"复位");
  HAL::canvasUpdate();
}
