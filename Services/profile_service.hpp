#pragma once

#include <cstdint>

struct PersonalProfile {
  uint8_t height_cm;
  uint8_t weight_kg;
};

void profile_service_start();
PersonalProfile profile_service_snapshot();
bool profile_service_set(PersonalProfile value);
