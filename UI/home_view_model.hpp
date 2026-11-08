#pragma once
#include <cstdint>

struct HomeSnapshot {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t battery_percent;
  bool battery_valid;
  bool battery_low;
  bool alarm_enabled;
  int8_t temperature_c;
  uint8_t humidity_percent;
  uint32_t steps;
  uint8_t activity_percent;
  uint8_t heart_rate;
  uint8_t spo2_percent;
  bool climate_valid;
  bool heart_valid;
  bool spo2_valid;
};

class HomeViewModel final {
 public:
  void begin(uint32_t now_ms);
  void update(uint32_t now_ms);
  const HomeSnapshot &snapshot() const { return snapshot_; }

 private:
  HomeSnapshot snapshot_{};
};
