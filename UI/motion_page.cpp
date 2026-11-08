#include "motion_page.hpp"

#include "hal/hal.h"
#include "motion_service.hpp"
#include <cstdlib>
#include <cstdio>
#include <string>

namespace {
void centered(float baseline,const std::string &text){
  std::string measured=text;HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
const char *level_name(MotionLevel level){
  return level==MotionLevel::Walking?"WALK":(level==MotionLevel::Active?"ACTIVE":"STILL");
}
}

void MotionPage::enter(uint32_t now_ms){entered_at_=now_ms;details_=false;}
void MotionPage::handle(BspKey event,uint32_t now_ms){(void)now_ms;if(event==BSP_KEY_ENTER)details_=!details_;}

void MotionPage::render(uint32_t now_ms){
  const MotionSnapshot snapshot=motion_service_snapshot();const bool demo=!snapshot.connected;
  const uint32_t elapsed=now_ms-entered_at_;const uint32_t phase=(elapsed/60u)%80u;
  const int32_t wave=phase<40u?(int32_t)phase:(int32_t)(80u-phase);
  const uint32_t steps=demo?1280u+elapsed/720u:snapshot.steps;
  const uint8_t activity=demo?(uint8_t)(28+wave*57/40):snapshot.activity_percent;
  const MotionLevel level=demo?MotionLevel::Walking:snapshot.level;
  const int16_t ax=demo?(int16_t)((wave-20)*11):snapshot.ax_mg;
  const int16_t ay=demo?(int16_t)((20-std::abs(wave-20))*9-90):snapshot.ay_mg;
  const int16_t az=demo?(int16_t)(980+(wave-20)*4):snapshot.az_mg;
  const uint16_t magnitude=demo?(uint16_t)(1000+wave*3):snapshot.magnitude_mg;
  HAL::canvasClear();HAL::setDrawType(1);
  centered(11,"运动监测");HAL::drawEnglish(demo?103:99,11,demo?"DEMO":(details_?"RAW":"READY"));HAL::drawHDottedLine(0,14,128);
  if(details_){
    char line[28]{};
    if(demo)std::snprintf(line,sizeof(line),"MODE:SIM N:%lu",(unsigned long)(elapsed/10u));
    else std::snprintf(line,sizeof(line),"ID:%02X  N:%lu",snapshot.device_id,(unsigned long)snapshot.sample_count);
    HAL::drawEnglish(4,27,line);
    std::snprintf(line,sizeof(line),"AX:%d AY:%d",ax,ay);HAL::drawEnglish(4,39,line);
    std::snprintf(line,sizeof(line),"AZ:%d M:%u",az,magnitude);HAL::drawEnglish(4,51,line);
    HAL::drawEnglish(4,63,demo?"DEMO DATA":"SENSOR DATA");
    HAL::drawEnglish(75,63,"OK SUMMARY");HAL::canvasUpdate();return;
  }
  HAL::drawChinese(5,28,"步数");
  char step_text[16]{};std::snprintf(step_text,sizeof(step_text),"%lu",(unsigned long)steps);HAL::drawEnglish(43,29,step_text);
  HAL::drawChinese(5,43,"活动量");HAL::drawEnglish(93,43,level_name(level));
  HAL::drawFrame(5,48,118,7);const float fill=114.0f*activity/100.0f;
  if(fill>=1.0f)HAL::drawBox(7,50,fill,3);
  char activity_text[12]{};std::snprintf(activity_text,sizeof(activity_text),"%u%%",activity);
  HAL::drawEnglish(5,63,activity_text);HAL::drawEnglish(72,63,"OK DETAIL");HAL::canvasUpdate();
}
