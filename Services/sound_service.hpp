#pragma once

#include <cstdint>

struct SoundSettings {
  bool master_enabled;
  bool key_tone_enabled;
};

void sound_service_start();
SoundSettings sound_service_snapshot();
bool sound_service_set(SoundSettings value);
void sound_service_key_click();
void sound_service_alarm_output(bool enabled);
