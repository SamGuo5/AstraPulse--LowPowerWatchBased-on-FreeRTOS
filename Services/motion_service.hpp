#pragma once
#include <cstdint>

enum class MotionDeviceState:uint8_t { Starting,Offline,Unsupported,Running,Fault };
enum class MotionLevel:uint8_t { Still,Walking,Active };

struct MotionSnapshot {
  uint32_t steps;
  uint32_t sample_count;
  uint16_t error_count;
  int16_t ax_mg;
  int16_t ay_mg;
  int16_t az_mg;
  uint16_t magnitude_mg;
  uint8_t activity_percent;
  uint8_t device_id;
  uint8_t ack_address;
  uint8_t i2c_initialized;
  uint8_t scl_high;
  uint8_t sda_high;
  uint8_t enable_high;
  uint32_t i2c_error;
  MotionDeviceState device_state;
  MotionLevel level;
  bool connected;
};

void motion_service_start();
MotionSnapshot motion_service_snapshot();
