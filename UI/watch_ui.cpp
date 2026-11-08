#include "watch_ui.hpp"
#include "feature_icons.hpp"
#include "hal/hal.h"
#include "alarm_service.hpp"
#include "sound_service.hpp"
#include "display_settings_service.hpp"
#include "power_policy_service.hpp"

namespace {
astra::Menu *feature(const char *title,FeatureIcon icon,
                     const char *first, const char *second) {
  auto *page = new astra::Menu(title,make_feature_icon(icon));
  page->addItem(new astra::Menu(first));
  page->addItem(new astra::Menu(second));
  return page;
}
}

void WatchUi::begin() {
  root_ = new astra::Menu("智能手表");
  system_settings_menu_=feature("系统设置",FeatureIcon::SystemSettings,"指示灯","心率设置");
  root_->addItem(system_settings_menu_);
  personal_info_menu_=feature("个人信息",FeatureIcon::PersonalInfo,"身高","体重");
  root_->addItem(personal_info_menu_);
  climate_menu_=feature("温湿度",FeatureIcon::Climate,"温度","湿度");
  root_->addItem(climate_menu_);
  health_menu_=feature("心率血氧",FeatureIcon::HeartOxygen,"心率","血氧");
  root_->addItem(health_menu_);
  motion_menu_=feature("运动监测",FeatureIcon::Motion,"步数","活动量");
  root_->addItem(motion_menu_);
  dino_menu_=feature("恐龙游戏",FeatureIcon::DinoGame,"开始游戏","最高分");
  root_->addItem(dino_menu_);
  date_time_menu_=feature("日期时间",FeatureIcon::DateTime,"日期","时间");
  root_->addItem(date_time_menu_);
  alarm_menu_=feature("闹钟设置",FeatureIcon::Alarm,"闹钟一","闹钟二");
  root_->addItem(alarm_menu_);
  stopwatch_menu_=feature("秒表",FeatureIcon::Stopwatch,"开始暂停","复位");
  root_->addItem(stopwatch_menu_);
  sound_settings_menu_=feature("声音设置",FeatureIcon::Sound,"总声音","按键音");
  root_->addItem(sound_settings_menu_);
  display_settings_menu_=feature("屏幕亮度",FeatureIcon::Brightness,"亮度等级","自动休眠");
  root_->addItem(display_settings_menu_);
  launcher_.init(root_);
  const uint32_t now=HAL::millis();
  HAL::setScreenBrightness(display_settings_contrast(display_settings_service_snapshot()));
  power_policy_set_screen_sleeping(false);
  last_activity_ms_=now;screen_sleeping_=false;
  home_model_.begin(now);
  date_time_page_.bind(&home_model_.snapshot());
  boot_.begin(now);
  mode_=Mode::Boot;
}

void WatchUi::open_feature(FeaturePage *page,uint32_t now_ms){
  active_page_=page;
  active_page_->enter(now_ms);
  mode_=Mode::Feature;
}

void WatchUi::wake_display(uint32_t now_ms){
  power_policy_set_screen_sleeping(false);
  HAL::screenOn();
  HAL::setScreenBrightness(display_settings_contrast(display_settings_service_snapshot()));
  screen_sleeping_=false;last_activity_ms_=now_ms;
}

void WatchUi::handle(BspKey event) {
  const uint32_t now=HAL::millis();
  const bool alarm_ringing=alarm_service_snapshot().ringing;
  if(screen_sleeping_){wake_display(now);if(!alarm_ringing)return;}
  last_activity_ms_=now;
  if(alarm_ringing){alarm_service_dismiss();return;}
  sound_service_key_click();
  if(mode_==Mode::Boot){mode_=Mode::Home;return;}
  if(mode_==Mode::Home){
    if(event==BSP_KEY_ENTER)mode_=Mode::Menu;
    return;
  }
  if(mode_==Mode::Feature){
    if(event==BSP_KEY_BACK){active_page_->exit(now);active_page_=nullptr;mode_=Mode::Menu;}
    else active_page_->handle(event,now);
    return;
  }
  if (event == BSP_KEY_UP) launcher_.previous();
  else if (event == BSP_KEY_DOWN) launcher_.next();
  else if (event == BSP_KEY_ENTER) {
    astra::Menu *selected=launcher_.selected();
    if(launcher_.current()==root_ && selected==system_settings_menu_)open_feature(&system_settings_page_,now);
    else if(launcher_.current()==root_ && selected==personal_info_menu_)open_feature(&personal_info_page_,now);
    else if(launcher_.current()==root_ && selected==climate_menu_)open_feature(&climate_page_,now);
    else if(launcher_.current()==root_ && selected==health_menu_)open_feature(&health_page_,now);
    else if(launcher_.current()==root_ && selected==motion_menu_)open_feature(&motion_page_,now);
    else if(launcher_.current()==root_ && selected==dino_menu_)open_feature(&dino_page_,now);
    else if(launcher_.current()==root_ && selected==date_time_menu_)open_feature(&date_time_page_,now);
    else if(launcher_.current()==root_ && selected==alarm_menu_)open_feature(&alarm_page_,now);
    else if(launcher_.current()==root_ && selected==stopwatch_menu_)open_feature(&stopwatch_page_,now);
    else if(launcher_.current()==root_ && selected==sound_settings_menu_)open_feature(&sound_settings_page_,now);
    else if(launcher_.current()==root_ && selected==display_settings_menu_)open_feature(&display_settings_page_,now);
    else launcher_.open();
  }
  else if (event == BSP_KEY_BACK) {
    if(launcher_.current()==root_)mode_=Mode::Home;
    else launcher_.close();
  }
}

void WatchUi::render() {
  const uint32_t now=HAL::millis();
  home_model_.update(now);
  const AlarmSnapshot alarm=alarm_service_snapshot();
  if(alarm.ringing){
    if(screen_sleeping_)wake_display(now);
    last_activity_ms_=now;
    alarm_page_.render_ringing(now,alarm);return;
  }
  if(screen_sleeping_)return;
  if(mode_!=Mode::Boot&&mode_!=Mode::Feature){
    const uint16_t timeout=display_settings_timeout_seconds(display_settings_service_snapshot());
    if(timeout>0u&&now-last_activity_ms_>=static_cast<uint32_t>(timeout)*1000u){
      HAL::screenOff();screen_sleeping_=true;power_policy_set_screen_sleeping(true);return;
    }
  }
  if(mode_==Mode::Boot){
    if(boot_.render(now,home_model_.snapshot())){
      mode_=Mode::Home;
      watch_face_.render(home_model_.snapshot());
    }
    return;
  }
  if(mode_==Mode::Home){
    watch_face_.render(home_model_.snapshot());
    return;
  }
  if(mode_==Mode::Feature){
    active_page_->update(now);
    active_page_->render(now);
    return;
  }
  launcher_.update();
}
