#pragma once
#include <cstdint>

struct HomeSnapshot;

class BootScene final {
 public:
  void begin(uint32_t now_ms){started_ms_=now_ms;}
  bool render(uint32_t now_ms,const HomeSnapshot &snapshot) const;

 private:
  uint32_t started_ms_{};
};
