#include "health_page.hpp"

#include "hal/hal.h"
#include "health_service.hpp"
#include "settings_service.hpp"
#include <cstdio>
#include <string>

namespace {
constexpr uint8_t digits[10][5]={
  {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
  {7,4,7,1,7},{7,4,7,5,7},{7,1,2,2,2},{7,5,7,5,7},{7,5,7,1,7}
};
void digit(float x,float y,uint8_t value){
  for(uint8_t row=0;row<5;++row)for(uint8_t column=0;column<3;++column)
    if((digits[value%10u][row]&(4u>>column))!=0u)HAL::drawBox(x+column*2,y+row*2,2,2);
}
void number(float cell_x,float y,uint8_t value,bool valid){
  if(!valid){HAL::drawBox(cell_x+20,y+4,6,2);HAL::drawBox(cell_x+29,y+4,6,2);return;}
  const uint8_t count=value>=100u?3u:2u;
  const float width=count==3u?20.0f:13.0f;
  float x=cell_x+(64.0f-width)/2.0f;
  if(count==3u){digit(x,y,value/100u);x+=7;}
  digit(x,y,(value/10u)%10u);digit(x+7,y,value%10u);
}
void centered(float baseline,const std::string &text){
  std::string measured=text;HAL::drawChinese((128.0f-HAL::getFontWidth(measured))/2.0f,baseline,text);
}
void progress(uint8_t percent,uint32_t raw_ir,uint32_t samples){
  HAL::drawFrame(12,39,104,8);
  const float width=100.0f*percent/100.0f;if(width>=1)HAL::drawBox(14,41,width,4);
  char text[30]{};std::snprintf(text,sizeof(text),"%u%% IR:%lu S:%lu",percent,
    static_cast<unsigned long>(raw_ir),static_cast<unsigned long>(samples%10000u));centered(61,text);
}
}

void HealthPage::handle(BspKey event,uint32_t now_ms){
  (void)now_ms;if(event==BSP_KEY_ENTER)health_service_restart();
}

void HealthPage::render(uint32_t now_ms){
  (void)now_ms;const HealthSnapshot snapshot=health_service_snapshot();
  HAL::canvasClear();HAL::setDrawType(1);centered(11,"心率血氧");
  HAL::drawHDottedLine(0,14,128);
  if(!snapshot.connected){
    centered(30,"MAX30102 OFFLINE");
    const char *reason=snapshot.sensor_status==1u?"I2C: NO ACK":
                       snapshot.sensor_status==2u?"ID READ FAILED":
                       snapshot.sensor_status==3u?"RESET FAILED":
                       snapshot.sensor_status==4u?"CONFIG FAILED":"DATA FAILED";
    centered(45,reason);
    char diagnostic[20]{};std::snprintf(diagnostic,sizeof(diagnostic),"PC0/PC1 ID:%02X",snapshot.sensor_id);
    centered(60,diagnostic);HAL::canvasUpdate();return;
  }
  if(!snapshot.finger_detected){
    centered(34,"PLACE FINGER");
    char signal[28]{};std::snprintf(signal,sizeof(signal),"IR:%lu S:%lu",
      static_cast<unsigned long>(snapshot.raw_ir),static_cast<unsigned long>(snapshot.sample_count%10000u));
    centered(55,signal);HAL::canvasUpdate();return;
  }
  if(snapshot.progress_percent<100u){progress(snapshot.progress_percent,snapshot.raw_ir,snapshot.sample_count);HAL::canvasUpdate();return;}

  HAL::drawEnglish(5,25,"HR");HAL::drawEnglish(69,25,"SPO2");
  number(0,27,snapshot.heart_rate,snapshot.heart_valid);
  number(64,27,snapshot.spo2_percent,snapshot.spo2_valid);
  HAL::drawEnglish(47,37,"BPM");HAL::drawEnglish(116,37,"%");
  if(!snapshot.heart_valid){centered(60,"ADJUST FINGER");HAL::canvasUpdate();return;}

  for(uint8_t index=0;index<63u;++index){
    const float x=index*2.0f;
    const float y1=43.0f+snapshot.waveform[index];
    const float y2=43.0f+snapshot.waveform[index+1u];
    HAL::drawHLine(x,y1,2);HAL::drawVLine(x+1,std::min(y1,y2),std::abs(y2-y1)+1);
  }
  const SystemSettings settings=settings_service_snapshot();
  char quality[12]{};
  if(settings.heart_low_enabled&&snapshot.heart_rate<settings.heart_low)std::snprintf(quality,sizeof(quality),"LOW");
  else if(settings.heart_high_enabled&&snapshot.heart_rate>settings.heart_high)std::snprintf(quality,sizeof(quality),"HIGH");
  else std::snprintf(quality,sizeof(quality),"Q%u",snapshot.signal_quality);
  HAL::drawEnglish(105,63,quality);HAL::canvasUpdate();
}
