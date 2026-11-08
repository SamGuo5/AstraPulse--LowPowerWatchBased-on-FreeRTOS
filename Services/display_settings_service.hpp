#pragma once

#include <cstdint>

struct DisplaySettings {
  uint8_t brightness_level;  // 1..8
  uint8_t timeout_index;     // OFF, 15, 30, 60, 120 seconds
};

void display_settings_service_start();
DisplaySettings display_settings_service_snapshot();
bool display_settings_service_set(DisplaySettings value);
uint8_t display_settings_contrast(DisplaySettings value);
uint16_t display_settings_timeout_seconds(DisplaySettings value);
