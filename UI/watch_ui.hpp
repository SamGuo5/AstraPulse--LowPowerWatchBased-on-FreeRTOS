#pragma once
#include "bsp.h"
#include "alarm_page.hpp"
#include "astra/ui/launcher.h"
#include "boot_scene.hpp"
#include "climate_page.hpp"
#include "date_time_page.hpp"
#include "dino_page.hpp"
#include "display_settings_page.hpp"
#include "feature_page.hpp"
#include "health_page.hpp"
#include "home_view_model.hpp"
#include "motion_page.hpp"
#include "personal_info_page.hpp"
#include "stopwatch_page.hpp"
#include "sound_settings_page.hpp"
#include "system_settings_page.hpp"
#include "watch_face.hpp"

class WatchUi final {
 public:
  void begin();
  void handle(BspKey event);
  void render();

 private:
  enum class Mode:uint8_t { Boot,Home,Menu,Feature };
  void open_feature(FeaturePage *page,uint32_t now_ms);
  void wake_display(uint32_t now_ms);
  Mode mode_{Mode::Boot};
  astra::Menu *root_{};
  astra::Menu *system_settings_menu_{};
  astra::Menu *personal_info_menu_{};
  astra::Menu *climate_menu_{};
  astra::Menu *health_menu_{};
  astra::Menu *motion_menu_{};
  astra::Menu *dino_menu_{};
  astra::Menu *date_time_menu_{};
  astra::Menu *alarm_menu_{};
  astra::Menu *stopwatch_menu_{};
  astra::Menu *sound_settings_menu_{};
  astra::Menu *display_settings_menu_{};
  FeaturePage *active_page_{};
  astra::Launcher launcher_;
  BootScene boot_;
  HomeViewModel home_model_;
  WatchFace watch_face_;
  ClimatePage climate_page_;
  HealthPage health_page_;
  MotionPage motion_page_;
  DinoPage dino_page_;
  DateTimePage date_time_page_;
  AlarmPage alarm_page_;
  StopwatchPage stopwatch_page_;
  SystemSettingsPage system_settings_page_;
  PersonalInfoPage personal_info_page_;
  SoundSettingsPage sound_settings_page_;
  DisplaySettingsPage display_settings_page_;
  uint32_t last_activity_ms_{};
  bool screen_sleeping_{};
};
