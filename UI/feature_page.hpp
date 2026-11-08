#pragma once

#include "bsp.h"
#include <cstdint>

class FeaturePage {
 public:
  virtual ~FeaturePage()=default;
  virtual void enter(uint32_t now_ms) {(void)now_ms;}
  virtual void exit(uint32_t now_ms) {(void)now_ms;}
  virtual void handle(BspKey event,uint32_t now_ms)=0;
  virtual void update(uint32_t now_ms) {(void)now_ms;}
  virtual void render(uint32_t now_ms)=0;
};
