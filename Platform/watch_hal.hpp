#ifndef WATCH_ASTRA_HAL_HPP
#define WATCH_ASTRA_HAL_HPP
#include "hal/hal.h"
#include "u8g2.h"

class WatchHal final : public HAL {
public:
  void init() override;
  std::string type() override { return "STM32U575/SSD1306"; }
  void *_getCanvasBuffer() override;
  uint8_t _getBufferTileHeight() override;
  uint8_t _getBufferTileWidth() override;
  void _canvasUpdate() override;
  void _canvasClear() override;
  void _setFont(const uint8_t *font) override;
  uint8_t _getFontWidth(std::string &text) override;
  uint8_t _getFontHeight() override;
  void _setDrawType(uint8_t type) override;
  void _drawPixel(float x,float y) override;
  void _drawEnglish(float x,float y,const std::string &text) override;
  void _drawChinese(float x,float y,const std::string &text) override;
  void _drawVDottedLine(float x,float y,float height) override;
  void _drawHDottedLine(float x,float y,float length) override;
  void _drawVLine(float x,float y,float height) override;
  void _drawHLine(float x,float y,float length) override;
  void _drawBMP(float x,float y,float width,float height,const uint8_t *bitmap) override;
  void _drawBox(float x,float y,float width,float height) override;
  void _drawRBox(float x,float y,float width,float height,float radius) override;
  void _drawFrame(float x,float y,float width,float height) override;
  void _drawRFrame(float x,float y,float width,float height,float radius) override;
  void _delay(unsigned long ms) override;
  unsigned long _millis() override;
  unsigned long _getTick() override;
  void _screenOn() override;
  void _screenOff() override;
  void _setScreenBrightness(uint8_t value) override;
private:
  u8g2_t canvas_{};
};
#endif
