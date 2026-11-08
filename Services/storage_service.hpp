#pragma once

#include <cstdint>

enum class StorageKey:uint8_t {
  SystemSettings=1,
  PersonalProfile=2,
  DinoHighScore=3,
  AlarmOne=4,
  AlarmTwo=5,
  SoundSettings=6,
  DisplaySettings=7
};

struct StorageStatus {
  uint32_t successful_writes;
  uint16_t error_count;
  uint8_t active_page;
  bool ready;
};

void storage_service_start();
bool storage_service_read(StorageKey key,uint32_t &value);
bool storage_service_write(StorageKey key,uint32_t value);
StorageStatus storage_service_status();
