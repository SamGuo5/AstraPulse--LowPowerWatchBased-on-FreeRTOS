#pragma once
#include <cstdint>

struct ClimateSnapshot {
  int16_t temperature_tenths;
  uint16_t humidity_tenths;
  uint32_t sample_count;
  uint16_t error_count;
  bool valid;
};

void climate_service_start();
ClimateSnapshot climate_service_snapshot();
