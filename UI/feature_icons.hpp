#pragma once

#include <cstdint>
#include <vector>

enum class FeatureIcon : uint8_t {
  SystemSettings,
  PersonalInfo,
  Climate,
  HeartOxygen,
  Motion,
  DinoGame,
  DateTime,
  Alarm,
  Stopwatch,
  Sound,
  Brightness
};

std::vector<uint8_t> make_feature_icon(FeatureIcon icon);
