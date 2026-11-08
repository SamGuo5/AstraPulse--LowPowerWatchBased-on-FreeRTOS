#include "watch_hal.hpp"
#include "watch_chinese_font.hpp"
#include "bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
uint8_t i2c_buffer[160];
uint16_t i2c_length;

bool display_write(uint8_t control,const uint8_t *data,uint16_t length) {
  if(length>128)return false;
  uint8_t packet[129];
  packet[0]=control;
  std::memcpy(packet+1,data,length);
  return HAL_I2C_Master_Transmit(&hi2c2,0x3c<<1,packet,length+1,20)==HAL_OK;
}

uint8_t display_i2c(u8x8_t *u8x8,uint8_t message,uint8_t count,void *data) {
  switch(message) {
    case U8X8_MSG_BYTE_INIT:return 1;
    case U8X8_MSG_BYTE_START_TRANSFER:i2c_length=0;return 1;
    case U8X8_MSG_BYTE_SEND:
      if(i2c_length+count>sizeof(i2c_buffer))return 0;
      std::memcpy(i2c_buffer+i2c_length,data,count);i2c_length+=count;return 1;
    case U8X8_MSG_BYTE_END_TRANSFER:
      return HAL_I2C_Master_Transmit(&hi2c2,u8x8_GetI2CAddress(u8x8),i2c_buffer,i2c_length,30)==HAL_OK;
    case U8X8_MSG_BYTE_SET_DC:return 1;
    default:return 0;
  }
}

uint8_t display_delay(u8x8_t *,uint8_t message,uint8_t arg,void *) {
  if(message==U8X8_MSG_DELAY_MILLI)HAL_Delay(arg);
  return 1;
}

int16_t pixel(float value){return static_cast<int16_t>(std::lround(value));}
uint8_t extent(float value){return static_cast<uint8_t>(std::max(0L,std::lround(value)));}
}

void WatchHal::init() {
  config.screenWeight=128;config.screenHeight=64;config.screenBright=255;
  HAL_Delay(200);
  u8g2_Setup_ssd1306_128x64_noname_f(&canvas_,U8G2_R0,display_i2c,display_delay);
  /* Astra's reduced u8g2 setup selects the generic 3-wire CAD adapter.  On
     SSD1306 I2C that omits the mandatory 0x00 command / 0x40 data control
     byte, so framebuffer bytes can be decoded as display on/off commands and
     make the panel flash.  Keep the same renderer but install the I2C CAD
     transport before the controller is initialized. */
  canvas_.u8x8.cad_cb=u8x8_cad_ssd13xx_i2c;
  u8x8_SetI2CAddress(&canvas_.u8x8,0x3c<<1);
  u8g2_InitDisplay(&canvas_);u8g2_SetPowerSave(&canvas_,0);
  u8g2_SetFontMode(&canvas_,1);u8g2_SetBitmapMode(&canvas_,1);
  u8g2_SetFont(&canvas_,u8g2_font_myfont);u8g2_ClearBuffer(&canvas_);u8g2_SendBuffer(&canvas_);
}
void *WatchHal::_getCanvasBuffer(){return u8g2_GetBufferPtr(&canvas_);}
uint8_t WatchHal::_getBufferTileHeight(){return u8g2_GetBufferTileHeight(&canvas_);}
uint8_t WatchHal::_getBufferTileWidth(){return u8g2_GetBufferTileWidth(&canvas_);}
void WatchHal::_canvasUpdate(){
  /* One command and one 128-byte data transaction per page.  The generic
     u8g2 I2C adapter fragments every page into small Arduino-sized packets,
     adding dozens of START/STOP cycles per frame. */
  const auto *frame=u8g2_GetBufferPtr(&canvas_);
  for(uint8_t page=0;page<8;++page){
    const uint8_t address[]={static_cast<uint8_t>(0xb0u|page),0x00,0x10};
    if(!display_write(0x00,address,sizeof(address)) ||
       !display_write(0x40,frame+page*128u,128))return;
  }
}
void WatchHal::_canvasClear(){u8g2_ClearBuffer(&canvas_);}
void WatchHal::_setFont(const uint8_t *font){if(font)u8g2_SetFont(&canvas_,font);}
uint8_t WatchHal::_getFontWidth(std::string &text){return static_cast<uint8_t>(std::min<uint16_t>(255,watch_font::utf8_width(text)));}
uint8_t WatchHal::_getFontHeight(){return static_cast<uint8_t>(u8g2_GetMaxCharHeight(&canvas_));}
void WatchHal::_setDrawType(uint8_t type){u8g2_SetDrawColor(&canvas_,type>2?1:type);}
void WatchHal::_drawPixel(float x,float y){u8g2_DrawPixel(&canvas_,pixel(x),pixel(y));}
void WatchHal::_drawEnglish(float x,float y,const std::string &text){u8g2_DrawUTF8(&canvas_,pixel(x),pixel(y),text.c_str());}
void WatchHal::_drawChinese(float x,float y,const std::string &text){watch_font::draw_utf8(&canvas_,pixel(x),pixel(y),text);}
void WatchHal::_drawVDottedLine(float x,float y,float height){for(int i=0;i<pixel(height);i+=3)u8g2_DrawPixel(&canvas_,pixel(x),pixel(y)+i);}
void WatchHal::_drawHDottedLine(float x,float y,float length){for(int i=0;i<pixel(length);i+=3)u8g2_DrawPixel(&canvas_,pixel(x)+i,pixel(y));}
void WatchHal::_drawVLine(float x,float y,float height){u8g2_DrawVLine(&canvas_,pixel(x),pixel(y),extent(height));}
void WatchHal::_drawHLine(float x,float y,float length){u8g2_DrawHLine(&canvas_,pixel(x),pixel(y),extent(length));}
void WatchHal::_drawBMP(float x,float y,float width,float height,const uint8_t *bitmap){u8g2_DrawXBMP(&canvas_,pixel(x),pixel(y),extent(width),extent(height),bitmap);}
void WatchHal::_drawBox(float x,float y,float width,float height){u8g2_DrawBox(&canvas_,pixel(x),pixel(y),extent(width),extent(height));}
void WatchHal::_drawRBox(float x,float y,float width,float height,float radius){u8g2_DrawRBox(&canvas_,pixel(x),pixel(y),extent(width),extent(height),extent(radius));}
void WatchHal::_drawFrame(float x,float y,float width,float height){u8g2_DrawFrame(&canvas_,pixel(x),pixel(y),extent(width),extent(height));}
void WatchHal::_drawRFrame(float x,float y,float width,float height,float radius){u8g2_DrawRFrame(&canvas_,pixel(x),pixel(y),extent(width),extent(height),extent(radius));}
void WatchHal::_delay(unsigned long ms){HAL_Delay(ms);}
unsigned long WatchHal::_millis(){return HAL_GetTick();}
unsigned long WatchHal::_getTick(){return xTaskGetTickCount();}
void WatchHal::_screenOn(){u8g2_SetPowerSave(&canvas_,0);}
void WatchHal::_screenOff(){u8g2_SetPowerSave(&canvas_,1);}
void WatchHal::_setScreenBrightness(uint8_t value){config.screenBright=value;u8g2_SetContrast(&canvas_,value);}
