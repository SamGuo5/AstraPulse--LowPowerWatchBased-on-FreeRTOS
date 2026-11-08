#pragma once
#include <cstdint>

struct HealthSnapshot {
  uint8_t heart_rate;
  uint8_t spo2_percent;
  uint8_t signal_quality;
  uint8_t progress_percent;
  uint8_t sensor_status;
  uint8_t sensor_id;
  uint8_t waveform[64];
  uint32_t raw_ir;
  uint32_t sample_count;
  uint16_t error_count;
  bool connected;
  bool finger_detected;
  bool heart_valid;
  bool spo2_valid;
};

void health_service_start();
void health_service_restart();
HealthSnapshot health_service_snapshot();
