#pragma once
#include <cstdint>

struct AlarmConfig {
  uint8_t hour;
  uint8_t minute;
  bool enabled;
};

struct AlarmSnapshot {
  AlarmConfig alarms[2];
  bool ringing;
  uint8_t active_index;
};

void alarm_service_start();
AlarmSnapshot alarm_service_snapshot();
bool alarm_service_set(uint8_t index,AlarmConfig config);
void alarm_service_dismiss();
