#pragma once
#include <cstdint>

struct PowerSnapshot {
  uint16_t battery_mv;
  uint32_t sample_count;
  uint16_t error_count;
  uint8_t battery_percent;
  bool valid;
  bool low;
};

void power_service_start();
PowerSnapshot power_service_snapshot();
