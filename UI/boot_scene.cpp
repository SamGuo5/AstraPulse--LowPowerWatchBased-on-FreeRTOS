#include "boot_scene.hpp"
#include "home_view_model.hpp"
#include "hal/hal.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr uint32_t duration_ms=1500;
constexpr uint8_t segments[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};

float clamp01(float value){return std::max(0.0f,std::min(1.0f,value));}
float phase(uint32_t elapsed,uint32_t start,uint32_t duration){
  if(elapsed<=start)return 0.0f;
  return clamp01(static_cast<float>(elapsed-start)/static_cast<float>(duration));
}
float ease(float value){
  const float t=clamp01(value);
  return t*t*(3.0f-2.0f*t);
}
float mix(float from,float to,float amount){return from+(to-from)*amount;}

void draw_eye(float center_x,float center_y,float width,float height,
              float pupil_x=0.0f,float pupil_y=0.0f,bool pupil=false){
  const float radius=std::max(1.0f,std::min(width,height)*0.28f);
  HAL::setDrawType(1);
  HAL::drawRBox(center_x-width/2.0f,center_y-height/2.0f,width,height,radius);
  if(pupil && width>=14.0f && height>=10.0f){
    HAL::setDrawType(0);
    HAL::drawRBox(center_x-2.0f+pupil_x,center_y-3.0f+pupil_y,4,6,2);
    HAL::setDrawType(1);
  }
}

void draw_sleep_mark(){
  HAL::drawHLine(98,13,6);HAL::drawPixel(103,14);HAL::drawPixel(102,15);
  HAL::drawPixel(101,16);HAL::drawHLine(98,17,6);
  HAL::drawHLine(108,7,4);HAL::drawPixel(111,8);HAL::drawPixel(110,9);
  HAL::drawHLine(108,10,4);
}

void draw_smile(float amount){
  if(amount<=0.0f)return;
  const float width=24.0f*amount;
  const float left=64.0f-width/2.0f;
  HAL::drawHLine(left,51,width);
  if(amount>0.45f){HAL::drawPixel(left-1,50);HAL::drawPixel(left+width,50);}
}

void hline(float x,float y){HAL::drawBox(x+2,y,8,2);}
void vline(float x,float y){HAL::drawBox(x,y,2,8);}
void draw_digit(float x,float y,uint8_t value){
  const uint8_t mask=segments[value%10u];
  if(mask&0x01u)hline(x,y);
  if(mask&0x02u)vline(x+10,y+2);
  if(mask&0x04u)vline(x+10,y+12);
  if(mask&0x08u)hline(x,y+20);
  if(mask&0x10u)vline(x,y+12);
  if(mask&0x20u)vline(x,y+2);
  if(mask&0x40u)hline(x,y+10);
}

void draw_clock_reveal(const HomeSnapshot &snapshot,float amount){
  const float t=ease(amount);
  constexpr float y=18;
  draw_digit(mix(-28,33,t),y,snapshot.hour/10u);
  draw_digit(mix(-14,47,t),y,snapshot.hour%10u);
  draw_digit(mix(128,67,t),y,snapshot.minute/10u);
  draw_digit(mix(142,81,t),y,snapshot.minute%10u);
  HAL::drawBox(62,24,2,2);HAL::drawBox(62,32,2,2);

  const float divider=128.0f*t;
  HAL::drawHDottedLine(64.0f-divider/2.0f,14,divider);
  HAL::drawHDottedLine(64.0f-divider/2.0f,44,divider);
}
}

bool BootScene::render(uint32_t now_ms,const HomeSnapshot &snapshot) const {
  const uint32_t elapsed=now_ms-started_ms_;
  if(elapsed>=duration_ms)return true;

  HAL::canvasClear();
  HAL::setDrawType(1);

  if(elapsed<150u){
    draw_eye(40,31,30,3);draw_eye(88,31,30,3);
    draw_sleep_mark();
  }else if(elapsed<520u){
    const float opening=ease(phase(elapsed,150,370));
    draw_eye(40,31,mix(30,34,opening),mix(3,25,opening),0,0,opening>0.68f);
    draw_eye(88,31,mix(30,34,opening),mix(3,25,opening),0,0,opening>0.68f);
  }else if(elapsed<760u){
    const float looking=phase(elapsed,520,240);
    float gaze=0.0f;
    if(looking<0.30f)gaze=mix(0,-6,ease(looking/0.30f));
    else if(looking<0.68f)gaze=mix(-6,6,ease((looking-0.30f)/0.38f));
    else gaze=mix(6,0,ease((looking-0.68f)/0.32f));
    draw_eye(40,31,34,25,gaze,0,true);
    draw_eye(88,31,34,25,gaze,0,true);
  }else if(elapsed<980u){
    const float wink=phase(elapsed,760,220);
    const float close=1.0f-std::abs(2.0f*wink-1.0f);
    draw_eye(40,31,34,mix(25,3,ease(close)),0,0,close<0.55f);
    draw_eye(88,31,34,25,0,0,true);
    draw_smile(ease(1.0f-close*0.35f));
  }else if(elapsed<1240u){
    const float morph=ease(phase(elapsed,980,260));
    draw_eye(mix(40,63,morph),mix(31,25,morph),mix(34,3,morph),mix(25,3,morph));
    draw_eye(mix(88,63,morph),mix(31,33,morph),mix(34,3,morph),mix(25,3,morph));
    draw_smile(1.0f-morph);
  }else{
    draw_clock_reveal(snapshot,phase(elapsed,1240,260));
  }

  HAL::canvasUpdate();
  return false;
}
