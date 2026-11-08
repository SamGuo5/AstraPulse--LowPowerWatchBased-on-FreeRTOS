#pragma once

#include <cstdint>

struct SystemSettings {
  bool status_led;
  bool heart_low_enabled;
  bool heart_high_enabled;
  uint8_t heart_low;
  uint8_t heart_high;
};

void settings_service_start();
SystemSettings settings_service_snapshot();
bool settings_service_set(SystemSettings value);
