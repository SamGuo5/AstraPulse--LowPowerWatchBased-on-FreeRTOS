#include "dino_page.hpp"

#include "hal/hal.h"
#include "storage_service.hpp"
#include <algorithm>
#include <cstdio>

namespace {
constexpr int32_t q8=256;
constexpr int16_t ground_y=54;
constexpr int16_t dino_x=12;
constexpr int16_t dino_width=18;
constexpr int16_t dino_height=16;
constexpr int16_t resting_y=ground_y-dino_height;

constexpr int16_t dino_stride=(dino_width+7)/8;
using DinoSprite=const uint8_t[dino_height*dino_stride];

constexpr DinoSprite dino_run_a={
  0x00,0x7e,0x00, 0x00,0xff,0x01, 0x00,0xfb,0x01,
  0x00,0xff,0x01, 0x00,0xdf,0x01, 0x00,0x3f,0x00,
  0x84,0x3f,0x00, 0xce,0x3f,0x00, 0xff,0x1f,0x00,
  0xfe,0xdf,0x00, 0xfc,0x67,0x00, 0xf8,0x03,0x00,
  0xb8,0x03,0x00, 0x18,0x03,0x00, 0x1c,0x07,0x00,
  0x3c,0x0f,0x00
};

constexpr DinoSprite dino_run_b={
  0x00,0x7e,0x00, 0x00,0xff,0x01, 0x00,0xfb,0x01,
  0x00,0xff,0x01, 0x00,0xdf,0x01, 0x00,0x3f,0x00,
  0x84,0x3f,0x00, 0xce,0x3f,0x00, 0xff,0x1f,0x00,
  0xfe,0xdf,0x00, 0xfc,0x67,0x00, 0xf8,0x03,0x00,
  0xb8,0x03,0x00, 0x38,0x03,0x00, 0x18,0x06,0x00,
  0x3c,0x3c,0x00
};

constexpr DinoSprite dino_jump={
  0x00,0x7e,0x00, 0x00,0xff,0x01, 0x00,0xfb,0x01,
  0x00,0xff,0x01, 0x00,0xdf,0x01, 0x00,0x3f,0x00,
  0x84,0x3f,0x00, 0xce,0x3f,0x00, 0xff,0x1f,0x00,
  0xfe,0xdf,0x00, 0xfc,0x67,0x00, 0xf8,0x03,0x00,
  0xf8,0x01,0x00, 0xb8,0x03,0x00, 0xb0,0x01,0x00,
  0xf0,0x01,0x00
};

void score_text(uint32_t score,uint32_t high){
  char text[24]{};
  std::snprintf(text,sizeof(text),"%05lu  HI %05lu",(unsigned long)score,(unsigned long)high);
  HAL::drawEnglish(3,10,text);
}
void cloud(int16_t x,int16_t y){
  HAL::drawHLine(x+3,y,8);HAL::drawHLine(x,y+2,17);
  HAL::drawPixel(x+2,y+1);HAL::drawPixel(x+12,y+1);HAL::drawPixel(x+16,y+1);
}
void ground(uint32_t offset){
  HAL::drawHLine(0,ground_y,128);
  for(uint8_t index=0;index<10u;++index){
    const int16_t x=(int16_t)((index*17u+128u-(offset%17u))%136u)-4;
    HAL::drawHLine(x,ground_y+4,6);HAL::drawPixel(x+8,ground_y+2);
  }
}
void draw_dino_sprite(int16_t y,const DinoSprite &sprite){
  HAL::drawBMP(dino_x,y,dino_width,dino_height,sprite);
}
void dino(int16_t y,bool running,bool grounded,uint32_t phase){
  if(running&&!grounded){draw_dino_sprite(y,dino_jump);return;}
  draw_dino_sprite(y,running&&(phase&1u)!=0u?dino_run_b:dino_run_a);
}
void cactus(int16_t x,uint8_t width,uint8_t height){
  const int16_t y=ground_y-height;
  HAL::drawRBox(x+2,y,width,height,2);
  if(height>=12u){
    HAL::drawVLine(x,y+4,6);HAL::drawHLine(x,y+9,4);
    HAL::drawVLine(x+width+3,y+6,5);HAL::drawHLine(x+width+1,y+10,4);
  }
}
}

void DinoPage::enter(uint32_t now_ms){
  uint32_t stored=0;if(storage_service_read(StorageKey::DinoHighScore,stored))high_score_=stored;
  state_=State::Ready;last_update_ms_=now_ms;run_time_ms_=0;score_=0;
  dino_y_q8_=resting_y*q8;velocity_y_q8_=0;grounded_=true;obstacle_x_q8_=142*q8;
}
void DinoPage::exit(uint32_t now_ms){
  (void)now_ms;
  if(score_>high_score_){high_score_=score_;(void)storage_service_write(StorageKey::DinoHighScore,high_score_);}
  state_=State::Ready;
}
void DinoPage::start(uint32_t now_ms){
  state_=State::Running;last_update_ms_=now_ms;run_time_ms_=0;score_=0;
  dino_y_q8_=resting_y*q8;velocity_y_q8_=0;grounded_=true;obstacle_x_q8_=142*q8;
  obstacle_width_=7;obstacle_height_=12;
}
void DinoPage::jump(){
  if(!grounded_)return;
  velocity_y_q8_=-220*q8;grounded_=false;
}
void DinoPage::next_obstacle(){
  random_=(uint16_t)((random_>>1)^((uint16_t)(-(int16_t)(random_&1u))&0xb400u));
  obstacle_width_=(uint8_t)(5u+(random_&0x03u));
  obstacle_height_=(uint8_t)(9u+((random_>>3)&0x09u));
  const uint16_t gap=(uint16_t)(54u+((random_>>7)%43u));
  obstacle_x_q8_=(128+(int32_t)gap)*q8;
}
bool DinoPage::collided() const {
  const int16_t dino_y=(int16_t)(dino_y_q8_/q8);
  const int16_t obstacle_x=(int16_t)(obstacle_x_q8_/q8);
  const int16_t obstacle_y=ground_y-obstacle_height_;
  return dino_x+14>obstacle_x+1&&dino_x+3<obstacle_x+obstacle_width_+2&&
         dino_y+14>obstacle_y+1&&dino_y+2<ground_y;
}
void DinoPage::handle(BspKey event,uint32_t now_ms){
  if(event!=BSP_KEY_ENTER&&event!=BSP_KEY_UP)return;
  if(state_==State::Ready||state_==State::GameOver){start(now_ms);return;}
  jump();
}
void DinoPage::update(uint32_t now_ms){
  uint32_t delta=now_ms-last_update_ms_;last_update_ms_=now_ms;
  if(state_!=State::Running)return;
  delta=std::min<uint32_t>(delta,50u);run_time_ms_+=delta;score_=run_time_ms_/100u;
  velocity_y_q8_+=(int32_t)(800*q8*(int64_t)delta/1000);
  dino_y_q8_+=(int32_t)(velocity_y_q8_*(int64_t)delta/1000);
  if(dino_y_q8_>=resting_y*q8){dino_y_q8_=resting_y*q8;velocity_y_q8_=0;grounded_=true;}
  const uint32_t speed=58u+std::min<uint32_t>(38u,score_/15u);
  obstacle_x_q8_-=(int32_t)(speed*q8*delta/1000u);
  if(obstacle_x_q8_<-(int32_t)(obstacle_width_+4u)*q8)next_obstacle();
  if(collided()){
    state_=State::GameOver;
    if(score_>high_score_){high_score_=score_;(void)storage_service_write(StorageKey::DinoHighScore,high_score_);}
  }
}
void DinoPage::render(uint32_t now_ms){
  HAL::canvasClear();HAL::setDrawType(1);score_text(score_,high_score_);
  HAL::drawHDottedLine(0,13,128);
  const int16_t cloud_x=(int16_t)(116-(int32_t)((run_time_ms_/70u)%154u));
  cloud(cloud_x,19);ground(run_time_ms_/16u);
  dino((int16_t)(dino_y_q8_/q8),state_==State::Running,grounded_,now_ms/120u);
  if(state_!=State::Ready)cactus((int16_t)(obstacle_x_q8_/q8),obstacle_width_,obstacle_height_);
  if(state_==State::Ready){
    HAL::drawRFrame(37,21,77,21,5);HAL::drawEnglish(51,30,"DINO RUN");HAL::drawEnglish(49,40,"OK START");
  }else if(state_==State::GameOver){
    HAL::setDrawType(0);HAL::drawRBox(27,19,91,27,5);HAL::setDrawType(1);
    HAL::drawRFrame(27,19,91,27,5);HAL::drawEnglish(44,30,"GAME OVER");HAL::drawEnglish(45,41,"OK RETRY");
  }
  HAL::canvasUpdate();
}
