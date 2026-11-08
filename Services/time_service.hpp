#pragma once
#include <cstdint>

struct TimeSnapshot {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;  // 0=Sunday, 1=Monday ... 6=Saturday
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  bool valid;
};

void time_service_start();
TimeSnapshot time_service_snapshot();
bool time_service_set(TimeSnapshot value);
